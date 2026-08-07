/* Formatted output, written in C.
 *
 * Only `__vig_write' reaches the VM: it writes one byte and adds nothing of its
 * own, which is what a formatting routine needs.
 *
 * Nothing here hands a pointer to the VM, so the rule that a `cstr' argument
 * must live in the program image does not apply.  `printf("%s", buffer)' works
 * for a buffer in a frame, which `__vig_print_string(buffer)' would refuse.
 *
 * The conversions are `d', `i', `u', `x', `X', `c', `s', `f', `F', `e', `E',
 * `g', `G' and `%%', each with an optional minimum field width, an optional
 * precision, and the `-' and `0' flags.  There is no length modifier: every
 * integer in this C subset is 32 bits, so `%%ld' and `%%d' would mean the same
 * thing.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <vig.h>

#define VIG_DIGITS_MAX 64

static char vig_digit_table[] = "0123456789abcdef";

static int vig_strlen(const char *text) {
	int length = 0;

	while (*text != '\0') {
		length++;
		text++;
	}
	return length;
}

/* Where formatted output goes.  `out' is null when the destination is the VM
 * itself; otherwise it is a buffer of `limit' bytes including the terminator.
 * `written' counts what a big enough buffer would have taken, which is what
 * snprintf has to report. */
typedef struct vig_sink {
	char *out;
	size_t limit;
	size_t written;
} vig_sink;

static void vig_put(vig_sink *sink, int c) {
	if (sink->out == NULL)
		__vig_write(c);
	else if (sink->written + 1 < sink->limit)
		sink->out[sink->written] = (char)c;
	sink->written++;
}

static void vig_pad(vig_sink *sink, int count, int fill) {
	while (count > 0) {
		vig_put(sink, fill);
		count--;
	}
}

/* The digits of `value' in `base', least significant first.  Returning them
 * reversed costs the caller a backwards loop and saves a second buffer. */
static int vig_digits(unsigned long value, unsigned long base, int upper, char *out) {
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

/* Floating point, for the conversions below.
 *
 * These work in `double', which is binary64 under VIG64.  That is not a matter
 * of precision but of what arrives: C promotes a variadic `float' to `double',
 * so `printf("%f", x)' hands over eight bytes whatever the argument was
 * written as.  Reading four would read half of one.
 *
 * Nine digits is what gets printed.  A binary32 carries about 7.2 decimal
 * digits and a binary64 about 15.9, so nine is more than a `float' knows and
 * less than a `double' does.  That matters for a large number: a C library
 * prints the exact binary expansion of what it holds, which for 1e30 runs to
 * 31 nonzero-looking digits, while this prints the nine that a `float' means
 * and zeros after them.  Within the range where the whole part fits an
 * unsigned int -- which is every value a program is likely to print -- the two
 * agree.
 */

#define VIG_FLOAT_DIGITS 9
#define VIG_MAX_PRECISION 9	/* ten to the ninth still fits an unsigned int */

static int vig_isnan(double value) {
	return !(value == value);
}

static int vig_isinf(double value) {
	return value != 0.0 && value / 2.0 == value && !vig_isnan(value);
}

/* The digits of a positive finite value, most significant first, and the
 * exponent `e' such that the value is 0.D1D2... times ten to the e.
 *
 * Scaling by a table of powers rather than one factor of ten at a time keeps
 * the error down: reaching 1e30 by thirty divisions rounds thirty times. */
static int vig_significand(double value, int count, char *digits) {
	/* The table reaches 1e256 rather than 1e32 because a binary64 runs to
	 * about 1.8e308.  Stopping at 1e32 would leave the loop below dividing by
	 * ten hundreds of times, and each division rounds. */
	static const double up[9] = {
		1e1, 1e2, 1e4, 1e8, 1e16, 1e32, 1e64, 1e128, 1e256
	};
	static const int step[9] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };
	char whole_digits[12];
	unsigned whole;
	double fraction;
	int exponent = 0;
	int i = 0;
	int wcount;

	if (value == 0.0) {
		for (i = 0; i < count; i++)
			digits[i] = '0';
		return 0;
	}

	if (value < 4294967296.0) {
		/* The whole part is an exact integer, and so are its digits.  That
		 * matters for a tie: 1234.5 has to come out as 12345 and then zeros,
		 * and arriving at it by dividing by a thousand leaves a remainder that
		 * makes an exact half look like a little more than one. */
		whole = (unsigned)value;
		fraction = value - (double)whole;
		if (whole > 0u) {
			wcount = vig_digits(whole, 10u, 0, whole_digits);
			exponent = wcount;
			for (i = 0; i < wcount && i < count; i++)
				digits[i] = whole_digits[wcount - 1 - i];
		} else {
			while (fraction < 0.1) {
				fraction = fraction * 10.0;
				exponent--;
			}
		}
		while (i < count) {
			double scaled = fraction * 10.0;
			int digit = (int)scaled;

			if (digit > 9)
				digit = 9;
			if (digit < 0)
				digit = 0;
			digits[i] = (char)('0' + digit);
			fraction = scaled - (double)digit;
			i++;
		}
		return exponent;
	}

	/* Beyond that the whole part does not fit an unsigned int, so the value is
	 * scaled down by a table of powers -- which rounds far fewer times than one
	 * factor of ten at a time would.  A value this large has no exact decimal
	 * form in nine digits anyway. */
	for (i = 8; i >= 0; i--)
		while (value >= up[i]) {
			value = value / up[i];
			exponent += step[i];
		}
	while (value >= 1.0) {
		value = value / 10.0;
		exponent++;
	}
	while (value < 0.1) {
		value = value * 10.0;
		exponent--;
	}
	for (i = 0; i < count; i++) {
		double scaled = value * 10.0;
		int digit = (int)scaled;

		if (digit > 9)
			digit = 9;
		if (digit < 0)
			digit = 0;
		digits[i] = (char)('0' + digit);
		value = scaled - (double)digit;
	}
	return exponent;
}

/* Round a run of decimal digits at `keep' places, carrying into the exponent if
 * every digit was a nine. */
static int vig_round_digits(char *digits, int count, int keep, int *exponent) {
	int i;

	if (keep >= count || keep <= 0)
		return 0;
	if (digits[keep] < '5')
		return 0;
	if (digits[keep] == '5') {
		/* Exactly half.  C rounds by the current mode, which is to nearest
		 * with ties going to an even digit, so the digit being kept decides.
		 * Anything nonzero further along means it was never a tie. */
		int tie = 1;

		for (i = keep + 1; i < count; i++)
			if (digits[i] != '0') {
				tie = 0;
				break;
			}
		if (tie && ((digits[keep - 1] - '0') % 2) == 0)
			return 0;
	}
	for (i = keep - 1; i >= 0; i--) {
		if (digits[i] != '9') {
			digits[i]++;
			return 0;
		}
		digits[i] = '0';
	}
	/* Every digit carried: the number became one place wider. */
	digits[0] = '1';
	(*exponent)++;
	return 1;
}

/* `value' in the style of %f.  It is finite and not negative. */
static int vig_fixed(double value, int precision, char *out) {
	char digits[VIG_FLOAT_DIGITS + 2];
	unsigned whole, fraction, power;
	int length = 0, exponent, i, point;

	if (value < 4294967296.0) {
		/* The whole part fits an unsigned int, so it is exact and the
		 * fraction is scaled by ten to the precision and rounded once. */
		whole = (unsigned)value;
		power = 1;
		for (i = 0; i < precision; i++)
			power = power * 10u;
		{
			/* The same rule, on a scaled fraction rather than on digits: round
			 * to nearest, and on an exact half to the even value. */
			double scaled = (value - (double)whole) * (double)power;
			double rest;

			fraction = (unsigned)scaled;
			rest = scaled - (double)fraction;
			/* The digit that decides is the last one kept: the fraction when
			 * there is one, and otherwise the whole part, whose final digit is
			 * what a precision of zero rounds. */
			if (rest > 0.5
			|| (rest == 0.5 && ((precision > 0 ? fraction : whole) & 1u) != 0u))
				fraction = fraction + 1u;
		}
		if (fraction >= power) {
			fraction = fraction - power;
			whole = whole + 1u;
		}

		i = vig_digits(whole, 10u, 0, digits);
		while (i > 0) {
			i--;
			out[length] = digits[i];
			length++;
		}
		if (precision > 0) {
			out[length] = '.';
			length++;
			for (i = precision - 1; i >= 0; i--) {
				out[length + i] = (char)('0' + fraction % 10u);
				fraction = fraction / 10u;
			}
			length += precision;
		}
		return length;
	}

	/* Too large for that, so the digits come from the significand and the rest
	 * of the whole part is zeros.  Only about seven of them are meaningful. */
	exponent = vig_significand(value, VIG_FLOAT_DIGITS, digits);
	point = exponent;	/* digits before the decimal point */
	for (i = 0; i < point; i++) {
		out[length] = i < VIG_FLOAT_DIGITS ? digits[i] : '0';
		length++;
	}
	if (precision > 0) {
		out[length] = '.';
		length++;
		for (i = 0; i < precision; i++) {
			out[length] = '0';
			length++;
		}
	}
	return length;
}

/* `value' in the style of %e. */
static int vig_scientific(double value, int precision, int upper, char *out) {
	char digits[VIG_FLOAT_DIGITS + 2];
	int length = 0, exponent, i, power, keep;

	exponent = vig_significand(value, VIG_FLOAT_DIGITS, digits);
	keep = precision + 1;
	if (keep > VIG_FLOAT_DIGITS)
		keep = VIG_FLOAT_DIGITS;
	vig_round_digits(digits, VIG_FLOAT_DIGITS, keep, &exponent);

	out[length] = digits[0];
	length++;
	if (precision > 0) {
		out[length] = '.';
		length++;
		for (i = 1; i <= precision; i++) {
			out[length] = i < VIG_FLOAT_DIGITS ? digits[i] : '0';
			length++;
		}
	}
	out[length] = upper ? 'E' : 'e';
	length++;

	/* The exponent of 0.D1... is one more than the exponent of D1.D2... */
	power = value == 0.0 ? 0 : exponent - 1;
	out[length] = power < 0 ? '-' : '+';
	length++;
	if (power < 0)
		power = -power;
	/* C asks for at least two digits. */
	out[length] = (char)('0' + power / 10 % 10);
	length++;
	out[length] = (char)('0' + power % 10);
	length++;
	return length;
}

/* `value' in the style of %g: whichever of the two above is shorter, with the
 * trailing zeros of the fraction removed. */
static int vig_general(double value, int precision, int upper, char *out) {
	char digits[VIG_FLOAT_DIGITS + 2];
	int exponent, length, i, keep;

	if (precision == 0)
		precision = 1;
	exponent = vig_significand(value, VIG_FLOAT_DIGITS, digits);
	keep = precision;
	if (keep > VIG_FLOAT_DIGITS)
		keep = VIG_FLOAT_DIGITS;
	vig_round_digits(digits, VIG_FLOAT_DIGITS, keep, &exponent);

	/* C chooses the scientific form when the exponent is small or large. */
	if (value != 0.0 && (exponent - 1 < -4 || exponent - 1 >= precision))
		length = vig_scientific(value, precision - 1, upper, out);
	else
		length = vig_fixed(value, precision - exponent < 0 ? 0 : precision - exponent, out);

	/* Strip a trailing run of zeros, and the point if nothing follows it. */
	for (i = 0; i < length; i++)
		if (out[i] == '.')
			break;
	if (i < length) {
		int end = length;
		int stop = i;

		for (i = 0; i < length; i++)
			if (out[i] == 'e' || out[i] == 'E') {
				end = i;
				break;
			}
		i = end;
		while (i > stop + 1 && out[i - 1] == '0')
			i--;
		if (i > stop && out[i - 1] == '.')
			i--;
		if (i < end) {
			int j;
			for (j = 0; j + end < length; j++)
				out[i + j] = out[end + j];
			length = i + (length - end);
		}
	}
	return length;
}

/* The text of a floating-point conversion, and its length.  A NaN and an
 * infinity have no digits, so they are named. */
static int vig_float_text(double value, int conversion, int precision,
	int *negative, char *out) {
	int upper = conversion == 'E' || conversion == 'G' || conversion == 'F';

	*negative = 0;
	if (value < 0.0 || (value == 0.0 && 1.0 / value < 0.0)) {
		*negative = 1;
		value = -value;
	}
	if (vig_isnan(value)) {
		*negative = 0;
		out[0] = upper ? 'N' : 'n';
		out[1] = upper ? 'A' : 'a';
		out[2] = upper ? 'N' : 'n';
		return 3;
	}
	if (vig_isinf(value)) {
		out[0] = upper ? 'I' : 'i';
		out[1] = upper ? 'N' : 'n';
		out[2] = upper ? 'F' : 'f';
		return 3;
	}
	if (precision > VIG_MAX_PRECISION)
		precision = VIG_MAX_PRECISION;

	if (conversion == 'e' || conversion == 'E')
		return vig_scientific(value, precision, upper, out);
	if (conversion == 'g' || conversion == 'G')
		return vig_general(value, precision, upper, out);
	return vig_fixed(value, precision, out);
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

static int vig_format(vig_sink *sink, const char *format, va_list arguments) {
	char digits[VIG_DIGITS_MAX];
	const char *text;
	int left, fill, width, length, negative, i, item;
	int precision, has_precision, longs;
	unsigned long value;
	long number;
	char conversion;

	while (*format != '\0') {
		if (*format != '%') {
			vig_put(sink, *format);
			format++;
		} else {
			format++;
			left = 0;
			fill = ' ';
			width = 0;
			negative = 0;
			length = 0;
			text = 0;
			precision = 6;
			has_precision = 0;
			longs = 0;

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
			/* A precision, which C writes after a point.  A point with no
			 * digits after it means zero, not the default. */
			if (*format == '.') {
				format++;
				has_precision = 1;
				precision = 0;
				while (*format >= '0' && *format <= '9') {
					precision = precision * 10 + (*format - '0');
					format++;
				}
			}
			/* A length modifier.  Under LP64 `l', `ll', `z', `j' and `t' all
			 * name a 64-bit type, so one flag answers for them all; `h' and
			 * `hh' name types that a variadic call has already promoted to
			 * `int'; and `L' is a `long double', which is the same binary64
			 * as a `double' here. */
			while (*format == 'h')
				format++;
			if (*format == 'l') {
				longs = 1;
				format++;
				if (*format == 'l')
					format++;
			} else if (*format == 'z' || *format == 'j' || *format == 't') {
				longs = 1;
				format++;
			} else if (*format == 'L')
				format++;
			conversion = *format;
			if (conversion != '\0')
				format++;

			if (conversion == 'd' || conversion == 'i') {
				number = longs ? va_arg(arguments, long)
					: (long)va_arg(arguments, int);
				value = (unsigned long)number;
				if (number < 0) {
					negative = 1;
					/* Negating as unsigned wraps rather than trapping, which
					 * matters for the most negative value: it has no positive
					 * counterpart and `0 - number' would overflow. */
					value = 0uL - value;
				}
				length = vig_digits(value, 10uL, 0, digits);
			} else if (conversion == 'u') {
				value = longs ? va_arg(arguments, unsigned long)
					: (unsigned long)va_arg(arguments, unsigned);
				length = vig_digits(value, 10uL, 0, digits);
			} else if (conversion == 'x' || conversion == 'X') {
				value = longs ? va_arg(arguments, unsigned long)
					: (unsigned long)va_arg(arguments, unsigned);
				length = vig_digits(value, 16uL, conversion == 'X', digits);
			} else if (conversion == 'f' || conversion == 'F'
			|| conversion == 'e' || conversion == 'E'
			|| conversion == 'g' || conversion == 'G') {
				/* C promotes a variadic `float' to `double', so the slot
				 * holds a binary64 whatever the caller wrote.  Reading a
				 * `float' out of it would read half of one.  See VIG64.md. */
				double number = va_arg(arguments, double);

				length = vig_float_text(number, conversion, precision,
					&negative, digits);
				text = digits;
			} else if (conversion == 'c') {
				digits[0] = (char)va_arg(arguments, int);
				length = 1;
			} else if (conversion == 's') {
				text = (const char *)va_arg(arguments, char *);
				length = vig_strlen(text);
				if (has_precision && precision < length)
					length = precision;
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
				vig_pad(sink, width - item, ' ');
			if (negative != 0)
				vig_put(sink, '-');
			if (left == 0 && fill == '0')
				vig_pad(sink, width - item, '0');

			if (text != 0) {
				for (i = 0; i < length; i++)
					vig_put(sink, text[i]);
			} else if (conversion == 'c' || conversion == '%') {
				vig_put(sink, digits[0]);
			} else if (conversion != 'd' && conversion != 'i'
			&& conversion != 'u' && conversion != 'x' && conversion != 'X') {
				for (i = 0; i < length; i++)
					vig_put(sink, digits[i]);
			} else {
				/* `vig_digits' leaves the number least significant first. */
				for (i = length - 1; i >= 0; i--)
					vig_put(sink, digits[i]);
			}

			if (left != 0)
				vig_pad(sink, width - item, ' ');

		}
	}
	return (int)sink->written;
}

int vprintf(const char *format, va_list arguments) {
	vig_sink sink;

	sink.out = NULL;
	sink.limit = 0;
	sink.written = 0;
	return vig_format(&sink, format, arguments);
}

int printf(const char *format, ...) {
	va_list arguments;
	int written;

	va_start(arguments, format);
	written = vprintf(format, arguments);
	va_end(arguments);
	return written;
}

/* `sprintf' trusts the buffer, as C does: there is no limit to compare against,
 * so the sink is given the largest one there is. */
int vsprintf(char *out, const char *format, va_list arguments) {
	vig_sink sink;

	sink.out = out;
	sink.limit = (size_t)-1;
	sink.written = 0;
	vig_format(&sink, format, arguments);
	out[sink.written] = '\0';
	return (int)sink.written;
}

int sprintf(char *out, const char *format, ...) {
	va_list arguments;
	int written;

	va_start(arguments, format);
	written = vsprintf(out, format, arguments);
	va_end(arguments);
	return written;
}

/* `snprintf' reports what the whole result would have been, which is how a
 * caller learns the buffer was too small. */
int vsnprintf(char *out, size_t limit, const char *format, va_list arguments) {
	vig_sink sink;

	sink.out = out;
	sink.limit = limit;
	sink.written = 0;
	vig_format(&sink, format, arguments);
	if (limit > 0)
		out[sink.written < limit - 1 ? sink.written : limit - 1] = '\0';
	return (int)sink.written;
}

int snprintf(char *out, size_t limit, const char *format, ...) {
	va_list arguments;
	int written;

	va_start(arguments, format);
	written = vsnprintf(out, limit, format, arguments);
	va_end(arguments);
	return written;
}
