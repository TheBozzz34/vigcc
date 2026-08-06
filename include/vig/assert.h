/* Assertions.
 *
 * This file has no include guard of its own, on purpose.  C says `assert' is
 * redefined every time <assert.h> is included, according to whether NDEBUG is
 * defined *at that moment*, so a program can turn assertions off for one part
 * of itself and on again for another.  Only the helper below is guarded.
 *
 * A failing assertion stops the program with `__vig_halt' rather than with
 * `abort' from <stdlib.h>.  There is no dead-code elimination here, so pulling
 * in <stdlib.h> would put its heap -- 64 KiB of zero-filled memory -- into
 * every program that asserts anything.
 *
 * The message goes to stdout, because the VM has one output stream.
 */

#ifndef VIG_ASSERT_HELPER
#define VIG_ASSERT_HELPER

#include <stdio.h>
#include <vig.h>

/* `__FILE__' is the path as the compiler was given it, which is absolute when a
 * build system passes it that way.  Only the last component is printed, so the
 * message reads the same wherever the source tree happens to sit. */
static const char *vig_assert_file(const char *path) {
	const char *base = path;
	const char *p;

	for (p = path; *p != '\0'; p++)
		if (*p == '/' || *p == '\\')
			base = p + 1;
	return base;
}

static void vig_assert_failed(const char *expression, const char *path, int line) {
	printf("Assertion failed: %s, file %s, line %d\n",
		expression, vig_assert_file(path), line);
	__vig_halt(1);
}

#endif

#undef assert

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) \
	((condition) ? (void)0 : vig_assert_failed(#condition, __FILE__, __LINE__))
#endif
