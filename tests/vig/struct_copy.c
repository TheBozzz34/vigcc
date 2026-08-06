struct record {
    char tag;
    int value;
};

int main(void) {
    struct record source;
    struct record copy;
    int zero = 0;

    source.tag = 7;
    source.value = 35;
    copy = source;
    if (copy.tag != 7 || copy.value != 35)
        return 1 / zero;
    return 0;
}
