/* Allocation and the odds and ends of <stdlib.h>, written in C.
 *
 * The heap is an array in the zero-filled region of this object, because that
 * is the only place a program can put one.  See <stdlib.h> for why, and for
 * what VIG_HEAP_SIZE means.
 *
 * The allocator is the one from K&R: a circular free list, first fit, and
 * coalescing with either neighbour on free.  It is small enough to read, and a
 * block carries only its size and a link.
 *
 * A failed allocation sets `errno' to ENOMEM.  C does not ask for that; POSIX
 * does, and a caller that has just been handed a null pointer has no other way
 * to learn whether the heap was full or the request was absurd.
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <vig.h>

typedef struct vig_block {
	struct vig_block *next;	/* the next free block, by address, wrapping */
	size_t units;		/* the size of this block, counted in blocks */
} vig_block;

static char vig_heap[VIG_HEAP_SIZE];
static vig_block vig_base;	/* the list head, which owns no storage */
static vig_block *vig_freep;	/* where the last search stopped */

/* The whole heap starts as one free block.  There is no `morecore': the arena
 * is a fixed array, so a request that does not fit fails rather than growing. */
static void vig_heap_start(void) {
	vig_block *whole = (vig_block *)vig_heap;

	whole->units = VIG_HEAP_SIZE / sizeof(vig_block);
	whole->next = &vig_base;
	vig_base.next = whole;
	vig_base.units = 0;
	vig_freep = &vig_base;
}

void *malloc(size_t bytes) {
	vig_block *p, *previous;
	size_t units;

	/* A request that could not fit whatever the heap held is refused here, so
	 * that rounding it up to whole blocks cannot overflow.  Zero is not
	 * refused: C leaves that case to the implementation, and handing back a
	 * pointer that owns no bytes is what the usual C libraries do. */
	if (bytes > VIG_HEAP_SIZE) {
		errno = ENOMEM;
		return NULL;
	}
	units = (bytes + sizeof(vig_block) - 1) / sizeof(vig_block) + 1;

	if (vig_freep == NULL)
		vig_heap_start();
	previous = vig_freep;
	for (p = previous->next; ; previous = p, p = p->next) {
		if (p->units >= units) {
			if (p->units == units)
				previous->next = p->next;
			else {
				/* Split the block and hand back the tail, which leaves the
				 * free list pointing at a block that is still linked. */
				p->units -= units;
				p += p->units;
				p->units = units;
			}
			vig_freep = previous;
			return (void *)(p + 1);
		}
		if (p == vig_freep) {	/* all the way round, and nothing fits */
			/* C does not require this, but a caller that has just been
			 * refused memory has no other way to learn why. */
			errno = ENOMEM;
			return NULL;
		}
	}
}

void free(void *address) {
	vig_block *block, *p;

	if (address == NULL)
		return;
	block = (vig_block *)address - 1;

	/* Find the pair of free blocks this one belongs between.  The list is kept
	 * in address order so that a neighbour can be recognised, and it wraps, so
	 * the block can also fall before the first or after the last. */
	for (p = vig_freep; !(block > p && block < p->next); p = p->next)
		if (p >= p->next && (block > p || block < p->next))
			break;

	if (block + block->units == p->next) {
		block->units += p->next->units;
		block->next = p->next->next;
	} else
		block->next = p->next;

	if (p + p->units == block) {
		p->units += block->units;
		p->next = block->next;
	} else
		p->next = block;

	vig_freep = p;
}

void *calloc(size_t count, size_t size) {
	size_t bytes;
	void *address;

	if (count != 0 && size > VIG_HEAP_SIZE / count) {
		errno = ENOMEM;	/* the product would not fit the heap anyway */
		return NULL;
	}
	bytes = count * size;
	address = malloc(bytes);
	if (address != NULL)
		memset(address, 0, bytes);
	return address;
}

void *realloc(void *address, size_t bytes) {
	vig_block *block;
	size_t held;
	void *moved;

	if (address == NULL)
		return malloc(bytes);
	if (bytes == 0) {
		free(address);
		return NULL;
	}
	/* The block records its own size, less the header it starts with. */
	block = (vig_block *)address - 1;
	held = (block->units - 1) * sizeof(vig_block);
	if (bytes <= held)
		return address;

	moved = malloc(bytes);
	if (moved == NULL)
		return NULL;	/* the old block is still the caller's */
	memcpy(moved, address, held);
	free(address);
	return moved;
}

/* `exit' is the `halt' instruction.  VIG keeps no exit status, so the value is
 * discarded: a program says what it did by what it printed. */
void exit(int status) {
	__vig_halt(status);
}

void abort(void) {
	__vig_halt(1);
}

static void vig_swap(char *left, char *right, size_t size) {
	size_t i;
	char held;

	for (i = 0; i < size; i++) {
		held = left[i];
		left[i] = right[i];
		right[i] = held;
	}
}

/* A shell sort, with Knuth's gaps.
 *
 * The name says quicksort but the standard asks only for a sort, and this one
 * suits the machine better: it needs no recursion at all.  A quicksort recurses
 * to a depth that the input decides, and VIG has a fixed call stack -- 256
 * frames by default -- with frame storage that grows down towards the program
 * image.  A sort that cannot run either of them out is worth an insertion pass.
 *
 * `compare' is reached only through the pointer the caller gave, so every call
 * to it is a `call_indirect' and the function is verified the first time one
 * arrives there.
 */
void qsort(void *base, size_t count, size_t size,
	int (*compare)(const void *, const void *)) {
	char *items = (char *)base;
	size_t gap, i, j;

	if (count < 2 || size == 0)
		return;

	gap = 1;
	while (gap < count / 3)
		gap = gap * 3 + 1;

	for (;;) {
		for (i = gap; i < count; i++)
			/* `j' is unsigned, so the test stops the loop before it could
			 * subtract past zero. */
			for (j = i; j >= gap
			&& compare(items + (j - gap)*size, items + j*size) > 0; j -= gap)
				vig_swap(items + (j - gap)*size, items + j*size, size);
		if (gap == 1)
			return;
		gap = gap / 3;
	}
}

void *bsearch(const void *key, const void *base, size_t count, size_t size,
	int (*compare)(const void *, const void *)) {
	const char *items = (const char *)base;
	size_t low = 0, high = count, middle;
	int order;

	while (low < high) {
		/* Written as a difference so the midpoint cannot overflow, which it
		 * could for a count near the top of a 32-bit size. */
		middle = low + (high - low)/2;
		order = compare(key, items + middle*size);
		if (order == 0)
			return (void *)(items + middle*size);
		if (order < 0)
			high = middle;
		else
			low = middle + 1;
	}
	return NULL;
}

int abs(int value) {
	return value < 0 ? -value : value;
}

int atoi(const char *text) {
	int value = 0;
	int negative = 0;

	while (*text == ' ' || *text == '\t' || *text == '\n')
		text++;
	if (*text == '-' || *text == '+') {
		negative = *text == '-';
		text++;
	}
	while (*text >= '0' && *text <= '9') {
		value = value * 10 + (*text - '0');
		text++;
	}
	return negative ? -value : value;
}

/* Read a decimal floating-point number, and report where it stopped.
 *
 * The digits are accumulated as an integer and scaled once at the end, so the
 * result rounds once rather than once per digit.  Nine digits is what this
 * reads: more than a binary32 can hold and fewer than a binary64 can, so a
 * longer literal keeps its scale and loses its tail. */
double strtod(const char *text, char **end) {
	const char *start = text;
	unsigned mantissa = 0;
	int digits = 0, exponent = 0, negative = 0, seen = 0;
	double value;

	while (*text == ' ' || *text == 9 || *text == 10 || *text == 13)
		text++;
	if (*text == '-' || *text == '+') {
		negative = *text == '-';
		text++;
	}
	while (*text >= '0' && *text <= '9') {
		seen = 1;
		if (digits < 9) {
			mantissa = mantissa * 10u + (unsigned)(*text - '0');
			digits++;
		} else
			exponent++;	/* past the precision: only the scale still matters */
		text++;
	}
	if (*text == '.') {
		text++;
		while (*text >= '0' && *text <= '9') {
			seen = 1;
			if (digits < 9) {
				mantissa = mantissa * 10u + (unsigned)(*text - '0');
				digits++;
				exponent--;
			}
			text++;
		}
	}
	if (!seen) {
		if (end != 0)
			*end = (char *)start;	/* nothing was a number */
		return 0.0;
	}
	if (*text == 'e' || *text == 'E') {
		const char *mark = text;
		int power = 0, negative_power = 0, any = 0;

		text++;
		if (*text == '-' || *text == '+') {
			negative_power = *text == '-';
			text++;
		}
		while (*text >= '0' && *text <= '9') {
			any = 1;
			if (power < 1000)
				power = power * 10 + (*text - '0');
			text++;
		}
		if (any)
			exponent = exponent + (negative_power ? -power : power);
		else
			text = mark;	/* an `e' with no digits is not part of the number */
	}

	value = (double)mantissa;
	while (exponent > 0) {
		value = value * 10.0;
		exponent--;
	}
	while (exponent < 0) {
		value = value / 10.0;
		exponent++;
	}
	if (end != 0)
		*end = (char *)text;
	return negative ? -value : value;
}

double atof(const char *text) {
	return strtod(text, 0);
}
