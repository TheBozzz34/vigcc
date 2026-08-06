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
    int zero = 0;

    if (total() != 60)
        return 1 / zero;
    return 0;
}
