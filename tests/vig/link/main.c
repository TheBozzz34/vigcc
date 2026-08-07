#include <stdio.h>

extern int total;
int add(int, int);

static int increment;
int (*operation)(int, int) = add;

int main(void) {
    printf("%d\n", add(20, 22));
    printf("%d\n", total + increment);
    return operation == add ? 0 : 1;
}
