/* Assertions that hold, and NDEBUG turning them off.
 *
 * <assert.h> has no include guard: C says `assert' is redefined at each
 * inclusion according to whether NDEBUG is set then, so a program can turn
 * assertions off for one part of itself and on again for another.  This file
 * exercises exactly that, which is why it includes the header three times.
 */
#include <assert.h>
#include <stdio.h>

static int calls;

static int watched(int value) {
    calls++;
    return value;
}

int main(void) {
    calls = 0;
    assert(1);
    assert(1 == 1);
    assert("a pointer is true");
    printf("held:1\n");

    /* An assertion that holds still evaluates its condition exactly once. */
    assert(watched(1));
    printf("evaluated:%d\n", calls);

/* Off for this stretch. */
#define NDEBUG
#include <assert.h>
    assert(0);
    assert(watched(0));
    printf("disabled:%d calls:%d\n", 1, calls);

/* And on again. */
#undef NDEBUG
#include <assert.h>
    assert(1);
    assert(watched(1));
    printf("reenabled:%d calls:%d\n", 1, calls);
    return 0;
}
