#ifndef VIG_STDIO_H
#define VIG_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)

int putchar(int c);
int puts(const char *text);
int vprintf(const char *format, va_list arguments);
int printf(const char *format, ...);
int vsprintf(char *out, const char *format, va_list arguments);
int sprintf(char *out, const char *format, ...);
int vsnprintf(char *out, size_t limit, const char *format, va_list arguments);
int snprintf(char *out, size_t limit, const char *format, ...);

#endif
