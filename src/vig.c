#include "c.h"
#define I(f) vig_##f

/*
 * The VIG backend deliberately emits VIGasm directly instead of reusing the
 * diagnostic bytecode syntax.  lcc's trees already preserve the order in
 * which values must be evaluated; this file turns their stack effects into
 * the corresponding VIG instructions.
 */

static int parameter_slots;
static int function_returns_value;
static Symbol struct_return_pointer;
static void dumptree(Node);

/* A `#pragma vig import' names the native library and symbol that a C function
 * declaration represents.  lcc parses pragmas before it finalizes unresolved
 * externals, so retaining this small table here lets the normal import pass
 * emit the matching VIGasm `extern' declaration. */
typedef struct ForeignImport ForeignImport;
struct ForeignImport {
	char *name;
	char *library;
	char *symbol;
	ForeignImport *link;
};
static ForeignImport *foreign_imports;

static ForeignImport *foreign_import(char *name) {
	ForeignImport *entry;

	for (entry = foreign_imports; entry; entry = entry->link)
		if (strcmp(entry->name, name) == 0)
			return entry;
	return NULL;
}

static void I(foreign_import)(char *name, char *library, char *symbol) {
	ForeignImport *entry;

	if (foreign_import(name) != NULL) {
		error("vig: foreign import %s is declared more than once\n", name);
		return;
	}
	NEW(entry, PERM);
	entry->name = name;
	entry->library = library;
	entry->symbol = symbol;
	entry->link = foreign_imports;
	foreign_imports = entry;
}

static void unsupported(Node p) {
	error("vig: unsupported operator %s\n", opname(p->op));
}

/* The intrinsics.
 *
 * VIG has no linker, so a runtime routine written in VIGasm cannot be brought
 * into a program that calls it.  Therefore the few operations that a C program
 * cannot express -- the ones that are VM instructions and nothing else -- are
 * emitted at the call instead of being called.  A program declares them like
 * any other function, in <vig.h>.
 *
 * Each replacement leaves the stack as a call to a void function would: the
 * argument is gone and nothing takes its place.  `print` and `print_string`
 * leave what they printed, so each of those needs a `pop`.
 */
static const struct {
	char *name;
	char *code;
} intrinsics[] = {
	{ "__vig_print",        "print\npop\n"        },
	{ "__vig_print_hex",    "print_hex\npop\n"    },
	{ "__vig_print_string", "print_string\npop\n" },
	/* Raw byte output, which is what a formatted-output routine written in C
	 * needs: it writes one byte and adds nothing of its own. */
	{ "__vig_write",        "write_byte\n"        },
	/* `halt' stops the VM, so what it leaves on the stack cannot matter and
	 * the instructions after it are unreachable.  The argument is dropped all
	 * the same, to keep the stack check honest up to that point. */
	{ "__vig_halt",         "pop\nhalt\n"         },
};

/* The instructions that a call to `p` becomes, or null if `p` is an ordinary
 * function. */
static char *intrinsic(Symbol p) {
	int i;

	if (p == NULL || p->name == NULL)
		return NULL;
	for (i = 0; i < NELEMS(intrinsics); i++)
		if (strcmp(p->name, intrinsics[i].name) == 0)
			return intrinsics[i].code;
	return NULL;
}

static void reject_float(void) {
	error("vig: floating-point operations are not supported\n");
}

static void I(segment)(int n) {
	/* VIGasm determines a label's region from the directive that follows it. */
	(void)n;
}

static void I(address)(Symbol q, Symbol p, long n) {
	if (n == 0)
		q->x.name = p->x.name;
	else
		q->x.name = stringf("%s%s%D", p->x.name, n > 0 ? "+" : "", n);
	/* ADDRL nodes use the numeric frame offset rather than x.name.  Preserve
	 * the displacement here so packed struct fields and array elements do not
	 * collapse back onto their base local. */
	q->x.offset = p->x.offset + n;
}

static void I(defaddress)(Symbol p) {
	print("i32 %s\n", p->x.name);
}

static void I(defconst)(int suffix, int size, Value v) {
	if (suffix == F) {
		reject_float();
		return;
	}
	if (size == 1)
		print("i8 %d\n", (int)v.i);
	else if (size == 2)
		print("i16 %d\n", (int)v.i);
	else if (size == 4)
		print("i32 %d\n", (int)v.i);
	else
		error("vig: data item of %d bytes is not supported\n", size);
}

static void I(defstring)(int len, char *str) {
	int i;

	for (i = 0; i < len; i++)
		print("i8 %d\n", str[i] & 0377);
}

static void I(defsymbol)(Symbol p) {
	if (p->scope == CONSTANTS)
		switch (optype(ttob(p->type))) {
		case I: p->x.name = stringf("%D", p->u.c.v.i); break;
		case U: p->x.name = stringf("%U", p->u.c.v.u); break;
		case P: p->x.name = stringf("%U", p->u.c.v.p); break;
		default: assert(0);
		}
	else if (p->scope >= LOCAL && p->sclass == STATIC)
		p->x.name = stringf("$%d", genlabel(1));
	else if (p->scope == LABELS || p->generated)
		p->x.name = stringf("$%s", p->name);
	else
		p->x.name = p->name;
}

static void emit_load(int size, int unsigned_load) {
	switch (size) {
	case 1: print(unsigned_load ? "load8_u\n" : "load8_s\n"); return;
	case 2: print(unsigned_load ? "load16_u\n" : "load16_s\n"); return;
	case 4: print("load32\n"); return;
	}
	error("vig: load of %d bytes is not supported\n", size);
}

static void emit_store(int size) {
	switch (size) {
	case 1: print("store8\n"); return;
	case 2: print("store16\n"); return;
	case 4: print("store32\n"); return;
	}
	error("vig: store of %d bytes is not supported\n", size);
}

/* The VIG foreign ABI accepts 32-bit signed and unsigned integers and guest
 * pointers.  A C pointer is deliberately always `ptr': it may be an output
 * pointer or a string in a frame, and the C declaration alone cannot safely
 * promise the stricter `cstr' lifetime rule. */
static char *foreign_argument_type(Type ty) {
	ty = unqual(ty);
	if (isptr(ty)) {
		if (isfunc(ty->type)) {
			error("vig: foreign callbacks are not supported\n");
			return NULL;
		}
		return "ptr";
	}
	if (isenum(ty) || isint(ty))
		return isunsigned(ty) ? "u32" : "i32";
	error("vig: foreign argument type %t is not supported\n", ty);
	return NULL;
}

static int foreign_return_type(Type ty) {
	ty = unqual(ty);
	if (ty == voidtype)
		return 1;
	if ((isenum(ty) || isint(ty)) && ty->size == 4)
		return 1;
	error("vig: foreign result type %t is not supported (use void or a 32-bit integer)\n", ty);
	return 0;
}

static int emit_foreign_extern(Symbol p, ForeignImport *entry) {
	Type *proto;
	int i;

	if (!isfunc(p->type)) {
		error("vig: foreign import %s is not a function\n", p->name);
		return 0;
	}
	if (p->type->u.f.oldstyle || p->type->u.f.proto == NULL) {
		error("vig: foreign import %s requires a prototype\n", p->name);
		return 0;
	}
	if (variadic(p->type)) {
		error("vig: variadic foreign import %s is not supported\n", p->name);
		return 0;
	}
	if (!foreign_return_type(freturn(p->type)))
		return 0;
	proto = p->type->u.f.proto;
	if (proto[0] == voidtype)
		proto++;
	for (i = 0; proto[i]; i++) {
		if (i == 4) {
			error("vig: foreign import %s has more than four arguments\n", p->name);
			return 0;
		}
		if (foreign_argument_type(proto[i]) == NULL)
			return 0;
	}
	print("extern %s %s %s", p->x.name, entry->library, entry->symbol);
	for (i = 0; proto[i]; i++)
		print(" %s", foreign_argument_type(proto[i]));
	print("\n");
	return 1;
}

/* The frame slot that an address node names, or -1 if it names something else.
 *
 * `load_local' and `store_local' reach a slot in one instruction where
 * `local_addr' and a separate load or store take two, and a slot that is not on
 * a four-byte boundary takes four.  Recognising the shape here rather than
 * folding the instructions afterwards keeps the decision where the information
 * is: a peephole over emitted text would have to know that nothing can jump
 * between the two instructions, and this cannot ask the question because it
 * never emits the first one.
 *
 * A parameter is always a whole slot.  A local is too, since `I(local)' aligns
 * them -- but an address *derived* from one is not: `&s.field' of a packed
 * structure lands part way through a slot, and that has to fall back.
 */
static int frame_slot(Node p) {
	Symbol s;

	if (p == NULL || (s = p->syms[0]) == NULL)
		return -1;
	if (specific(p->op) == ADDRF + P)
		return s->x.offset/4;
	if (specific(p->op) == ADDRL + P && s->x.offset%4 == 0)
		return parameter_slots + s->x.offset/4;
	return -1;
}

static void emit_local_address(Symbol p) {
	int off = p->x.offset;

	if (off%4 == 0)
		print("local_addr %d\n", parameter_slots + off/4);
	else {
		print("local_addr %d\n", parameter_slots);
		print("push %d\n", off);
		print("add\n");
	}
}

static void emit_local_offset(int off) {
	if (off%4 == 0)
		print("local_addr %d\n", parameter_slots + off/4);
	else {
		print("local_addr %d\n", parameter_slots);
		print("push %d\n", off);
		print("add\n");
	}
}

/* ASGNB's children are addresses.  Save them before copying so expressions
 * with side effects still run exactly once, then copy the aggregate bytewise.
 * VIG permits unaligned byte accesses and this also handles packed structs. */
static void emit_block_copy(Node p) {
	int i, size;
	Symbol slots;
	int dst, src;

	assert(p->kids[0] && p->kids[1] && p->syms[0]);
	slots = p->syms[2];
	assert(slots);
	dst = slots->x.offset;
	src = dst + 4;
	size = p->syms[0]->u.c.v.i;

	dumptree(p->kids[0]);
	emit_local_offset(dst);
	print("store32\n");
	/* The right child is INDIRB: assignment needs its address, not a block
	 * value (which has no scalar load instruction). */
	assert(generic(p->kids[1]->op) == INDIR);
	dumptree(p->kids[1]->kids[0]);
	emit_local_offset(src);
	print("store32\n");
	for (i = 0; i < size; i++) {
		emit_local_offset(src);
		print("load32\n");
		if (i)
			print("push %d\nadd\n", i);
		print("load8_u\n");
		emit_local_offset(dst);
		print("load32\n");
		if (i)
			print("push %d\nadd\n", i);
		print("store8\n");
	}
}

static void dumptree(Node p) {
	if (optype(p->op) == F) {
		reject_float();
		return;
	}

	switch (generic(p->op)) {
	case CNST:
		assert(!p->kids[0] && !p->kids[1]);
		assert(p->syms[0] && p->syms[0]->x.name);
		if (optype(p->op) == U)
			print("push %d\n", (int)strtoul(p->syms[0]->x.name, NULL, 0));
		else
			print("push %s\n", p->syms[0]->x.name);
		return;

	case ADDRG:
		assert(!p->kids[0] && !p->kids[1]);
		assert(p->syms[0] && p->syms[0]->x.name);
		print("push %s\n", p->syms[0]->x.name);
		return;

	case ADDRF:
		assert(!p->kids[0] && !p->kids[1]);
		assert(p->syms[0]);
		print("local_addr %d\n", p->syms[0]->x.offset/4);
		return;

	case ADDRL:
		assert(!p->kids[0] && !p->kids[1]);
		assert(p->syms[0]);
		emit_local_address(p->syms[0]);
		return;

	case INDIR:
		assert(p->kids[0] && !p->kids[1]);
		/* Reading a whole slot of the frame is one instruction.  A narrower
		 * read is not: `load_local' is 32 bits and says nothing about a sign. */
		if (opsize(p->op) == 4) {
			int slot = frame_slot(p->kids[0]);

			if (slot >= 0) {
				print("load_local %d\n", slot);
				return;
			}
		}
		dumptree(p->kids[0]);
		emit_load(opsize(p->op), optype(p->op) == U);
		return;

	case ASGN:
		if (optype(p->op) == B) {
			emit_block_copy(p);
			return;
		}
		/* lcc orders assignment children as address, value. VIG storeX wants
		 * value, address, so their emission order is intentionally reversed. */
		assert(p->kids[0] && p->kids[1]);
		dumptree(p->kids[1]);
		/* `store_local' takes the value alone, so the address is never
		 * computed.  The value has to be on the stack first either way. */
		if (opsize(p->op) == 4) {
			int slot = frame_slot(p->kids[0]);

			if (slot >= 0) {
				print("store_local %d\n", slot);
				return;
			}
		}
		dumptree(p->kids[0]);
		emit_store(opsize(p->op));
		return;

	case ARG:
		assert(p->kids[0] && !p->kids[1]);
		/* The value already occupies its ABI argument slot on the operand stack. */
		dumptree(p->kids[0]);
		return;

	case CALL:
		assert(p->kids[0] && !p->kids[1]);
		if (specific(p->kids[0]->op) == ADDRG+P) {
			char *code;
			ForeignImport *entry;

			assert(p->kids[0]->syms[0] && p->kids[0]->syms[0]->x.name);
			code = intrinsic(p->kids[0]->syms[0]);
			entry = foreign_import(p->kids[0]->syms[0]->name);
			/* An intrinsic is its instructions, so there is no call and no
			 * result to discard afterwards. */
			if (code != NULL) {
				print("%s", code);
				return;
			}
			if (entry != NULL) {
				print("foreign_call %s\n", p->kids[0]->syms[0]->x.name);
				/* The foreign ABI always produces its 32-bit result.  A C void
				 * call must discard that result before the next statement. */
				if (optype(p->op) == V)
					print("pop\n");
			} else
				print("call %s\n", p->kids[0]->syms[0]->x.name);
		} else {
			dumptree(p->kids[0]);
			print("call_indirect\n");
		}
		/* lcc represents a lowered struct call as CALLV because the result is
		 * already in the caller-supplied destination.  The VIG ABI still has
		 * the callee return that destination pointer, so discard it here. */
		if (optype(p->op) == V && p->syms[0]
		&& isstruct(freturn(p->syms[0]->type)))
			print("pop\n");
		return;

	case RET:
		if (optype(p->op) == V) {
			assert(!p->kids[0] && !p->kids[1]);
			print("ret\n");
		} else {
			assert(p->kids[0] && !p->kids[1]);
			dumptree(p->kids[0]);
			print("ret_val\n");
		}
		return;

	case LABEL:
		assert(!p->kids[0] && !p->kids[1]);
		assert(p->syms[0] && p->syms[0]->x.name);
		print("%s:\n", p->syms[0]->x.name);
		return;

	case JUMP:
		assert(p->kids[0] && !p->kids[1]);
		/* A jump to a label the instruction can name is a direct one.  Anything
		 * else is a computed target -- a switch reaching its arm through a jump
		 * table -- so the address is worked out and jumped to. */
		if (specific(p->kids[0]->op) == ADDRG+P) {
			print("jmp %s\n", p->kids[0]->syms[0]->x.name);
			return;
		}
		dumptree(p->kids[0]);
		print("jmp_indirect\n");
		return;

	case NEG:
		assert(p->kids[0] && !p->kids[1]);
		print("push 0\n");
		dumptree(p->kids[0]);
		print("sub\n");
		return;

	case CVI:
		assert(p->kids[0] && !p->kids[1]);
		dumptree(p->kids[0]);
		if (opsize(p->op) == 1) {
			print("push 24\nshl\npush 24\nshr_s\n");
		} else if (opsize(p->op) == 2) {
			print("push 16\nshl\npush 16\nshr_s\n");
		}
		return;

	case CVU:
		assert(p->kids[0] && !p->kids[1]);
		dumptree(p->kids[0]);
		/* A narrowing conversion has to drop the high bits, exactly as the
		 * signed CVI above has to extend the sign.  Storing the result would
		 * truncate it, but a value that is returned or used in an expression
		 * never passes through a store.  Widening needs nothing: an unsigned
		 * value already has its high bits clear. */
		if (opsize(p->op) == 1)
			print("push 255\nand\n");
		else if (opsize(p->op) == 2)
			print("push 65535\nand\n");
		return;

	case CVP:
		assert(p->kids[0] && !p->kids[1]);
		dumptree(p->kids[0]);
		return;

	case ADD: case SUB: case MUL: case DIV: case MOD:
	case LSH: case RSH: case BAND: case BOR: case BXOR:
		assert(p->kids[0] && p->kids[1]);
		dumptree(p->kids[0]);
		dumptree(p->kids[1]);
		switch (generic(p->op)) {
		case ADD:  print(optype(p->op) == U ? "add_wrap\n" : "add\n"); return;
		case SUB:  print(optype(p->op) == U ? "sub_wrap\n" : "sub\n"); return;
		case MUL:  print(optype(p->op) == U ? "mul_wrap\n" : "mul\n"); return;
		case DIV:  print(optype(p->op) == U ? "div_u\n" : "div\n"); return;
		case MOD:  print(optype(p->op) == U ? "mod_u\n" : "mod\n"); return;
		case LSH:  print("shl\n"); return;
		case RSH:  print(optype(p->op) == U ? "shr_u\n" : "shr_s\n"); return;
		case BAND: print("and\n"); return;
		case BOR:  print("or\n"); return;
		case BXOR: print("xor\n"); return;
		}
		assert(0);

	case BCOM:
		assert(p->kids[0] && !p->kids[1]);
		dumptree(p->kids[0]);
		print("not\n");
		return;

	case EQ: case NE: case GE: case GT: case LE: case LT:
		assert(p->kids[0] && p->kids[1]);
		assert(p->syms[0] && p->syms[0]->x.name);
		dumptree(p->kids[0]);
		dumptree(p->kids[1]);
		switch (generic(p->op)) {
		case EQ: print("eq\n"); break;
		case NE: print("ne\n"); break;
		case GE: print(optype(p->op) == U ? "gte_u\n" : "gte\n"); break;
		case GT: print(optype(p->op) == U ? "gt_u\n" : "gt\n"); break;
		case LE: print(optype(p->op) == U ? "lte_u\n" : "lte\n"); break;
		case LT: print(optype(p->op) == U ? "lt_u\n" : "lt\n"); break;
		}
		print("jmp_not_zero %s\n", p->syms[0]->x.name);
		return;
	}

	unsupported(p);
}

static void I(emit)(Node p) {
	for (; p; p = p->link) {
		dumptree(p);
		/* A call at the root of the forest is a call whose result nothing
		 * wants: `printf(...);' as a statement.  One that is wanted appears as
		 * a child of whatever wants it, so it is never seen here.  The value
		 * is on the operand stack either way, and an unwanted one has to come
		 * off before the next statement runs.
		 *
		 * A void call leaves nothing, and a struct-returning call is discarded
		 * where it is emitted, because its result reached the caller through
		 * the hidden pointer instead. */
		if (generic(p->op) == CALL && optype(p->op) != V)
			print("pop\n");
	}
}

static void I(export)(Symbol p) {
	(void)p;
}

static void I(function)(Symbol f, Symbol caller[], Symbol callee[], int ncalls) {
	int i;

	(void)ncalls;
	(*IR->segment)(CODE);
	for (i = 0; caller[i] && callee[i]; i++) {
		caller[i]->x.name = callee[i]->x.name = stringf("%d", 4*i);
		caller[i]->x.offset = callee[i]->x.offset = 4*i;
	}
	parameter_slots = i;
	function_returns_value = f->type->type != voidtype;
	struct_return_pointer = isstruct(f->type->type) ? caller[0] : NULL;
	maxargoffset = maxoffset = argoffset = offset = 0;
	gencode(caller, callee);
	print("%s:\n", f->x.name);
	print("enter %d %d\n", parameter_slots, (maxoffset + 3)/4);
	emitcode();
	/* lcc leaves a label after an explicit return. It can be reached when a C
	 * function falls off its end, and VIG requires every path to terminate. */
	if (function_returns_value) {
		if (struct_return_pointer) {
			emit_local_address(struct_return_pointer);
			print("load32\nret_val\n");
		} else
			print("push 0\nret_val\n");
	}
	else
		print("ret\n");
}

static void prepare_block_copies(Node p) {
	for (; p; p = p->link) {
		if (specific(p->op) == ASGN+B && p->syms[2] == NULL) {
			Symbol dst = temporary(AUTO, voidptype);
			Symbol src = temporary(AUTO, voidptype);
			(*IR->local)(dst);
			(*IR->local)(src);
			dst->defined = src->defined = 1;
			/* syms[0] and syms[1] hold ASGNB's size and alignment.  The
			 * third slot is available for the first of our two adjacent
			 * pointer spill slots. */
			p->syms[2] = dst;
		}
		prepare_block_copies(p->kids[0]);
		prepare_block_copies(p->kids[1]);
	}
}

static Node I(gen)(Node p) {
	prepare_block_copies(p);
	return p;
}

static void I(global)(Symbol p) {
	if (isfloat(p->type))
		reject_float();
	print("%s:\n", p->x.name);
}

static void I(import)(Symbol p) {
	ForeignImport *entry = foreign_import(p->name);

	/* An intrinsic is declared like an external function and defined nowhere,
	 * because its definition is the instruction that replaces the call. */
	if (intrinsic(p) != NULL)
		return;
	if (entry != NULL) {
		(void)emit_foreign_extern(p, entry);
		return;
	}
	error("vig: external symbol %s requires a VIG runtime declaration\n", p->name);
}

static void I(local)(Symbol p) {
	/* Each local starts on a slot boundary.
	 *
	 * VIG permits an unaligned access, so this is not about what the machine
	 * requires -- it is about what `load_local' and `store_local' can reach.
	 * Those index four-byte slots, so a local at a byte offset that is not a
	 * multiple of four can only be read through `local_addr', a `push' and an
	 * `add'.  One `char' local would otherwise push every local after it off a
	 * slot boundary and make all of them cost three instructions instead of one.
	 *
	 * The padding this adds is frame space, and a frame has a megabyte to grow
	 * down into.  It is not an ABI change either: the packed layout that ABI.md
	 * promises is about the fields of a structure, which fixes `sizeof' and
	 * `offsetof'.  Where a function puts its own locals is its own business.
	 */
	offset = roundup(offset, 4);
	p->x.name = stringf("%d", offset);
	p->x.offset = offset;
	offset += p->type->size;
}

static void I(progbeg)(int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	print("entry _start\n_start:\ncall main\npop\nhalt\n");
}

static void I(progend)(void) {}

static void I(space)(int n) {
	print("reserve %d\n", n);
}

#define vig_blockbeg blockbeg
#define vig_blockend blockend

Interface vigIR = {
	1, 1, 0, /* char */
	2, 2, 0, /* short */
	4, 4, 0, /* int */
	4, 4, 0, /* long */
	4, 4, 0, /* long long: the 32-bit VIG representation */
	4, 4, 1, /* float: parsed for a useful backend rejection */
	8, 8, 1, /* double */
	8, 8, 1, /* long double */
	4, 4, 0, /* T* */
	0, 1, 0, /* struct: no padding in the VIG ABI */
	1,        /* little_endian */
	0,        /* mulops_calls */
	0,        /* wants_callb */
	0,        /* wants_argb */
	1,        /* left_to_right */
	0,        /* wants_dag */
	0,        /* unsigned_char */
	I(address),
	I(blockbeg),
	I(blockend),
	I(defaddress),
	I(defconst),
	I(defstring),
	I(defsymbol),
	I(emit),
	I(export),
	I(function),
	I(gen),
	I(global),
	I(import),
	I(local),
	I(progbeg),
	I(progend),
	I(segment),
	I(space),
	0, /* I(stabblock) */
	0, /* I(stabend) */
	0, /* I(stabfend) */
	0, /* I(stabinit) */
	0, /* I(stabline) */
	0, /* I(stabsym) */
	0, /* I(stabtype) */
	{ 0 }, /* Xinterface */
	I(foreign_import),
};
