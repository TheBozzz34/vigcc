struct pair {
    int left;
    int right;
};

int sum_pair(struct pair value) {
    return value.left + value.right;
}

struct pair make_pair(int left, int right) {
    struct pair result;
    result.left = left;
    result.right = right;
    return result;
}

int main(void) {
    struct pair source;
    struct pair result;
    int zero = 0;

    source.left = 4;
    source.right = 9;
    result = make_pair(8, 11);
    if (sum_pair(source) != 13 || result.left != 8 || result.right != 11)
        return 1 / zero;
    return 0;
}
