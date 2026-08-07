#ifndef VIG_STDARG_H
#define VIG_STDARG_H

#include <stddef.h>

/* The VIG ABI gives every variadic callee two hidden parameters after its
 * fixed ones: __vig_va_count and __vig_va_args.  The latter points to a
 * caller-owned array of default-promoted argument slots, each one the width of
 * a pointer: four bytes under VIG32 and eight under VIG64.  A promoted `int',
 * a `double' and a pointer each occupy exactly one.  See VIG64.md. */
typedef char *va_list;

#define va_start(list, last) ((void)((list) = __vig_va_args))
#define va_arg(list, type) \
	(*(type *)((list += sizeof (void *)) - sizeof (void *)))
#define va_end(list) ((void)0)
#define va_count() (__vig_va_count)

#endif
