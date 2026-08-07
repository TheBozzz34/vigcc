/* The mathematical functions, written in C.
 *
 * Only `sqrt' is a VM instruction, because IEEE-754 specifies the square root
 * exactly and it therefore gives the same bits on every host.  Nothing else in
 * this file is specified to the last bit by anything, and one platform's
 * library differs from another's in the final digits.  Implementing them here
 * rather than as instructions is what keeps a VIG program reproducible: the
 * answer is whatever this source computes, on every machine.
 *
 * Every value is binary32, which carries about seven decimal digits.  These are
 * accurate to a few units in the last place over their useful range -- good
 * enough to print at any precision the format can show, and not a substitute
 * for a library that promises correct rounding.
 */

#include <math.h>
#include <vig.h>

static float vig_nan(void) {
	float zero = 0.0f;

	return zero / zero;
}

int isnan(float value) {
	return !(value == value);
}

int isinf(float value) {
	return value != 0.0f && value / 2.0f == value && !isnan(value);
}

float fabs(float value) {
	return value < 0.0f ? -value : value;
}

float sqrt(float value) {
	/* The one instruction: exact, and a negative operand gives a NaN as
	 * IEEE-754 says it should. */
	return __vig_sqrt(value);
}

float floor(float value) {
	float whole;

	if (isnan(value) || isinf(value) || fabs(value) >= 8388608.0f)
		return value;	/* every binary32 this large is already whole */
	whole = (float)(int)value;
	return whole > value ? whole - 1.0f : whole;
}

float ceil(float value) {
	float whole;

	if (isnan(value) || isinf(value) || fabs(value) >= 8388608.0f)
		return value;
	whole = (float)(int)value;
	return whole < value ? whole + 1.0f : whole;
}

/* The whole part toward zero, and the fraction, which is what C splits off. */
float modf(float value, float *whole) {
	float part = value < 0.0f ? ceil(value) : floor(value);

	*whole = part;
	return value - part;
}

float fmod(float value, float divisor) {
	float quotient;

	if (divisor == 0.0f || isnan(value) || isnan(divisor) || isinf(value))
		return vig_nan();
	if (isinf(divisor))
		return value;
	quotient = value / divisor;
	quotient = quotient < 0.0f ? ceil(quotient) : floor(quotient);
	return value - quotient * divisor;
}

float ldexp(float value, int power) {
	/* Doubling and halving are exact, so this loses nothing until it runs out
	 * of exponent, where it gives an infinity or a zero as it should. */
	while (power > 0) {
		value = value * 2.0f;
		power--;
	}
	while (power < 0) {
		value = value * 0.5f;
		power++;
	}
	return value;
}

float frexp(float value, int *power) {
	int count = 0;

	*power = 0;
	if (value == 0.0f || isnan(value) || isinf(value))
		return value;
	while (fabs(value) >= 1.0f) {
		value = value * 0.5f;
		count++;
	}
	while (fabs(value) < 0.5f) {
		value = value * 2.0f;
		count--;
	}
	*power = count;
	return value;
}

/* exp, by range reduction to [-ln2/2, ln2/2] and a Taylor series there.  The
 * series converges quickly that close to zero: nine terms are past what a
 * binary32 can hold. */
float exp(float value) {
	int power;
	float term, sum;
	int i;

	if (isnan(value))
		return value;
	if (value > 88.7f)
		return HUGE_VAL;
	if (value < -103.0f)
		return 0.0f;

	power = (int)(value / VIG_LN2 + (value < 0.0f ? -0.5f : 0.5f));
	value = value - (float)power * VIG_LN2;

	sum = 1.0f;
	term = 1.0f;
	for (i = 1; i <= 9; i++) {
		term = term * value / (float)i;
		sum = sum + term;
	}
	return ldexp(sum, power);
}

/* log, by splitting off the exponent and using the series for the mantissa.
 * atanh converges faster than the plain log series and stays accurate across
 * the whole of [sqrt(1/2), sqrt(2)). */
float log(float value) {
	int power;
	float mantissa, z, z2, sum, term;
	int i;

	if (isnan(value))
		return value;
	if (value < 0.0f)
		return vig_nan();
	if (value == 0.0f)
		return -HUGE_VAL;
	if (isinf(value))
		return value;

	mantissa = frexp(value, &power);
	/* frexp gives [0.5, 1); centring on 1 makes the series converge fastest. */
	if (mantissa < 0.70710678f) {
		mantissa = mantissa * 2.0f;
		power--;
	}
	z = (mantissa - 1.0f) / (mantissa + 1.0f);
	z2 = z * z;
	term = z;
	sum = z;
	for (i = 3; i <= 15; i += 2) {
		term = term * z2;
		sum = sum + term / (float)i;
	}
	return 2.0f * sum + (float)power * VIG_LN2;
}

float log10(float value) {
	return log(value) / VIG_LN10;
}

float pow(float base, float power) {
	int whole;

	/* The cases C names, before anything is computed. */
	if (power == 0.0f)
		return 1.0f;
	if (isnan(base) || isnan(power))
		return vig_nan();
	if (base == 0.0f)
		return power < 0.0f ? HUGE_VAL : 0.0f;

	/* A whole power is repeated multiplication, which is exact where the
	 * format allows and avoids exp and log entirely. */
	whole = (int)power;
	if ((float)whole == power && power > -32.0f && power < 32.0f) {
		float result = 1.0f;
		int count = whole < 0 ? -whole : whole;

		while (count > 0) {
			result = result * base;
			count--;
		}
		return whole < 0 ? 1.0f / result : result;
	}
	if (base < 0.0f)
		return vig_nan();	/* a fractional power of a negative has no real value */
	return exp(power * log(base));
}

/* sin and cos, by reduction to a quarter turn and the Taylor series there. */
static float vig_sin_series(float value) {
	float term = value, sum = value, square = value * value;
	int i;

	for (i = 3; i <= 13; i += 2) {
		term = -term * square / (float)(i * (i - 1));
		sum = sum + term;
	}
	return sum;
}

float sin(float value) {
	int quadrant;
	float reduced;

	if (isnan(value) || isinf(value))
		return vig_nan();
	quadrant = (int)(value / VIG_HALF_PI + (value < 0.0f ? -0.5f : 0.5f));
	reduced = value - (float)quadrant * VIG_HALF_PI;
	quadrant = quadrant % 4;
	if (quadrant < 0)
		quadrant = quadrant + 4;
	switch (quadrant) {
	case 0: return vig_sin_series(reduced);
	case 1: return vig_sin_series(VIG_HALF_PI - reduced) * 1.0f;
	case 2: return -vig_sin_series(reduced);
	}
	return -vig_sin_series(VIG_HALF_PI - reduced);
}

float cos(float value) {
	return sin(value + VIG_HALF_PI);
}

float tan(float value) {
	float c = cos(value);

	if (c == 0.0f)
		return HUGE_VAL;
	return sin(value) / c;
}

/* atan, by reduction to [0, tan(pi/12)] where the series is quick. */
float atan(float value) {
	int negative = 0, inverted = 0;
	float sum, term, square, extra = 0.0f;
	int i;

	if (isnan(value))
		return value;
	if (isinf(value))
		return value > 0.0f ? VIG_HALF_PI : -VIG_HALF_PI;
	if (value < 0.0f) {
		value = -value;
		negative = 1;
	}
	if (value > 1.0f) {
		value = 1.0f / value;
		inverted = 1;
	}
	/* tan(pi/12) is 2 - sqrt(3); above it, subtract pi/6 from the angle. */
	if (value > 0.26794919f) {
		extra = VIG_PI / 6.0f;
		value = (value * 1.73205081f - 1.0f) / (1.73205081f + value);
	}

	square = value * value;
	term = value;
	sum = value;
	for (i = 3; i <= 17; i += 2) {
		term = -term * square;
		sum = sum + term / (float)i;
	}
	sum = sum + extra;
	if (inverted)
		sum = VIG_HALF_PI - sum;
	return negative ? -sum : sum;
}

float atan2(float y, float x) {
	if (isnan(x) || isnan(y))
		return vig_nan();
	if (x == 0.0f) {
		if (y == 0.0f)
			return 0.0f;
		return y > 0.0f ? VIG_HALF_PI : -VIG_HALF_PI;
	}
	if (x > 0.0f)
		return atan(y / x);
	return y >= 0.0f ? atan(y / x) + VIG_PI : atan(y / x) - VIG_PI;
}

float asin(float value) {
	if (isnan(value))
		return value;
	if (value > 1.0f || value < -1.0f)
		return vig_nan();
	if (value == 1.0f)
		return VIG_HALF_PI;
	if (value == -1.0f)
		return -VIG_HALF_PI;
	return atan(value / sqrt(1.0f - value * value));
}

float acos(float value) {
	if (isnan(value))
		return value;
	if (value > 1.0f || value < -1.0f)
		return vig_nan();
	return VIG_HALF_PI - asin(value);
}

float sinh(float value) {
	return (exp(value) - exp(-value)) * 0.5f;
}

float cosh(float value) {
	return (exp(value) + exp(-value)) * 0.5f;
}

float tanh(float value) {
	float e;

	if (value > 15.0f)
		return 1.0f;
	if (value < -15.0f)
		return -1.0f;
	e = exp(2.0f * value);
	return (e - 1.0f) / (e + 1.0f);
}
