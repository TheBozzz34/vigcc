/* Allocation, checked against the system C library: the same source compiled
 * natively produces the same bytes.  Addresses are never printed, because
 * those are the one thing that legitimately differs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    int value;
    struct node *next;
};

int main(void) {
    char *a, *b, *c;
    int *numbers;
    struct node *head, *item;
    int i, total;

    /* A block is usable, and what is written to it stays written. */
    a = (char *)malloc(16);
    printf("a:%d\n", a != NULL);
    strcpy(a, "first");
    printf("a:[%s]\n", a);

    /* Two live blocks do not overlap. */
    b = (char *)malloc(16);
    strcpy(b, "second");
    printf("ab:[%s][%s] %d\n", a, b, a != b);

    /* calloc zeroes what it hands back. */
    numbers = (int *)calloc(4, sizeof(int));
    printf("calloc:%d %d %d %d %d\n", numbers != NULL,
        numbers[0], numbers[1], numbers[2], numbers[3]);
    for (i = 0; i < 4; i++)
        numbers[i] = i * 11;
    printf("filled:%d %d %d %d\n", numbers[0], numbers[1], numbers[2], numbers[3]);

    /* realloc keeps the contents it already had. */
    c = (char *)malloc(8);
    strcpy(c, "grow");
    c = (char *)realloc(c, 64);
    printf("realloc:[%s] %d\n", c, c != NULL);
    strcat(c, "n bigger");
    printf("realloc:[%s]\n", c);

    /* A freed block may be handed out again; either way the heap stays sound. */
    free(b);
    b = (char *)malloc(16);
    strcpy(b, "third");
    printf("reuse:[%s][%s]\n", a, b);

    /* A list built out of separately allocated nodes. */
    head = NULL;
    for (i = 5; i >= 1; i--) {
        item = (struct node *)malloc(sizeof(struct node));
        item->value = i * i;
        item->next = head;
        head = item;
    }
    total = 0;
    for (item = head; item != NULL; item = item->next) {
        printf("%d ", item->value);
        total += item->value;
    }
    printf("total:%d\n", total);
    while (head != NULL) {
        item = head->next;
        free(head);
        head = item;
    }

    /* free(NULL) does nothing, and realloc(NULL, n) is malloc. */
    free(NULL);
    a = (char *)realloc(NULL, 8);
    strcpy(a, "via re");
    printf("edge:[%s] %d\n", a, malloc(0) == NULL);

    printf("abs:%d %d %d\n", abs(-5), abs(5), abs(0));
    printf("atoi:%d %d %d %d\n", atoi("42"), atoi("-17"), atoi("  8x"), atoi("z"));
    return 0;
}
