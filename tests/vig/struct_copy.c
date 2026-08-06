/* Whole-structure assignment, which the backend lowers to a byte copy. */
#include <vig.h>

struct record {
    char tag;
    int value;
};

int main(void) {
    struct record source;
    struct record copy;

    source.tag = 7;
    source.value = 35;
    copy = source;
    __vig_print(copy.tag);
    __vig_print(copy.value);

    /* The copy is independent of what it was made from. */
    source.value = 0;
    __vig_print(copy.value);
    return 0;
}
