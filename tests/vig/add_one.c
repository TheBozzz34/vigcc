int add_one(int value) {
    return value + 1;
}

int sum(int left, int right) {
    return left + right;
}

int main(void) {
    return sum(add_one(40), 1);
}
