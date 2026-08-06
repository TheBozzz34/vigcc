int count_to(int limit) {
    int value = 0;

    while (value < limit)
        value = value + 1;
    return value;
}

int main(void) {
    int zero = 0;

    if (count_to(7) != 7)
        return 1 / zero;
    return 0;
}
