#ifndef VIG_TIME_H
#define VIG_TIME_H

#include <stddef.h>

typedef int time_t;
typedef int clock_t;

#define CLOCKS_PER_SEC 1

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct tm *gmtime(const time_t *when);
struct tm *localtime(const time_t *when);
time_t mktime(struct tm *when);
time_t difftime(time_t later, time_t earlier);
char *asctime(const struct tm *when);
char *ctime(const time_t *when);
size_t strftime(char *out, size_t limit, const char *format, const struct tm *when);
time_t time(time_t *store);
clock_t clock(void);

#endif
