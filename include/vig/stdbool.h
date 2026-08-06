#ifndef VIG_STDBOOL_H
#define VIG_STDBOOL_H

/* Booleans.
 *
 * C99 defines `bool' as `_Bool', a type that holds only 0 or 1 and converts
 * anything else to 1 on assignment.  This compiler is C89 and has no `_Bool',
 * so `bool' is an `int' here.
 *
 * The difference shows in one place: `bool b = 5;' stores 5 rather than 1, so
 * `b == true' is false while `b' is still true.  Compare against zero, or
 * against nothing at all -- `if (b)' -- rather than against `true', and the two
 * behave the same.  Every operator that yields a truth value (`==', `<', `!',
 * `&&', `||') already yields 0 or 1, so a `bool' holding one of those is exact.
 */

typedef int bool;

#define true  1
#define false 0

#define __bool_true_false_are_defined 1

#endif
