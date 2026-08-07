#include <vig.h>

extern int total;
int add(int, int);

static int increment;
int (*operation)(int, int) = add;

int main(void) {
    __vig_print(add(20, 22));
    __vig_print(total + increment);
    return operation == add ? 0 : 1;
}
