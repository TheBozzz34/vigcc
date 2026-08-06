/* Structures passed and returned by value, through the hidden-pointer ABI. */
#include <vig.h>

struct pair {
    int left;
    int right;
};

int sum_pair(struct pair value) {
    return value.left + value.right;
}

struct pair make_pair(int left, int right) {
    struct pair result;

    result.left = left;
    result.right = right;
    return result;
}

int main(void) {
    struct pair source;
    struct pair result;

    source.left = 4;
    source.right = 9;
    __vig_print(sum_pair(source));

    result = make_pair(8, 11);
    __vig_print(result.left);
    __vig_print(result.right);
    __vig_print(sum_pair(make_pair(20, 22)));

    /* A by-value argument is a copy: the callee cannot reach the original. */
    __vig_print(source.left);
    return 0;
}
