#ifndef VIG_STDIO_H
#define VIG_STDIO_H

/* Formatted output.
 *
 * There is no file system and no `FILE': the VM has one output stream, and
 * these write to it.  `printf' and its relatives are in `runtime/stdio.c'.
 *
 * The conversions are `d', `i', `u', `x', `X', `c', `s', `f', `F', `e', `E',
 * `g', `G' and `%%', each with an optional minimum field width, an optional
 * precision, and the `-' and `0' flags.  There is no length modifier: every
 * integer in this C subset is 32 bits, so `%%ld' and `%%d' would mean the same
 * thing, and every floating type is binary32, so `%%f' takes them all.
 *
 * Nothing here hands a pointer to the VM, so the rule that a `cstr' argument
 * must live in the program image does not apply.  `printf("%%s", buffer)' works
 * for a buffer in a frame, which `__vig_print_string(buffer)' would refuse.
 */

#include <stdarg.h>
#include <stddef.h>

/* End of input, and the value the `ctype' functions accept alongside a
 * character.  `read_byte' reports the end of the input stream as -1, which is
 * the same number. */
#define EOF (-1)

int putchar(int c);
int puts(const char *text);

int printf(const char *format, ...);
int vprintf(const char *format, va_list arguments);
int sprintf(char *out, const char *format, ...);
int vsprintf(char *out, const char *format, va_list arguments);
int snprintf(char *out, size_t limit, const char *format, ...);
int vsnprintf(char *out, size_t limit, const char *format, va_list arguments);

#endif
