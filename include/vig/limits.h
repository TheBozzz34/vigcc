#ifndef VIG_LIMITS_H
#define VIG_LIMITS_H

/* The ranges of the integer types.
 *
 * Every integer in this C subset is at most 32 bits, so `long' and `int' have
 * the same limits and there is no `long long'.  `char' is signed, matching the
 * `i8' data directive.  See ABI.md.
 *
 * The most negative values are written as `-MAX - 1' rather than as the literal
 * itself.  In C a literal is never negative -- the minus is an operator -- so
 * `-2147483648' is the negation of a value that does not fit an `int', which
 * makes it unsigned and the result wrong.
 */

#define CHAR_BIT   8
#define MB_LEN_MAX 1

#define SCHAR_MAX  127
#define SCHAR_MIN  (-127 - 1)
#define UCHAR_MAX  255

#define CHAR_MAX   SCHAR_MAX
#define CHAR_MIN   SCHAR_MIN

#define SHRT_MAX   32767
#define SHRT_MIN   (-32767 - 1)
#define USHRT_MAX  65535

#define INT_MAX    2147483647
#define INT_MIN    (-2147483647 - 1)
#define UINT_MAX   4294967295U

#define LONG_MAX   INT_MAX
#define LONG_MIN   INT_MIN
#define ULONG_MAX  4294967295UL

#endif
