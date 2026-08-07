/* printf's floating-point conversions.
 *
 * Checked against the system C library: the same source compiled natively
 * prints the same bytes.  Every value is a float on both sides, so the host's
 * double holds exactly what binary32 holds and the digits must agree.
 *
 * The exactly-representable values are the ones that pin the rounding: 0.125
 * at two decimals and 1234.5 at three significant digits are exact ties, and C
 * rounds a tie to an even digit rather than away from zero.
 */
#include <stdio.h>

int main(void) {
    /* Values exactly representable in binary32, where the host's double holds
       exactly the same number and both must print the same digits. */
    static const float exact[] = {
        0.0f, 1.0f, 0.5f, 0.25f, 0.125f, 2.5f, 42.0f, -42.0f, -0.5f,
        100.0f, 1024.0f, 65536.0f, 0.0625f, 3.0f, 7.5f, 1000000.0f
    };
    /* Values that are not, where the two agree only as far as the printed
       precision reaches. */
    static const float inexact[] = { 0.1f, 0.2f, 1.0f/3.0f, 3.14159f, 2.718281828f };
    int i;

    for (i = 0; i < (int)(sizeof exact / sizeof exact[0]); i++)
        printf("f:%f e:%e g:%g\n", exact[i], exact[i], exact[i]);

    for (i = 0; i < (int)(sizeof inexact / sizeof inexact[0]); i++)
        printf("f:%f e:%e g:%g\n", inexact[i], inexact[i], inexact[i]);

    /* Precision. */
    printf("p:%.0f %.1f %.2f %.5f %.9f\n", 1.5f, 1.5f, 1.5f, 1.5f, 1.5f);
    printf("p:%.0f %.1f %.2f %.5f\n", 0.125f, 0.125f, 0.125f, 0.125f);
    printf("p:%.3e %.1e %.0e\n", 1234.5f, 1234.5f, 1234.5f);
    printf("p:%.3g %.1g %.8g\n", 1234.5f, 1234.5f, 1234.5f);

    /* Width, alignment and the zero flag, with a sign in the field. */
    printf("w:[%12.4f][%-12.4f][%012.4f]\n", -12.75f, -12.75f, -12.75f);
    printf("w:[%12.4e][%-12.4e][%012.4e]\n", -12.75f, -12.75f, -12.75f);
    printf("w:[%8g][%-8g]\n", 0.5f, 0.5f);

    /* The two zeros. */
    printf("z:%f %f %g %e\n", 0.0f, -0.0f, -0.0f, -0.0f);

    /* Where %g switches between the two forms. */
    printf("g:%g %g %g %g %g\n", 0.0001f, 0.00001f, 123456.0f, 1234567.0f, 1.0f);

    /* Integers that a float holds exactly, at the top of its range for this. */
    printf("i:%f %f\n", 16777216.0f, 8388608.0f);
    return 0;
}
