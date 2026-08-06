/* Calls, parameters and integer arithmetic. */
#include <vig.h>

int add_one(int value) {
    return value + 1;
}

int sum(int left, int right) {
    return left + right;
}

int main(void) {
    __vig_print(add_one(41));
    __vig_print(sum(30, 12));
    __vig_print(sum(add_one(-1), 7));
    __vig_print(add_one(add_one(add_one(39))));
    return 0;
}
