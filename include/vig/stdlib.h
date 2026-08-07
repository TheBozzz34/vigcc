#ifndef VIG_STDLIB_H
#define VIG_STDLIB_H

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

#endif
