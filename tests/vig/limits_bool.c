/* <limits.h> and <stdbool.h>.
 *
 * The limit values were checked against the system C library: the same source
 * compiled natively prints the same numbers.  VIG64 is LP64, so `long' and
 * `long long' are both 64 bits; that agrees with a 64-bit Unix but not with
 * Windows, where `long' is 32.  MB_LEN_MAX is likewise a property of the
 * platform: it is 1 here because this C subset has no multibyte characters.
 *
 * The `bool' section is deliberately not oracle-compared: C99 `bool' is `_Bool'
 * and this compiler is C89, so `bool' is an `int' here.  The difference is
 * shown and explained rather than hidden.
 */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

int main(void) {
    bool yes = true;
    bool no = false;
    bool odd;
    int i;

    printf("char:%d %d %d %d\n", CHAR_BIT, CHAR_MIN, CHAR_MAX, UCHAR_MAX);
    printf("schar:%d %d\n", SCHAR_MIN, SCHAR_MAX);
    printf("short:%d %d %d\n", SHRT_MIN, SHRT_MAX, USHRT_MAX);
    printf("int:%d %d %u\n", INT_MIN, INT_MAX, UINT_MAX);
    printf("long:%ld %ld %lu\n", LONG_MIN, LONG_MAX, ULONG_MAX);
    printf("llong:%lld %lld %llu\n", LLONG_MIN, LLONG_MAX, ULLONG_MAX);
    printf("mb:%d\n", MB_LEN_MAX);

    /* The limits are the real edges of the types: one step past either end
     * wraps around to the other, using unsigned arithmetic so it is defined. */
    printf("edge:%d %d\n",
        (int)((unsigned)INT_MAX + 1u) == INT_MIN,
        (int)((unsigned)INT_MIN - 1u) == INT_MAX);
    printf("width:%d %d %d %d %d\n",
        (int)sizeof(char) * CHAR_BIT, (int)sizeof(short) * CHAR_BIT,
        (int)sizeof(int) * CHAR_BIT, (int)sizeof(long) * CHAR_BIT,
        (int)sizeof(void *) * CHAR_BIT);

    /* Booleans that came from an operator hold exactly 0 or 1. */
    printf("bool:%d %d %d %d\n", yes, no, 1 == 1, 1 == 2);
    printf("logic:%d %d %d\n", yes && no, yes || no, !no);
    for (i = 0; i < 3; i++) {
        odd = i % 2 != 0;
        printf("%d", odd);
    }
    printf(" parity\n");

    /* Where this `bool' differs from C99's: an `int' keeps whatever it was
     * given, so a value other than 0 or 1 stays itself.  It is still true. */
    odd = 5;
    printf("deviation:%d %d %d\n", odd, odd == true, odd ? 1 : 0);
    return 0;
}
