int select_value(int value) {
    switch (value) {
    case 0: return 10;
    case 1: return 11;
    case 2: return 12;
    case 3: return 13;
    case 4: return 14;
    default: return 99;
    }
}

int main(void) {
    int zero = 0;

    if (select_value(4) != 14 || select_value(9) != 99)
        return 1 / zero;
    return 0;
}
