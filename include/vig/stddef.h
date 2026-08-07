#ifndef VIG_STDDEF_H
#define VIG_STDDEF_H

/* The types that the rest of the headers share.
 *
 * VIG64 is LP64: a pointer and a `long' are eight bytes and an `int' is four.
 * Therefore a size is an `unsigned long' and a pointer difference is a `long',
 * and neither one is an `int'.  See VIG64.md.
 */

typedef unsigned long size_t;
typedef long ptrdiff_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif
