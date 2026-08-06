struct record {
    char tag;
    int value;
};

struct record initial;

int main(void) {
    struct record local;
    int zero = 0;

    initial.tag = 1;
    initial.value = 41;
    local.tag = initial.tag;
    local.value = initial.value + 1;
    if (sizeof(struct record) != 5 || local.tag != 1 || local.value != 42)
        return 1 / zero;
    return 0;
}
