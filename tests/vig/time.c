/* Calendar time.
 *
 * The section from the epoch onwards was checked against the system C library:
 * the same source compiled natively produces the same bytes.  The pre-epoch
 * section could not be, because the Windows CRT's gmtime refuses a negative
 * time_t and returns NULL -- those values were checked against an independent
 * reference instead, and 1969-12-31 really was a Wednesday.
 *
 * Nothing here calls `time', so this program carries no foreign import and runs
 * on any VIG host.  `time' itself is exercised by ffi_clock.c.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

/* The corners: the epoch, a leap day, a leap century, the day after a leap day,
 * the end of a leap year, and the last second a 32-bit time_t can hold. */
static const int stamps[] = {
    0, 86399, 86400,
    951782400, 946684800, 978220800,
    1078012800, 1709164800, 2147483647
};

/* Before the epoch, where a truncating division needs care. */
static const int early[] = { -1, -86400, -86401, -31536000 };

int main(void) {
    char buffer[64];
    char small[8];
    int i, n;
    time_t stamp, again;
    struct tm parts;
    struct tm *broken;

    n = sprintf(buffer, "%d/%s/%c/%05d", 42, "mid", 'Z', -7);
    printf("sprintf:[%s] %d\n", buffer, n);
    n = snprintf(small, sizeof small, "%s", "abcdefghij");
    printf("snprintf:[%s] %d\n", small, n);
    n = snprintf(small, sizeof small, "%d", 42);
    printf("snprintf2:[%s] %d\n", small, n);

    for (i = 0; i < (int)(sizeof stamps / sizeof stamps[0]); i++) {
        stamp = (time_t)stamps[i];
        strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S %a %b yday=%j",
            gmtime(&stamp));
        printf("%s\n", buffer);
    }

    printf("asc:%s", asctime(gmtime(&stamps[4])));

    for (i = 0; i < 4; i++) {
        stamp = (time_t)(i * 21600);
        strftime(buffer, sizeof buffer, "%I%p ", gmtime(&stamp));
        printf("%s", buffer);
    }
    printf("\n");

    /* Before the epoch. */
    for (i = 0; i < (int)(sizeof early / sizeof early[0]); i++) {
        stamp = (time_t)early[i];
        strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S %a yday=%j",
            gmtime(&stamp));
        printf("early:%s\n", buffer);
    }

    /* mktime puts a time back together, and every one of these has to come back
     * as the number it started from. */
    for (i = 0; i < (int)(sizeof stamps / sizeof stamps[0]); i++) {
        stamp = (time_t)stamps[i];
        parts = *gmtime(&stamp);
        again = mktime(&parts);
        printf("%d", again == stamp);
    }
    for (i = 0; i < (int)(sizeof early / sizeof early[0]); i++) {
        stamp = (time_t)early[i];
        parts = *gmtime(&stamp);
        again = mktime(&parts);
        printf("%d", again == stamp);
    }
    printf(" roundtrip\n");

    /* mktime also normalises a field that is out of range, which is what makes
     * it useful for date arithmetic: the 32nd of January is the 1st of
     * February, and month 12 is January of the next year. */
    parts.tm_year = 100; parts.tm_mon = 0; parts.tm_mday = 32;
    parts.tm_hour = 0; parts.tm_min = 0; parts.tm_sec = 0; parts.tm_isdst = 0;
    mktime(&parts);
    strftime(buffer, sizeof buffer, "%Y-%m-%d", &parts);
    printf("norm:%s ", buffer);

    parts.tm_year = 100; parts.tm_mon = 12; parts.tm_mday = 1;
    parts.tm_hour = 25; parts.tm_min = 0; parts.tm_sec = 0; parts.tm_isdst = 0;
    mktime(&parts);
    strftime(buffer, sizeof buffer, "%Y-%m-%d %H", &parts);
    printf("%s\n", buffer);

    /* difftime, and the leap-year rule that catches naive implementations:
     * 2000 is a leap year and 1900 was not. */
    printf("diff:%d %d\n", difftime(86400, 0), difftime(0, 86400));
    stamp = 951782400;              /* 2000-02-29 */
    broken = gmtime(&stamp);
    printf("leap:%d-%02d-%02d ", broken->tm_year + 1900, broken->tm_mon + 1,
        broken->tm_mday);
    parts.tm_year = 100; parts.tm_mon = 1; parts.tm_mday = 29;
    parts.tm_hour = 0; parts.tm_min = 0; parts.tm_sec = 0; parts.tm_isdst = 0;
    printf("%d\n", mktime(&parts) == 951782400);

    /* A date a 32-bit time_t cannot hold has no answer.  C asks for -1; VIG
     * traps on signed overflow, so this only works because mktime checks the
     * range before it multiplies. */
    parts.tm_year = 200; parts.tm_mon = 0; parts.tm_mday = 1;
    parts.tm_hour = 0; parts.tm_min = 0; parts.tm_sec = 0; parts.tm_isdst = 0;
    printf("range2100:%d ", mktime(&parts) == -1);
    parts.tm_year = -100; parts.tm_mon = 0; parts.tm_mday = 1;
    parts.tm_hour = 0; parts.tm_min = 0; parts.tm_sec = 0; parts.tm_isdst = 0;
    printf("range1800:%d\n", mktime(&parts) == -1);

    /* clock has nothing to read, and C says to report that with -1. */
    printf("clock:%d\n", (int)clock());
    return 0;
}
