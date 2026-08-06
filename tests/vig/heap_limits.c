/* Behaviour that a host C library cannot be compared against: this heap has a
 * fixed size, so running it out is a defined and reachable thing to do. */
#define VIG_HEAP_SIZE 4096
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    void *blocks[64];
    void *big;
    int i, taken;

    /* A request larger than the whole heap fails rather than trapping. */
    printf("toobig:%d\n", malloc(VIG_HEAP_SIZE + 1) == NULL);

    /* Fill the heap.  The count is not the interesting part; that it stops
     * rather than running off the end is. */
    taken = 0;
    for (i = 0; i < 64; i++) {
        blocks[i] = malloc(64);
        if (blocks[i] == NULL)
            break;
        taken++;
    }
    printf("filled:%d exhausted:%d\n", taken > 0, malloc(64) == NULL);

    /* Give it all back.  A block bigger than any one of them can then be had,
     * which is only true if free rejoined them into one run. */
    for (i = 0; i < taken; i++)
        free(blocks[i]);
    big = malloc(2048);
    printf("coalesced:%d\n", big != NULL);
    free(big);

    /* And the heap is still usable afterwards. */
    big = malloc(2048);
    printf("again:%d\n", big != NULL);

    printf("before exit\n");
    exit(0);
    printf("after exit\n");
    return 0;
}
