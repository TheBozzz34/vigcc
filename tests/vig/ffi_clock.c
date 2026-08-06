/* This program asks for the host clock, which the header leaves out by default. */
#define VIG_CLOCK
/* `time', which is the one part of <time.h> that leaves the VM.
 *
 * A program that calls it carries a foreign import naming a library of the host
 * that will run it, so this program is not portable across VIG hosts the way
 * every other case in this corpus is.  See ABI.md.
 *
 * Nothing here prints the clock: it changes every run.  What is checked is that
 * the call reaches the host, comes back, and gives a value that agrees with the
 * rest of the header.
 */
#include <stdio.h>
#include <time.h>

int main(void) {
    time_t now, stored, later;
    struct tm *parts;
    char buffer[64];

    now = time(NULL);
    printf("read:%d\n", now > 0);

    /* The out-parameter form gives the same answer as the return value. */
    stored = 0;
    later = time(&stored);
    printf("store:%d\n", stored == later);

    /* The clock does not run backwards between two reads. */
    printf("forward:%d\n", time(NULL) >= now);

    /* And the value means what the rest of the header thinks it means: some
     * time after 2020 and before this time_t runs out in 2038. */
    printf("range:%d\n", now > 1577836800 && now < 2147483647);

    parts = gmtime(&now);
    printf("year:%d\n", parts->tm_year + 1900 >= 2020);
    printf("month:%d day:%d hour:%d\n",
        parts->tm_mon >= 0 && parts->tm_mon <= 11,
        parts->tm_mday >= 1 && parts->tm_mday <= 31,
        parts->tm_hour >= 0 && parts->tm_hour <= 23);

    /* And it round-trips through the conversions like any other time. */
    strftime(buffer, sizeof buffer, "%Y-%m-%d", parts);
    printf("format:%d\n", (int)strlen(buffer) == 10);
    return 0;
}
