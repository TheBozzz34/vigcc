/* Variadic calls, which the ABI passes as a count and an array of slots. */
#include <stdarg.h>
#include <vig.h>

int sum(int fixed, ...) {
    va_list values;
    int total = fixed;
    int i = 0;

    va_start(values, fixed);
    while (i < va_count()) {
        total += va_arg(values, int);
        i++;
    }
    va_end(values);
    return total;
}

int main(void) {
    __vig_print(sum(3, 4, 5, 6));
    /* No variable arguments at all: the count is zero and the list unused. */
    __vig_print(sum(7));
    __vig_print(sum(0, -1, 1));
    __vig_print(sum(1, 2, 3, 4, 5, 6, 7, 8));
    return 0;
}
