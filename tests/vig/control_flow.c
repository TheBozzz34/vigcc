/* Loops, branches and the paths that run zero times. */
#include <vig.h>

int count_to(int limit) {
    int value = 0;

    while (value < limit)
        value = value + 1;
    return value;
}

int classify(int value) {
    if (value < 0)
        return -1;
    else if (value == 0)
        return 0;
    else
        return 1;
}

int main(void) {
    __vig_print(count_to(7));
    __vig_print(count_to(0));
    __vig_print(count_to(-3));
    __vig_print(classify(-5));
    __vig_print(classify(0));
    __vig_print(classify(9));
    return 0;
}
