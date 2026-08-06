/* Arrays, pointer arithmetic and pointer difference. */
#include <vig.h>

int values[3] = { 10, 20, 30 };

int total(void) {
    int *cursor = values;
    int sum = 0;

    while (cursor != values + 3) {
        sum = sum + *cursor;
        cursor = cursor + 1;
    }
    return sum;
}

int main(void) {
    int *cursor = values + 2;

    __vig_print(total());
    __vig_print(values[1]);
    __vig_print(*(values + 2));
    __vig_print(*cursor);
    /* A pointer difference is a count of elements, not of bytes. */
    __vig_print((int)(cursor - values));
    return 0;
}
