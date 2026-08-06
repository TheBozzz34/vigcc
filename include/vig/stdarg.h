#ifndef VIG_STDARG_H
#define VIG_STDARG_H

/* The VIG ABI gives every variadic callee two hidden parameters after its
 * fixed ones: __vig_va_count and __vig_va_args.  The latter points to a
 * caller-owned array of four-byte, default-promoted argument slots. */
typedef char *va_list;

#define va_start(list, last) ((void)((list) = __vig_va_args))
#define va_arg(list, type) (*(type *)((list += 4) - 4))
#define va_end(list) ((void)0)
#define va_count() (__vig_va_count)

#endif
