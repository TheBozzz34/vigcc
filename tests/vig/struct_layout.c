/* The VIG ABI lays a structure out with no padding. */
#include <vig.h>

struct record {
    char tag;
    int value;
};

struct record initial;

int main(void) {
    struct record local;

    __vig_print((int)sizeof(struct record));
    initial.tag = 1;
    initial.value = 41;
    local.tag = initial.tag;
    local.value = initial.value + 1;
    __vig_print(local.tag);
    __vig_print(local.value);
    /* A global structure starts as zeros. */
    __vig_print(sizeof(struct record) == 5 ? 1 : 0);
    return 0;
}
