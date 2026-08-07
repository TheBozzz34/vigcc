#ifndef VIG_TIME_H
#define VIG_TIME_H

#include <stddef.h>

typedef int time_t;
typedef int clock_t;

#define CLOCKS_PER_SEC 1

struct tm {
	int tm_sec;	/* 0..60, the extra second being a leap second */
	int tm_min;	/* 0..59 */
	int tm_hour;	/* 0..23 */
	int tm_mday;	/* 1..31 */
	int tm_mon;	/* 0..11 */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* 0..6, Sunday is 0 */
	int tm_yday;	/* 0..365 */
	int tm_isdst;	/* always 0: there is no timezone information */
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
