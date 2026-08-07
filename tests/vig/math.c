/* <math.h> and the floating-point parts of <stdlib.h>.
 *
 * This one is *not* compared byte for byte against the system C library, and
 * that is deliberate.  Only `sqrt' is specified exactly by IEEE-754; every
 * other function here is a series this file chose, and a host library chooses a
 * different one.  Two correct implementations disagree in the last digits, so
 * comparing their output would be comparing the algorithms rather than the
 * answers.
 *
 * What is checked instead is that each answer is within a tolerance of the
 * value it should have, and that the identities that must hold do.  The
 * tolerance is 1e-5 relative, which is loose for binary32's seven digits and
 * tight enough that a wrong formula cannot pass.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;

/* Report whether `got' is `want' to within the tolerance, so the recorded
 * output is a row of ones rather than digits that depend on the algorithm. */
static int near(float got, float want) {
    float error = got - want;
    float scale = want < 0.0f ? -want : want;

    if (error < 0.0f)
        error = -error;
    if (scale < 1.0f)
        scale = 1.0f;
    if (!(error / scale <= 1.0e-5f)) {
        failures++;
        return 0;
    }
    return 1;
}

int main(void) {
    float whole;
    char *end;
    int power;
    int i;

    /* sqrt is the instruction, and it is exact. */
    printf("sqrt:%d%d%d%d\n", sqrt(4.0f) == 2.0f, sqrt(0.0f) == 0.0f,
        near(sqrt(2.0f), 1.41421356f), isnan(sqrt(-1.0f)));

    printf("round:%d%d%d%d%d%d\n",
        floor(2.7f) == 2.0f, floor(-2.1f) == -3.0f,
        ceil(2.1f) == 3.0f, ceil(-2.7f) == -2.0f,
        floor(-0.0f) == 0.0f, ceil(5.0f) == 5.0f);

    printf("fabs:%d%d%d\n", fabs(-3.5f) == 3.5f, fabs(3.5f) == 3.5f,
        fabs(0.0f) == 0.0f);

    printf("fmod:%d%d%d\n", near(fmod(7.0f, 3.0f), 1.0f),
        near(fmod(-7.0f, 3.0f), -1.0f), isnan(fmod(1.0f, 0.0f)));

    printf("modf:%d%d\n", near(modf(3.75f, &whole), 0.75f), whole == 3.0f);

    printf("exp:%d%d%d%d\n", near(exp(0.0f), 1.0f), near(exp(1.0f), 2.71828183f),
        near(exp(-1.0f), 0.36787944f), near(exp(10.0f), 22026.4658f));

    printf("log:%d%d%d%d\n", near(log(1.0f), 0.0f), near(log(2.71828183f), 1.0f),
        near(log(10.0f), 2.30258509f), near(log10(1000.0f), 3.0f));

    /* The identity that catches a wrong range reduction in either. */
    for (i = 1; i <= 8; i++)
        near(log(exp((float)i)), (float)i);
    printf("roundtrip:%d\n", failures == 0);

    printf("pow:%d%d%d%d%d\n", pow(2.0f, 10.0f) == 1024.0f,
        near(pow(2.0f, 0.5f), 1.41421356f), pow(5.0f, 0.0f) == 1.0f,
        near(pow(2.0f, -2.0f), 0.25f), near(pow(9.0f, 0.5f), 3.0f));

    printf("sin:%d%d%d%d\n", near(sin(0.0f), 0.0f), near(sin(1.5707963f), 1.0f),
        near(sin(3.1415927f), 0.0f), near(sin(-1.5707963f), -1.0f));

    printf("cos:%d%d%d\n", near(cos(0.0f), 1.0f), near(cos(3.1415927f), -1.0f),
        near(cos(1.5707963f), 0.0f));

    /* The Pythagorean identity, over a full turn. */
    for (i = -6; i <= 6; i++) {
        float angle = (float)i * 0.5f;
        near(sin(angle)*sin(angle) + cos(angle)*cos(angle), 1.0f);
    }
    printf("identity:%d\n", failures == 0);

    printf("tan:%d%d\n", near(tan(0.0f), 0.0f), near(tan(0.7853982f), 1.0f));

    printf("atan:%d%d%d%d\n", near(atan(0.0f), 0.0f), near(atan(1.0f), 0.78539816f),
        near(atan(-1.0f), -0.78539816f), near(atan(1000.0f), 1.56979633f));	/* pi/2 - 1/1000, not pi/2 */

    printf("atan2:%d%d%d%d\n", near(atan2(1.0f, 1.0f), 0.78539816f),
        near(atan2(1.0f, -1.0f), 2.35619449f),
        near(atan2(-1.0f, -1.0f), -2.35619449f),
        near(atan2(1.0f, 0.0f), 1.57079633f));

    printf("asin:%d%d%d\n", near(asin(0.0f), 0.0f), near(asin(1.0f), 1.57079633f),
        near(acos(0.0f), 1.57079633f));

    printf("hyper:%d%d%d\n", near(sinh(1.0f), 1.17520119f),
        near(cosh(1.0f), 1.54308063f), near(tanh(1.0f), 0.76159416f));

    printf("scale:%d%d%d\n", ldexp(1.5f, 3) == 12.0f, ldexp(8.0f, -2) == 2.0f,
        frexp(12.0f, &power) == 0.75f && power == 4);

    /* strtod and atof, which have to read back what printf writes. */
    printf("strtod:%d%d%d%d\n", near(atof("3.25"), 3.25f), near(atof("-0.5"), -0.5f),
        near(atof("1.5e3"), 1500.0f), near(atof("2.5e-2"), 0.025f));
    /* Each call is its own statement: C does not say in which order the
     * arguments of one call are evaluated, so reading `end' in the same printf
     * that another argument reassigns it would be reading either answer. */
    {
        float parsed = strtod("1.25xyz", &end);

        printf("strtod:%.4f [%s]\n", parsed, end);
        parsed = strtod("abc", &end);
        printf("strtod:%d\n", parsed == 0.0f && *end == 'a');
    }

    printf("failures:%d\n", failures);
    return 0;
}
