/* qsort and bsearch, checked against the system C library: the same source
 * compiled natively produces the same bytes.
 *
 * The comparison function is reached only through the pointer the caller
 * gave, so every call to it is a call_indirect and the function is verified
 * when the first one arrives.  Nothing here depends on the order of equal
 * elements, which no qsort promises: the int arrays hold values that are
 * indistinguishable when equal, and the struct and name arrays have unique
 * keys. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct person {
    int key;
    const char *name;
};

static int by_int(const void *a, const void *b) {
    int left = *(const int *)a;
    int right = *(const int *)b;
    return left < right ? -1 : (left > right ? 1 : 0);
}

static int by_int_desc(const void *a, const void *b) {
    return by_int(b, a);
}

static int by_name(const void *a, const void *b) {
    return strcmp(*(const char * const *)a, *(const char * const *)b);
}

static int by_key(const void *a, const void *b) {
    const struct person *left = (const struct person *)a;
    const struct person *right = (const struct person *)b;
    return left->key - right->key;
}

static void show_ints(const char *tag, int *values, int count) {
    int i;
    printf("%s", tag);
    for (i = 0; i < count; i++)
        printf("%d ", values[i]);
    printf("\n");
}

int main(void) {
    int values[10];
    int sorted[8];
    int empty[1];
    const char *names[5];
    struct person people[4];
    int key, i;
    int *found;
    struct person want;
    struct person *who;
    const char **name_found;
    const char *name_key;

    /* A shuffled array, including duplicates and negatives. */
    values[0] = 5;  values[1] = -3; values[2] = 9;  values[3] = 0;  values[4] = 5;
    values[5] = -8; values[6] = 42; values[7] = 1;  values[8] = -3; values[9] = 7;
    qsort(values, 10, sizeof(int), by_int);
    show_ints("asc:", values, 10);

    qsort(values, 10, sizeof(int), by_int_desc);
    show_ints("desc:", values, 10);

    /* Already sorted, reversed, and all equal. */
    for (i = 0; i < 8; i++) sorted[i] = i;
    qsort(sorted, 8, sizeof(int), by_int);
    show_ints("already:", sorted, 8);
    for (i = 0; i < 8; i++) sorted[i] = 8 - i;
    qsort(sorted, 8, sizeof(int), by_int);
    show_ints("reversed:", sorted, 8);
    for (i = 0; i < 8; i++) sorted[i] = 4;
    qsort(sorted, 8, sizeof(int), by_int);
    show_ints("equal:", sorted, 8);

    /* Counts that have nothing to do. */
    empty[0] = 7;
    qsort(empty, 0, sizeof(int), by_int);
    qsort(empty, 1, sizeof(int), by_int);
    printf("small:%d\n", empty[0]);

    /* An array of pointers, sorted by what they point at. */
    names[0] = "pear"; names[1] = "apple"; names[2] = "fig";
    names[3] = "date"; names[4] = "cherry";
    qsort(names, 5, sizeof(const char *), by_name);
    printf("names:");
    for (i = 0; i < 5; i++) printf("%s ", names[i]);
    printf("\n");

    /* Structures, sorted by a field. */
    people[0].key = 30; people[0].name = "thirty";
    people[1].key = 10; people[1].name = "ten";
    people[2].key = 40; people[2].name = "forty";
    people[3].key = 20; people[3].name = "twenty";
    qsort(people, 4, sizeof(struct person), by_key);
    printf("people:");
    for (i = 0; i < 4; i++) printf("%d=%s ", people[i].key, people[i].name);
    printf("\n");

    /* bsearch over the ascending array, without duplicates. */
    for (i = 0; i < 8; i++) sorted[i] = i * 10;
    printf("find:");
    for (i = 0; i < 8; i++) {
        key = i * 10;
        found = (int *)bsearch(&key, sorted, 8, sizeof(int), by_int);
        printf("%d ", found != NULL && *found == key);
    }
    printf("\n");
    key = 35;
    printf("miss:%d ", bsearch(&key, sorted, 8, sizeof(int), by_int) == NULL);
    key = -1;
    printf("%d ", bsearch(&key, sorted, 8, sizeof(int), by_int) == NULL);
    key = 100;
    printf("%d ", bsearch(&key, sorted, 8, sizeof(int), by_int) == NULL);
    key = 0;
    printf("%d\n", bsearch(&key, sorted, 0, sizeof(int), by_int) == NULL);

    /* bsearch over the structures and the names. */
    want.key = 20;
    who = (struct person *)bsearch(&want, people, 4, sizeof(struct person), by_key);
    printf("who:%s\n", who != NULL ? who->name : "none");
    name_key = "date";
    name_found = (const char **)bsearch(&name_key, names, 5,
        sizeof(const char *), by_name);
    printf("name:%s\n", name_found != NULL ? *name_found : "none");

    /* A larger array, filled from a generator so that both compilers sort the
     * same input.  A recursive sort would put its depth at the mercy of this
     * data and VIG has a fixed call stack; a shell sort has no depth at all, so
     * what is being checked here is only the ordering. */
    {
        static int many[300];
        unsigned seed = 12345u;
        int ordered = 1;
        int total = 0;

        for (i = 0; i < 300; i++) {
            seed = seed * 1103515245u + 12345u;
            many[i] = (int)((seed >> 16) % 1000u);
        }
        qsort(many, 300, sizeof(int), by_int);
        for (i = 1; i < 300; i++)
            if (many[i - 1] > many[i])
                ordered = 0;
        for (i = 0; i < 300; i++)
            total += many[i];
        printf("big:%d %d %d %d\n", ordered, many[0], many[299], total);
    }
    return 0;
}
