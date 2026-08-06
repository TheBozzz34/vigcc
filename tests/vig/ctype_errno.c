/* <ctype.h> and <errno.h>.
 *
 * The classification is checked against the system C library over every value a
 * character can take, plus EOF.  Each result is normalised with `!!' first: C
 * promises only "nonzero" for true, and some C libraries return other nonzero
 * values, so comparing the raw numbers would be comparing something C does not
 * define.
 *
 * The errno section is not oracle-compared.  Nothing in a host C library fails
 * the way this heap does, and the wording of a strerror message is left to the
 * implementation by C.
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One line per class, 257 characters wide: EOF, then 0 through 255. */
static void classify(const char *name, int (*test)(int)) {
    int c;

    printf("%-8s", name);
    printf("%d", !!test(EOF));
    for (c = 0; c <= 255; c++)
        printf("%d", !!test(c));
    printf("\n");
}

int main(void) {
    int c, changed = 0;
    void *block;

    classify("alnum", isalnum);
    classify("alpha", isalpha);
    classify("cntrl", iscntrl);
    classify("digit", isdigit);
    classify("graph", isgraph);
    classify("lower", islower);
    classify("print", isprint);
    classify("punct", ispunct);
    classify("space", isspace);
    classify("upper", isupper);
    classify("xdigit", isxdigit);

    /* The two conversions, over the same range.  Only the letters move, and a
     * value that is not a letter comes back unchanged. */
    for (c = 0; c <= 255; c++)
        if (tolower(c) != c || toupper(c) != c)
            changed++;
    printf("changed:%d\n", changed);
    printf("eof:%d %d\n", tolower(EOF) == EOF, toupper(EOF) == EOF);
    printf("case:%c%c %c%c %d %d\n", tolower('Q'), toupper('q'),
        tolower('7'), toupper('7'), tolower('a') == 'a', toupper('Z') == 'Z');

    /* A round trip through both conversions returns every letter to itself. */
    changed = 0;
    for (c = 'a'; c <= 'z'; c++)
        if (tolower(toupper(c)) != c)
            changed++;
    for (c = 'A'; c <= 'Z'; c++)
        if (toupper(tolower(c)) != c)
            changed++;
    printf("roundtrip:%d\n", changed);

    /* errno starts at zero and stays where it is put. */
    printf("errno:%d\n", errno);
    errno = EINVAL;
    printf("set:%d %s\n", errno == EINVAL, strerror(errno));
    errno = 0;

    /* A refused allocation says why.  Nothing clears errno first, so the
     * success below must leave the zero it started with. */
    block = malloc(16);
    printf("ok:%d errno:%d\n", block != NULL, errno);
    free(block);

    /* A request larger than the heap could ever be. */
    block = malloc(VIG_HEAP_SIZE + 1);
    printf("refused:%d errno:%d [%s]\n", block == NULL, errno == ENOMEM,
        strerror(errno));

    /* And a request the heap simply has no room left for, which is a different
     * path through malloc and needs its own check. */
    {
        void *held[128];
        int taken = 0;
        int i;

        errno = 0;
        for (i = 0; i < 128; i++) {
            held[i] = malloc(1024);
            if (held[i] == NULL)
                break;
            taken++;
        }
        printf("exhausted:%d errno:%d\n", taken > 0 && taken < 128,
            errno == ENOMEM);
        for (i = 0; i < taken; i++)
            free(held[i]);
    }

    printf("messages:[%s][%s][%s]\n", strerror(0), strerror(ERANGE),
        strerror(9999));
    return 0;
}
