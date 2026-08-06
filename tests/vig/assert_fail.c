/* A failing assertion: it names the expression and stops the program.
 *
 * The file in the message is the last component of __FILE__ and not the whole
 * path, so this output is the same wherever the source tree sits.  The line
 * after the assertion never runs.
 */
#include <assert.h>
#include <stdio.h>

int main(void) {
    int count = 2;

    printf("before\n");
    assert(count == 2);
    printf("passed\n");
    assert(count == 3);
    printf("never reached\n");
    return 0;
}
