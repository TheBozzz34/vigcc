#ifndef VIG_STDIO_H
#define VIG_STDIO_H

/* Formatted output, written in C.
 *
 * VIG has no linker, so a library cannot be linked into a program that calls
 * it.  These are defined here instead, and a program gets them by including
 * this file.  Only `__vig_write' reaches the VM: it writes one byte and adds
 * nothing of its own, which is what a formatting routine needs.
 *
 * Nothing here hands a pointer to the VM, so the rule that a `cstr' argument
 * must live in the program image does not apply.  `printf("%s", buffer)' works
 * for a buffer in a frame, which `__vig_print_string(buffer)' would refuse.
 *
 * The conversions are `d', `i', `u', `x', `X', `c', `s' and `%%', each with an
 * optional minimum field width and the `-' and `0' flags.  There is no
 * precision, and no length modifier: every integer in this C subset is 32 bits,
 * so `%%ld' and `%%d' would mean the same thing.
 */

#include <stdarg.h>
#include <vig.h>

#define VIG_DIGITS_MAX 16

static char vig_digit_table[] = "0123456789abcdef";

static int vig_strlen(const char *text) {
	int length = 0;

	while (*text != '\0') {
		length++;
		text++;
	}
	return length;
}

static void vig_pad(int count, int fill) {
	while (count > 0) {
		__vig_write(fill);
		count--;
	}
}

/* The digits of `value' in `base', least significant first.  Returning them
 * reversed costs the caller a backwards loop and saves a second buffer. */
static int vig_digits(unsigned value, unsigned base, int upper, char *out) {
	int length = 0;
	char digit;

	if (value == 0) {
		out[0] = '0';
		return 1;
	}
	while (value != 0) {
		digit = vig_digit_table[value % base];
		if (upper != 0 && digit >= 'a')
			digit = (char)(digit - 'a' + 'A');
		out[length] = digit;
		length++;
		value = value / base;
	}
	return length;
}

int putchar(int c) {
	__vig_write(c);
	return c;
}

int puts(const char *text) {
	int written = 0;

	while (*text != '\0') {
		__vig_write(*text);
		text++;
		written++;
	}
	__vig_write('\n');
	return written + 1;
}

int printf(const char *format, ...) {
	va_list arguments;
	char digits[VIG_DIGITS_MAX];
	const char *text;
	int written = 0;
	int left, fill, width, length, negative, i, item;
	unsigned value;
	char conversion;

	va_start(arguments, format);
	while (*format != '\0') {
		if (*format != '%') {
			__vig_write(*format);
			written++;
			format++;
		} else {
			format++;
			left = 0;
			fill = ' ';
			width = 0;
			negative = 0;
			length = 0;
			text = 0;

			while (*format == '-' || *format == '0') {
				if (*format == '-')
					left = 1;
				else
					fill = '0';
				format++;
			}
			while (*format >= '0' && *format <= '9') {
				width = width * 10 + (*format - '0');
				format++;
			}
			conversion = *format;
			if (conversion != '\0')
				format++;

			if (conversion == 'd' || conversion == 'i') {
				item = va_arg(arguments, int);
				value = (unsigned)item;
				if (item < 0) {
					negative = 1;
					/* Negating as unsigned wraps rather than trapping, which
					 * matters for the most negative value: it has no positive
					 * counterpart and `0 - item' would overflow. */
					value = 0u - value;
				}
				length = vig_digits(value, 10u, 0, digits);
			} else if (conversion == 'u') {
				value = (unsigned)va_arg(arguments, int);
				length = vig_digits(value, 10u, 0, digits);
			} else if (conversion == 'x' || conversion == 'X') {
				value = (unsigned)va_arg(arguments, int);
				length = vig_digits(value, 16u, conversion == 'X', digits);
			} else if (conversion == 'c') {
				digits[0] = (char)va_arg(arguments, int);
				length = 1;
			} else if (conversion == 's') {
				text = (const char *)va_arg(arguments, char *);
				length = vig_strlen(text);
			} else if (conversion == '%') {
				digits[0] = '%';
				length = 1;
			} else {
				/* An unknown conversion prints as it was written, so a mistake
				 * in a format string is visible rather than silent. */
				digits[0] = '%';
				digits[1] = conversion;
				length = 2;
				text = 0;
			}

			/* The sign sits inside the field, so it counts towards the width. */
			item = length + negative;
			if (left == 0 && fill == ' ')
				vig_pad(width - item, ' ');
			if (negative != 0)
				__vig_write('-');
			if (left == 0 && fill == '0')
				vig_pad(width - item, '0');

			if (text != 0) {
				for (i = 0; i < length; i++)
					__vig_write(text[i]);
			} else if (conversion == 'c' || conversion == '%') {
				__vig_write(digits[0]);
			} else if (conversion != 'd' && conversion != 'i'
			&& conversion != 'u' && conversion != 'x' && conversion != 'X') {
				for (i = 0; i < length; i++)
					__vig_write(digits[i]);
			} else {
				/* `vig_digits' leaves the number least significant first. */
				for (i = length - 1; i >= 0; i--)
					__vig_write(digits[i]);
			}

			if (left != 0)
				vig_pad(width - item, ' ');
			written = written + (item > width ? item : width);
		}
	}
	va_end(arguments);
	return written;
}

#endif
