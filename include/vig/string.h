#ifndef VIG_STRING_H
#define VIG_STRING_H

/* The string and memory functions, written in C.
 *
 * VIG has no linker, so these are defined here and a program gets them by
 * including this file.  Nothing here reaches the VM: every one is ordinary C
 * over ordinary pointers, so all of them work on an automatic array as well as
 * on a global.  That is not true of `__vig_print_string', which hands its
 * pointer to the VM and therefore needs the string to be in the program image.
 *
 * A comparison reads its bytes as `unsigned char', which is what C requires:
 * `strcmp("\200", "\001")' is positive, and reading the bytes as a signed
 * `char' would make it negative.
 */

#include <stddef.h>

void *memcpy(void *destination, const void *source, size_t count) {
	unsigned char *to = (unsigned char *)destination;
	const unsigned char *from = (const unsigned char *)source;
	size_t i;

	for (i = 0; i < count; i++)
		to[i] = from[i];
	return destination;
}

/* The regions may overlap, so the direction of the copy has to suit which way
 * they overlap: copying forwards into a later region would read bytes that the
 * copy has already overwritten. */
void *memmove(void *destination, const void *source, size_t count) {
	unsigned char *to = (unsigned char *)destination;
	const unsigned char *from = (const unsigned char *)source;
	size_t i;

	if (to < from) {
		for (i = 0; i < count; i++)
			to[i] = from[i];
	} else if (to > from) {
		i = count;
		while (i > 0) {
			i--;
			to[i] = from[i];
		}
	}
	return destination;
}

void *memset(void *destination, int value, size_t count) {
	unsigned char *to = (unsigned char *)destination;
	size_t i;

	for (i = 0; i < count; i++)
		to[i] = (unsigned char)value;
	return destination;
}

int memcmp(const void *left, const void *right, size_t count) {
	const unsigned char *a = (const unsigned char *)left;
	const unsigned char *b = (const unsigned char *)right;
	size_t i;

	for (i = 0; i < count; i++)
		if (a[i] != b[i])
			return (int)a[i] - (int)b[i];
	return 0;
}

void *memchr(const void *block, int value, size_t count) {
	const unsigned char *bytes = (const unsigned char *)block;
	unsigned char wanted = (unsigned char)value;
	size_t i;

	for (i = 0; i < count; i++)
		if (bytes[i] == wanted)
			return (void *)(bytes + i);
	return NULL;
}

size_t strlen(const char *text) {
	size_t length = 0;

	while (text[length] != '\0')
		length++;
	return length;
}

char *strcpy(char *destination, const char *source) {
	size_t i = 0;

	while (source[i] != '\0') {
		destination[i] = source[i];
		i++;
	}
	destination[i] = '\0';
	return destination;
}

/* `strncpy' writes exactly `count' bytes: it pads a short source with NULs and
 * does not terminate one that fills the whole field. */
char *strncpy(char *destination, const char *source, size_t count) {
	size_t i = 0;

	while (i < count && source[i] != '\0') {
		destination[i] = source[i];
		i++;
	}
	while (i < count) {
		destination[i] = '\0';
		i++;
	}
	return destination;
}

char *strcat(char *destination, const char *source) {
	strcpy(destination + strlen(destination), source);
	return destination;
}

/* `strncat' takes at most `count' bytes from the source and always terminates,
 * so it can write `count' + 1 bytes.  `strncpy' does neither of those things. */
char *strncat(char *destination, const char *source, size_t count) {
	size_t end = strlen(destination);
	size_t i = 0;

	while (i < count && source[i] != '\0') {
		destination[end + i] = source[i];
		i++;
	}
	destination[end + i] = '\0';
	return destination;
}

int strcmp(const char *left, const char *right) {
	size_t i = 0;

	while (left[i] != '\0' && left[i] == right[i])
		i++;
	return (int)(unsigned char)left[i] - (int)(unsigned char)right[i];
}

int strncmp(const char *left, const char *right, size_t count) {
	size_t i = 0;

	while (i < count) {
		if (left[i] != right[i])
			return (int)(unsigned char)left[i] - (int)(unsigned char)right[i];
		if (left[i] == '\0')
			return 0;
		i++;
	}
	return 0;
}

/* The terminator is part of the string, so `strchr(text, 0)' finds it. */
char *strchr(const char *text, int value) {
	char wanted = (char)value;
	size_t i = 0;

	for (;;) {
		if (text[i] == wanted)
			return (char *)(text + i);
		if (text[i] == '\0')
			return NULL;
		i++;
	}
}

char *strrchr(const char *text, int value) {
	char wanted = (char)value;
	const char *found = NULL;
	size_t i = 0;

	for (;;) {
		if (text[i] == wanted)
			found = text + i;
		if (text[i] == '\0')
			break;
		i++;
	}
	return (char *)found;
}

/* An empty needle is found immediately, which is what C requires. */
char *strstr(const char *haystack, const char *needle) {
	size_t i, j;

	if (needle[0] == '\0')
		return (char *)haystack;
	for (i = 0; haystack[i] != '\0'; i++) {
		j = 0;
		while (needle[j] != '\0' && haystack[i + j] == needle[j])
			j++;
		if (needle[j] == '\0')
			return (char *)(haystack + i);
	}
	return NULL;
}

/* The message for an error number.  C puts this in <string.h> rather than in
 * <errno.h>, and leaves the wording to the implementation.  The returned string
 * must not be modified, and a later call may overwrite it -- this one returns
 * literals, so it never does. */
char *strerror(int number) {
	switch (number) {
	case 0:  return "no error";
	case 2:  return "no such file or directory";
	case 5:  return "input/output error";
	case 9:  return "bad file descriptor";
	case 12: return "not enough memory";
	case 13: return "permission denied";
	case 14: return "bad address";
	case 17: return "file exists";
	case 22: return "invalid argument";
	case 28: return "no space left on device";
	case 33: return "argument out of domain";
	case 34: return "result out of range";
	case 84: return "illegal byte sequence";
	}
	return "unknown error";
}

size_t strspn(const char *text, const char *accept) {
	size_t length = 0;

	while (text[length] != '\0' && strchr(accept, text[length]) != NULL)
		length++;
	return length;
}

size_t strcspn(const char *text, const char *reject) {
	size_t length = 0;

	while (text[length] != '\0' && strchr(reject, text[length]) == NULL)
		length++;
	return length;
}

char *strpbrk(const char *text, const char *accept) {
	size_t i = 0;

	while (text[i] != '\0') {
		if (strchr(accept, text[i]) != NULL)
			return (char *)(text + i);
		i++;
	}
	return NULL;
}

#endif
