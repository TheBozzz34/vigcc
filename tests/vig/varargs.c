#include <stdarg.h>

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
    int zero = 0;
    if (sum(3, 4, 5, 6) != 18)
        return 1 / zero;
    return 0;
}
