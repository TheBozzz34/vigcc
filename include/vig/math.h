#ifndef VIG_MATH_H
#define VIG_MATH_H

/* The mathematical functions.
 *
 * Only `sqrt' is a VM instruction, because IEEE-754 specifies the square root
 * exactly and it therefore gives the same bits on every host.  Nothing else
 * here is specified to the last bit by anything, and one platform's library
 * differs from another's in the final digits.  Implementing them in C, in
 * `runtime/math.c', is what keeps a VIG program reproducible: the answer is
 * whatever that source computes, on every machine.
 *
 * Every value is binary32, which carries about seven decimal digits.  These are
 * accurate to a few units in the last place over their useful range -- good
 * enough to print at any precision the format can show, and not a substitute
 * for a library that promises correct rounding.
 *
 * `double' is binary32 here too, so the `f'-suffixed names would be the same
 * functions rather than narrower ones.  See ABI.md.
 */

#define VIG_PI      3.14159265358979323846f
#define VIG_HALF_PI 1.57079632679489661923f
#define VIG_LN2     0.69314718055994530942f
#define VIG_LN10    2.30258509299404568402f

#ifndef HUGE_VAL
/* An overflow, which is what C says these return when the result is too large.
 * Written as a division rather than a literal: a binary32 infinity has no
 * decimal form for the compiler to fold. */
#define HUGE_VAL  (1.0f / 0.0f)
#define HUGE_VALF HUGE_VAL
#endif

int isnan(float value);
int isinf(float value);

float fabs(float value);
float sqrt(float value);
float floor(float value);
float ceil(float value);
float modf(float value, float *whole);
float fmod(float value, float divisor);
float ldexp(float value, int power);
float frexp(float value, int *power);

float exp(float value);
float log(float value);
float log10(float value);
float pow(float base, float power);

float sin(float value);
float cos(float value);
float tan(float value);
float atan(float value);
float atan2(float y, float x);
float asin(float value);
float acos(float value);

float sinh(float value);
float cosh(float value);
float tanh(float value);

#endif
