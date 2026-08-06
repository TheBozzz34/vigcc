#ifndef VIG_STDDEF_H
#define VIG_STDDEF_H

/* The types that the rest of the headers share.
 *
 * VIG is a 32-bit machine and its C subset has no type wider than four bytes,
 * so a size and a pointer difference are an unsigned int and an int.  See
 * ABI.md.
 */

typedef unsigned int size_t;
typedef int ptrdiff_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif
