#ifndef VIG_STDLIB_H
#define VIG_STDLIB_H

/* Allocation and the odds and ends of <stdlib.h>, written in C.
 *
 * The heap is an array in the zero-filled region, because that is the only
 * place a program can put one.  VIG has no `sbrk' and no instruction that
 * reports the frame pointer, so the space above the program image cannot be
 * used: nothing says how far down the frames have grown.  An array in `.bss'
 * costs no bytes in the program file and sits below the image end, where a
 * frame can never reach it.  See ABI.md.
 *
 * Define VIG_HEAP_SIZE before including this file to change how much.  The heap
 * has to fit in the memory the VM was given, which is 1 MiB unless `vig' was
 * run with --memory.
 *
 * The allocator is the one from K&R: a circular free list, first fit, and
 * coalescing with either neighbour on free.  It is small enough to read, and a
 * block carries only its size and a link.
 */

#include <stddef.h>
#include <string.h>
#include <vig.h>

#ifndef VIG_HEAP_SIZE
#define VIG_HEAP_SIZE 65536
#endif

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
	if (bytes > VIG_HEAP_SIZE)
		return NULL;
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
		if (p == vig_freep)	/* all the way round, and nothing fits */
			return NULL;
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

	if (count != 0 && size > VIG_HEAP_SIZE / count)
		return NULL;	/* the product would not fit the heap anyway */
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

#endif
