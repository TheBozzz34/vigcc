/* The string and memory functions.
 *
 * Every line below was checked against the system library: the same source
 * compiled natively produces the same bytes.  The comparisons matter most --
 * `cmpu' and the last `mcmp' are positive only if the bytes are read as
 * unsigned char, which is what C requires and what reading them as a plain
 * char would get wrong.
 */
#include <stdio.h>
#include <string.h>

static char buf[64];
static char b2[64];

static int sign(int v) { return v < 0 ? -1 : (v > 0 ? 1 : 0); }

int main(void) {
    char local[16];
    int i;

    printf("len:%d %d %d\n", (int)strlen(""), (int)strlen("a"), (int)strlen("hello"));

    strcpy(buf, "hello");
    printf("cpy:[%s]\n", buf);
    strcat(buf, ", world");
    printf("cat:[%s]\n", buf);
    strncat(buf, "!!!!!", 2);
    printf("ncat:[%s]\n", buf);

    strncpy(b2, "abc", 6);
    printf("ncpy:%d%d%d%d%d%d\n", b2[0], b2[1], b2[2], b2[3], b2[4], b2[5]);
    strncpy(b2, "abcdef", 3);
    printf("ncpy2:%c%c%c\n", b2[0], b2[1], b2[2]);

    printf("cmp:%d %d %d %d\n", sign(strcmp("a", "a")), sign(strcmp("a", "b")),
        sign(strcmp("b", "a")), sign(strcmp("abc", "ab")));
    printf("cmpu:%d\n", sign(strcmp("\200", "\001")));
    printf("ncmp:%d %d %d\n", sign(strncmp("abc", "abd", 2)),
        sign(strncmp("abc", "abd", 3)), sign(strncmp("", "", 5)));

    printf("chr:%d %d\n", strchr("hello", 'z') == NULL, *strchr("hello", '\0') == 0);
    printf("chr2:[%s][%s]\n", strchr("hello", 'l'), strrchr("hello", 'l'));
    printf("str:[%s] %d [%s]\n", strstr("hello world", "o w"),
        strstr("abc", "xyz") == NULL, strstr("abc", ""));

    printf("spn:%d %d %d\n", (int)strspn("abcde", "abc"), (int)strspn("xyz", "abc"),
        (int)strcspn("abcde", "dc"));
    printf("pbrk:[%s] %d\n", strpbrk("hello", "lo"), strpbrk("abc", "xyz") == NULL);

    memset(local, 'z', 5);
    local[5] = '\0';
    printf("set:[%s]\n", local);
    memcpy(local, "ab", 2);
    printf("cpyM:[%s]\n", local);
    printf("mcmp:%d %d %d\n", sign(memcmp("abc", "abc", 3)),
        sign(memcmp("abc", "abd", 3)), sign(memcmp("\200x", "\001x", 1)));
    printf("mchr:%d %d\n", memchr("abc", 'b', 3) != NULL, memchr("abc", 'z', 3) == NULL);

    strcpy(buf, "0123456789");
    memmove(buf + 2, buf, 5);
    printf("mvR:[%s]\n", buf);
    strcpy(buf, "0123456789");
    memmove(buf, buf + 2, 5);
    printf("mvL:[%s]\n", buf);

    local[0] = 'Q';
    memset(local, 'q', 0);
    memcpy(local, "zz", 0);
    printf("zero:%d %d %c\n", memcmp("", "", 0), (int)strlen(""), local[0]);

    for (i = 0; i < 3; i++) {
        strcpy(local, "ab");
        local[i % 2] = 'X';
        printf("%s ", local);
    }
    printf("\n");
    return 0;
}
