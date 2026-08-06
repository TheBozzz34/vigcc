/* printf, written in C on top of the one-byte-output intrinsic.
 *
 * Every line below was checked against the system printf: the same source
 * compiled natively produces the same bytes.  The one exception is the invalid
 * conversion at the end, which C leaves undefined -- this printf echoes what it
 * was given so a mistake in a format string is visible rather than silent.
 */
#include <stdio.h>

int main(void) {
    int i;

    printf("plain text\n");
    printf("d:%d %d %d %d\n", 0, 42, -7, -2147483647 - 1);
    printf("u:%u %u %u\n", 0, 2147483648u, 4294967295u);
    printf("x:%x %X %x %X\n", 255, 255, 0, 3735928559u);
    printf("c:%c%c%c\n", 'a', 'b', 'c');
    printf("s:%s|%s|%s|\n", "hello", "", "z");
    printf("pct:%%\n");
    printf("w:[%5d][%-5d][%05d]\n", 42, 42, 42);
    printf("w:[%5d][%-5d][%05d]\n", -42, -42, -42);
    printf("w:[%1d][%10d][%-10d]\n", 5, 5, 5);
    printf("w:[%6s][%-6s][%3s]\n", "ab", "ab", "abcdef");
    printf("mix:%d/%s/%c/%x\n", 7, "mid", 'Z', 48879);
    for (i = 0; i < 4; i++)
        printf("%d:%x ", i * 100, i * 100);
    printf("\n");
    /* printf returns the number of characters it wrote. */
    printf("ret=%d\n", printf("abc\n"));
    puts("puts adds its own newline");
    putchar('!');
    putchar('\n');
    /* Undefined in C; this implementation shows the specification unchanged. */
    printf("bad:%q\n", 1);
    return 0;
}
