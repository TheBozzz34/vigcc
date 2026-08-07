/* Behaviour that a host C library cannot be compared against: this heap has a
 * fixed size, so running it out is a defined and reachable thing to do.
 *
 * The two coalescing rounds free in opposite orders on purpose.  malloc splits
 * blocks off the end of the free run, so successive blocks come back at
 * descending addresses; freeing them in ascending index order therefore only
 * ever meets a free *upper* neighbour, and the lower-neighbour branch of free
 * is never taken.  Freeing in descending index order takes the other one.  A
 * round in one direction alone passes with half of free deleted.
 */
#include <stdio.h>
#include <stdlib.h>

/* The heap belongs to `runtime/stdlib.c', so VIG_HEAP_SIZE is what that object
 * was built with and not something this program can choose.  BLOCKS therefore
 * has to be large enough to exhaust whatever that is: each request costs its
 * size rounded up to a block plus one block of header, so a heap of N bytes
 * holds fewer than N/BLOCK_BYTES of them. */
#define BLOCKS (VIG_HEAP_SIZE / BLOCK_BYTES)
#define BLOCK_BYTES 64
#define BIG_BYTES 2048

/* Take blocks until the heap is empty, then give them all back in the order
 * `descending' asks for, and report whether one large block can then be had.
 * That is only true if the small ones were rejoined into a single run.
 *
 * The heap has to be taken to exhaustion for this to prove anything: while any
 * large free run is left over, the big request is satisfied from that and says
 * nothing about whether the small blocks were rejoined at all. */
static int round_trip(int descending) {
    void *blocks[BLOCKS];
    void *big;
    int i, taken = 0;

    for (i = 0; i < BLOCKS; i++) {
        blocks[i] = malloc(BLOCK_BYTES);
        if (blocks[i] == NULL)
            break;
        taken++;
    }
    if (taken == 0 || taken == BLOCKS)
        return 0;	/* the heap did not run out: the round proves nothing */
    if (descending) {
        i = taken;
        while (i > 0) {
            i--;
            free(blocks[i]);
        }
    } else {
        for (i = 0; i < taken; i++)
            free(blocks[i]);
    }

    big = malloc(BIG_BYTES);
    if (big == NULL)
        return 0;
    free(big);
    return 1;
}

int main(void) {
    void *big;

    /* A request larger than the whole heap fails rather than trapping. */
    printf("toobig:%d\n", malloc(VIG_HEAP_SIZE + 1) == NULL);

    printf("coalesce_up:%d\n", round_trip(0));
    printf("coalesce_down:%d\n", round_trip(1));

    /* The heap survives both rounds and is still whole. */
    big = malloc(2048);
    printf("intact:%d\n", big != NULL);
    free(big);

    printf("before exit\n");
    exit(0);
    printf("after exit\n");
    return 0;
}
