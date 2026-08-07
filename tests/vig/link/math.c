#include <stdio.h>

int total;
static int increment = 1;

int add(int a, int b) {
    total += increment;
    return a + b;
}
