/* Floating point.
 *
 * Every value here is a `float' on both sides, because that is the one way to
 * compare against a host C compiler: `double' is 64 bits there and binary32
 * here, so any expression that went through a `double' would legitimately
 * differ in its last digits.  With `float' throughout, IEEE-754 fixes every
 * result exactly and the two must agree bit for bit.
 *
 * Results are printed as integers for the same reason -- printf has no %f in
 * this library yet -- so each is scaled to make the fractional part visible.
 */
#include <stdio.h>

static float halve(float x) { return x / 2.0f; }

static float poly(float x) { return 3.0f*x*x - 2.0f*x + 1.0f; }

int main(void) {
    float a = 2.5f, b = 4.0f, zero = 0.0f;
    float values[5];
    int i;

    /* Arithmetic that is exact in binary32, so the scaling below is exact too. */
    printf("arith:%d %d %d %d\n", (int)((a + b)*100), (int)((a - b)*100),
        (int)(a*b*100), (int)((b/a)*100));
    printf("calls:%d %d\n", (int)(halve(21.0f)*100), (int)(poly(2.0f)*100));

    /* A quarter is exact; a tenth is not, and the error is the format's. */
    printf("exact:%d %d\n", (int)(0.25f*4.0f), (int)(0.5f + 0.25f + 0.125f == 0.875f));
    printf("tenth:%d\n", (int)(0.1f*10.0f == 1.0f));

    /* 24 bits of significand: adding one to 2^24 changes nothing. */
    printf("precision:%d %d\n",
        (int)(16777216.0f + 1.0f == 16777216.0f),
        (int)(16777216.0f + 2.0f == 16777218.0f));

    /* Comparisons, including the two zeros, which differ in bits and compare
     * equal. */
    printf("compare:%d %d %d %d %d %d\n", a < b, a > b, a <= a, a >= a,
        a == 2.5f, a != b);
    printf("zeros:%d %d\n", 0.0f == -0.0f, 1.0f/(-0.0f) < 0.0f);

    /* Infinities and NaN, which arrive without a trap. */
    {
        float inf = 1.0f/zero;
        float nan = zero/zero;

        printf("inf:%d %d %d\n", inf > 1.0e38f, -inf < -1.0e38f, inf + 1.0f == inf);
        /* A NaN is false against everything, itself included, except !=. */
        printf("nan:%d %d %d %d\n", nan == nan, nan < 1.0f, nan > 1.0f, nan != nan);
        printf("over:%d %d\n", 3.0e38f*3.0e38f > 1.0e38f, 1.0e-30f*1.0e-30f == 0.0f);
    }

    /* Conversions in both directions, and the truncation toward zero that C
     * requires of a cast. */
    printf("trunc:%d %d %d %d\n", (int)2.9f, (int)-2.9f, (int)0.9f, (int)-0.9f);
    printf("widen:%d %d\n", (int)((float)7/2.0f*100), (int)((float)-7/2.0f*100));
    printf("unsigned:%d\n", (int)((float)4000000000u/1000.0f));

    /* An array of floats, indexed and summed, so loads and stores of the type
     * are exercised rather than only values in registers. */
    for (i = 0; i < 5; i++)
        values[i] = (float)i * 1.5f;
    {
        float total = 0.0f;
        for (i = 0; i < 5; i++)
            total = total + values[i];
        printf("array:%d %d\n", (int)(total*100), (int)(values[3]*100));
    }
    return 0;
}
