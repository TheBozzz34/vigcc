#ifndef VIG_STDLIB_H
#define VIG_STDLIB_H

/* Allocation and the odds and ends of <stdlib.h>.
 *
 * The implementation is in `runtime/stdlib.c'.  The heap is an array in the
 * zero-filled region of that object, because that is the only place a program
 * can put one: VIG has no `sbrk' and no instruction that reports the frame
 * pointer, so the space above the program image cannot be used -- nothing says
 * how far down the frames have grown.  An array in `.bss' costs no bytes in the
 * program file and sits below the image end, where a frame can never reach it.
 * See ABI.md.
 *
 * VIG_HEAP_SIZE is how large it is.  It belongs to the runtime rather than to
 * any one program, so defining it before including this file changes what this
 * program *believes* and not what it gets; to change the heap, define it for
 * the whole build so that `runtime/stdlib.c' is compiled with it too.  The heap
 * has to fit in the memory the VM was given, which is 1 MiB unless `vig' was
 * run with --memory.
 */

#include <stddef.h>

#ifndef VIG_HEAP_SIZE
#define VIG_HEAP_SIZE 65536
#endif

void *malloc(size_t bytes);
void free(void *address);
void *calloc(size_t count, size_t size);
void *realloc(void *address, size_t bytes);

void exit(int status);
void abort(void);

void qsort(void *base, size_t count, size_t size,
    int (*compare)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t count, size_t size,
    int (*compare)(const void *, const void *));

int abs(int value);
int atoi(const char *text);
double strtod(const char *text, char **end);
double atof(const char *text);

#endif
