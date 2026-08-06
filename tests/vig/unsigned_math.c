/* Unsigned arithmetic: wrapping, the unsigned instructions, and narrowing. */
#include <vig.h>

unsigned char narrow_byte(unsigned int value) {
    return (unsigned char)value;
}

unsigned short narrow_short(unsigned int value) {
    return (unsigned short)value;
}

int main(void) {
    unsigned value = 0;

    /* Unsigned arithmetic wraps rather than trapping. */
    value = value - 1;
    __vig_print(value > 1);
    __vig_print((int)(value / 2u));
    __vig_print((int)(value % 10u));
    __vig_print((int)(value >> 24));

    /* The signed instructions would order these the other way round. */
    __vig_print(1u < value);
    __vig_print((int)(3u * 5u));

    /* A narrowing conversion drops the high bits even when its result is never
     * stored.  This is a regression test: the conversion emitted nothing once,
     * and only a store of the narrow type hid it. */
    __vig_print(narrow_byte(300u));
    __vig_print(narrow_short(70000u));
    __vig_print(((unsigned char)300u) == 44);
    return 0;
}
