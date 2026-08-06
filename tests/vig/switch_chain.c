/* A switch, which the backend compiles to a chain of comparisons. */
#include <vig.h>

int select_value(int value) {
    switch (value) {
    case 0: return 10;
    case 1: return 11;
    case 2: return 12;
    case 3: return 13;
    case 4: return 14;
    default: return 99;
    }
}

int main(void) {
    int i;

    for (i = 0; i < 5; i++)
        __vig_print(select_value(i));
    __vig_print(select_value(9));
    __vig_print(select_value(-1));
    return 0;
}
