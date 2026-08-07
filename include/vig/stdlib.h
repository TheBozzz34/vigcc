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
	int (*compare)(const void *, const void *)) {
	const char *items = (const char *)base;
	size_t low = 0, high = count, middle;
	int order;

	while (low < high) {
		/* Written as a difference so the midpoint cannot overflow, which it
		 * could for a count near the top of a 32-bit size. */
		middle = low + (high - low)/2;
		order = compare(key, items + middle*size);
		if (order == 0)
			return (void *)(items + middle*size);
		if (order < 0)
			high = middle;
		else
			low = middle + 1;
	}
	return NULL;
}

int abs(int value) {
	return value < 0 ? -value : value;
}

/* Read a decimal floating-point number, and report where it stopped.
 *
 * The digits are accumulated as an integer and scaled once at the end, so the
 * result rounds once rather than once per digit.  More than nine digits cannot
 * change a binary32, so the rest only move the exponent. */
float strtod(const char *text, char **end) {
	const char *start = text;
	unsigned mantissa = 0;
	int digits = 0, exponent = 0, negative = 0, seen = 0;
	float value;

	while (*text == ' ' || *text == 9 || *text == 10 || *text == 13)
		text++;
	if (*text == '-' || *text == '+') {
		negative = *text == '-';
		text++;
	}
	while (*text >= '0' && *text <= '9') {
		seen = 1;
		if (digits < 9) {
			mantissa = mantissa * 10u + (unsigned)(*text - '0');
			digits++;
		} else
			exponent++;	/* past the precision: only the scale still matters */
		text++;
	}
	if (*text == '.') {
		text++;
		while (*text >= '0' && *text <= '9') {
			seen = 1;
			if (digits < 9) {
				mantissa = mantissa * 10u + (unsigned)(*text - '0');
				digits++;
				exponent--;
			}
			text++;
		}
	}
	if (!seen) {
		if (end != 0)
			*end = (char *)start;	/* nothing was a number */
		return 0.0f;
	}
	if (*text == 'e' || *text == 'E') {
		const char *mark = text;
		int power = 0, negative_power = 0, any = 0;

		text++;
		if (*text == '-' || *text == '+') {
			negative_power = *text == '-';
			text++;
		}
		while (*text >= '0' && *text <= '9') {
			any = 1;
			if (power < 1000)
				power = power * 10 + (*text - '0');
			text++;
		}
		if (any)
			exponent = exponent + (negative_power ? -power : power);
		else
			text = mark;	/* an `e` with no digits is not part of the number */
	}

	value = (float)mantissa;
	while (exponent > 0) {
		value = value * 10.0f;
		exponent--;
	}
	while (exponent < 0) {
		value = value / 10.0f;
		exponent++;
	}
	if (end != 0)
		*end = (char *)text;
	return negative ? -value : value;
}

float atof(const char *text) {
	return strtod(text, 0);
}

int atoi(const char *text) {
	int value = 0;
	int negative = 0;

	while (*text == ' ' || *text == '\t' || *text == '\n')
		text++;
	if (*text == '-' || *text == '+') {
		negative = *text == '-';
		text++;
	}
	while (*text >= '0' && *text <= '9') {
		value = value * 10 + (*text - '0');
		text++;
	}
	return negative ? -value : value;
}

#endif
