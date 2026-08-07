
/* Character classification.
 *
 * There is one locale here -- the "C" locale -- so every one of these is a
 * fixed range of the ASCII set and none of them can change under a program's
 * feet.  They are written as range tests rather than as a lookup table: there
 * is no speed to be won that matters, and a range test is its own explanation.
 *
 * C says the argument must be representable as `unsigned char' or be EOF, and
 * leaves anything else undefined.  These do something sensible regardless: a
 * value outside every range simply belongs to no class, so EOF and any stray
 * negative answer false to all the `is' functions and pass through the two
 * `to' functions unchanged.
 *
 * Each `is' function returns 1 or 0.  C promises only "nonzero" for true, and
 * some C libraries return other nonzero values, so a program comparing one of
 * these against 1 is relying on more than C gives it.
 */

#include <ctype.h>
#include <stdio.h>	/* for EOF */

int isdigit(int c) {
	return c >= '0' && c <= '9';
}

int isupper(int c) {
	return c >= 'A' && c <= 'Z';
}

int islower(int c) {
	return c >= 'a' && c <= 'z';
}

int isalpha(int c) {
	return isupper(c) || islower(c);
}

int isalnum(int c) {
	return isalpha(c) || isdigit(c);
}

int isxdigit(int c) {
	return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/* The six characters that separate words in the C locale. */
int isspace(int c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\v'
		|| c == '\f' || c == '\r';
}

int iscntrl(int c) {
	return (c >= 0 && c <= 31) || c == 127;
}

/* `isprint' takes the space and `isgraph' does not; that is the whole of the
 * difference between them. */
int isprint(int c) {
	return c >= ' ' && c <= '~';
}

int isgraph(int c) {
	return c > ' ' && c <= '~';
}

int ispunct(int c) {
	return isgraph(c) && !isalnum(c);
}

int tolower(int c) {
	return isupper(c) ? c - 'A' + 'a' : c;
}

int toupper(int c) {
	return islower(c) ? c - 'a' + 'A' : c;
}
