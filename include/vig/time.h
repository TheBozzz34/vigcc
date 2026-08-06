#ifndef VIG_TIME_H
#define VIG_TIME_H

/* Calendar time.
 *
 * Everything here but `time' and `clock' is arithmetic, and runs on any host.
 * Those two need a clock, which the VM does not have: they reach the one on the
 * machine through `#pragma vig import'.
 *
 * **The clock is not here unless it is asked for.**  Define VIG_CLOCK before
 * including this file to get a working `time'.  Without it, `time' reports that
 * the calendar time is unavailable, which C explicitly allows.
 *
 * The reason is that a foreign import names a library of the system that will
 * run the program, and nothing in the bytecode makes that name portable -- see
 * ABI.md.  There is also no dead-code elimination here, so a defined `time' is
 * an emitted `time', and its import would be carried by every program that
 * included this file whether it read the clock or not.  Asking for it is
 * therefore a decision the program makes rather than one the header makes for
 * it.
 *
 * The host is chosen from the machine `vigcc' was built on.  Define VIG_HOST_
 * WINDOWS, VIG_HOST_MACOS or VIG_HOST_POSIX yourself to compile for another.
 *
 * `time_t' is a signed 32-bit count of seconds since 1970-01-01T00:00:00Z, so
 * it runs out in 2038.  Every integer in this C subset is 32 bits, so there is
 * no wider one to use.
 *
 * All conversions are UTC.  There is no timezone database and no way to reach
 * the host's, so `localtime' is `gmtime' and `tm_isdst' is always 0.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

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

#define VIG_SECONDS_PER_DAY 86400

static int vig_leap(int year) {
	return (year%4 == 0 && year%100 != 0) || year%400 == 0;
}

static int vig_month_days(int year, int month) {
	static const int lengths[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

	if (month == 1 && vig_leap(year))
		return 29;
	return lengths[month];
}

/* The number of days from 1970-01-01 to the first of `year'.  Counting a year
 * at a time is enough: `time_t' spans 1901 to 2038. */
static int vig_days_before_year(int year) {
	int days = 0;
	int y;

	if (year >= 1970) {
		for (y = 1970; y < year; y++)
			days += vig_leap(y) ? 366 : 365;
	} else {
		for (y = year; y < 1970; y++)
			days -= vig_leap(y) ? 366 : 365;
	}
	return days;
}

static int vig_days_before_month(int year, int month) {
	int days = 0;
	int m;

	for (m = 0; m < month; m++)
		days += vig_month_days(year, m);
	return days;
}

/* Break a time down into its parts.  The result is a single static structure,
 * as C says it may be, so a second call overwrites the first. */
struct tm *gmtime(const time_t *when) {
	static struct tm broken;
	int days, seconds, year;

	if (when == NULL)
		return NULL;
	days = *when / VIG_SECONDS_PER_DAY;
	seconds = *when % VIG_SECONDS_PER_DAY;
	/* C truncates a division towards zero, and a time before the epoch needs
	 * it to go the other way so that the seconds within the day stay positive. */
	if (seconds < 0) {
		seconds += VIG_SECONDS_PER_DAY;
		days--;
	}

	broken.tm_hour = seconds / 3600;
	broken.tm_min = seconds / 60 % 60;
	broken.tm_sec = seconds % 60;

	/* 1970-01-01 was a Thursday. */
	broken.tm_wday = (days + 4) % 7;
	if (broken.tm_wday < 0)
		broken.tm_wday += 7;

	year = 1970;
	while (days < 0) {
		year--;
		days += vig_leap(year) ? 366 : 365;
	}
	for (;;) {
		int length = vig_leap(year) ? 366 : 365;
		if (days < length)
			break;
		days -= length;
		year++;
	}
	broken.tm_year = year - 1900;
	broken.tm_yday = days;

	broken.tm_mon = 0;
	while (days >= vig_month_days(year, broken.tm_mon)) {
		days -= vig_month_days(year, broken.tm_mon);
		broken.tm_mon++;
	}
	broken.tm_mday = days + 1;
	broken.tm_isdst = 0;
	return &broken;
}

/* There is no timezone information, so local time is UTC. */
struct tm *localtime(const time_t *when) {
	return gmtime(when);
}

/* Put the parts back together, and put `when' itself back in order: a field
 * outside its range carries into the next, which is what makes `mktime' useful
 * for date arithmetic. */
time_t mktime(struct tm *when) {
	int year, month, days;
	time_t result;

	if (when == NULL)
		return (time_t)-1;

	/* Months first, because a month out of range changes which year the day
	 * count belongs to. */
	year = when->tm_year + 1900;
	month = when->tm_mon;
	year += month / 12;
	month = month % 12;
	if (month < 0) {
		month += 12;
		year--;
	}

	days = vig_days_before_year(year) + vig_days_before_month(year, month)
		+ when->tm_mday - 1;

	/* A date outside what a 32-bit `time_t' can hold has no answer, and C asks
	 * for (time_t)-1 rather than a wrong one.  The test comes before the
	 * arithmetic because VIG traps on signed overflow: computing the value
	 * first and checking it afterwards would kill the program instead of
	 * returning.  24855 days is the last whole day the type reaches. */
	if (days > 24855 || days < -24856)
		return (time_t)-1;
	{
		/* Accumulated as unsigned so that a wildly out-of-range field wraps
		 * rather than trapping on the way to the check below. */
		unsigned total = (unsigned)days * 86400u
			+ (unsigned)when->tm_hour * 3600u
			+ (unsigned)when->tm_min * 60u
			+ (unsigned)when->tm_sec;

		if (days >= 0 && total > 2147483647u)
			return (time_t)-1;
		result = (time_t)total;
	}

	/* Hand back the normalised form, as C requires. */
	{
		time_t copy = result;
		struct tm *tidy = gmtime(&copy);
		if (tidy != NULL)
			*when = *tidy;
	}
	return result;
}

/* C declares this as returning `double'.  This subset has no floating point, so
 * it returns whole seconds instead -- a deliberate deviation, and the only one
 * in this header.  Nothing is lost: a program here could not hold a `double'
 * result anyway, and every difference a 32-bit `time_t' can express is a whole
 * number of seconds that an `int' holds exactly. */
time_t difftime(time_t later, time_t earlier) {
	return later - earlier;
}

static const char vig_day_names[7][4] = {
	"Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
static const char vig_month_names[12][4] = {
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

/* `asctime' writes a fixed 26-character form, into a single static buffer. */
char *asctime(const struct tm *when) {
	static char text[32];

	if (when == NULL)
		return NULL;
	sprintf(text, "%s %s %2d %02d:%02d:%02d %d\n",
		vig_day_names[when->tm_wday % 7], vig_month_names[when->tm_mon % 12],
		when->tm_mday, when->tm_hour, when->tm_min, when->tm_sec,
		when->tm_year + 1900);
	return text;
}

char *ctime(const time_t *when) {
	return asctime(gmtime(when));
}

size_t strftime(char *out, size_t limit, const char *format,
	const struct tm *when) {
	size_t written = 0;
	char piece[32];
	const char *text;
	size_t length, i;
	char conversion;

	if (out == NULL || when == NULL || limit == 0)
		return 0;
	while (*format != '\0') {
		if (*format != '%') {
			if (written + 1 >= limit)
				return 0;
			out[written] = *format;
			written++;
			format++;
			continue;
		}
		format++;
		conversion = *format;
		if (conversion != '\0')
			format++;
		text = piece;
		piece[0] = '\0';

		switch (conversion) {
		case 'Y': sprintf(piece, "%d", when->tm_year + 1900); break;
		case 'y': sprintf(piece, "%02d", (when->tm_year + 1900) % 100); break;
		case 'm': sprintf(piece, "%02d", when->tm_mon + 1); break;
		case 'd': sprintf(piece, "%02d", when->tm_mday); break;
		case 'e': sprintf(piece, "%2d", when->tm_mday); break;
		case 'H': sprintf(piece, "%02d", when->tm_hour); break;
		case 'M': sprintf(piece, "%02d", when->tm_min); break;
		case 'S': sprintf(piece, "%02d", when->tm_sec); break;
		case 'j': sprintf(piece, "%03d", when->tm_yday + 1); break;
		case 'a': text = vig_day_names[when->tm_wday % 7]; break;
		case 'b':
		case 'h': text = vig_month_names[when->tm_mon % 12]; break;
		case 'p': text = when->tm_hour < 12 ? "AM" : "PM"; break;
		case 'I': {
			int hour = when->tm_hour % 12;
			sprintf(piece, "%02d", hour == 0 ? 12 : hour);
			break;
		}
		case 'D': sprintf(piece, "%02d/%02d/%02d", when->tm_mon + 1,
			when->tm_mday, (when->tm_year + 1900) % 100); break;
		case 'F': sprintf(piece, "%d-%02d-%02d", when->tm_year + 1900,
			when->tm_mon + 1, when->tm_mday); break;
		case 'T': sprintf(piece, "%02d:%02d:%02d", when->tm_hour,
			when->tm_min, when->tm_sec); break;
		case 'n': text = "\n"; break;
		case 't': text = "\t"; break;
		case '%': text = "%"; break;
		default:
			/* An unknown conversion is written as it was given, which is what
			 * this toolchain's printf does with one. */
			piece[0] = '%';
			piece[1] = conversion;
			piece[2] = '\0';
			break;
		}

		length = strlen(text);
		if (written + length >= limit)
			return 0;
		for (i = 0; i < length; i++)
			out[written + i] = text[i];
		written += length;
	}
	if (written >= limit)
		return 0;
	out[written] = '\0';
	return written;
}

/* The clock ------------------------------------------------------------------
 *
 * This is the only part that leaves the VM, and the only part that ties a
 * compiled program to one kind of host.
 */

#if defined(VIG_CLOCK) && defined(VIG_HOST_WINDOWS)

/* Windows has no 32-bit `time' in kernel32, so the broken-down form is asked
 * for and put back together with the arithmetic above. */
struct vig_systemtime {
	unsigned short year, month, weekday, day;
	unsigned short hour, minute, second, milliseconds;
};

#pragma vig import vig_get_system_time, "kernel32.dll", "GetSystemTime"
extern void vig_get_system_time(struct vig_systemtime *out);

time_t time(time_t *store) {
	struct vig_systemtime now;
	struct tm parts;
	time_t result;

	vig_get_system_time(&now);
	parts.tm_year = (int)now.year - 1900;
	parts.tm_mon = (int)now.month - 1;
	parts.tm_mday = (int)now.day;
	parts.tm_hour = (int)now.hour;
	parts.tm_min = (int)now.minute;
	parts.tm_sec = (int)now.second;
	parts.tm_isdst = 0;
	result = mktime(&parts);
	if (store != NULL)
		*store = result;
	return result;
}

#elif defined(VIG_CLOCK) && (defined(VIG_HOST_POSIX) || defined(VIG_HOST_MACOS))

#if defined(VIG_HOST_MACOS)
#define VIG_LIBC "libSystem.B.dylib"
#else
#define VIG_LIBC "libc.so.6"
#endif

/* The host's `time_t' is 64 bits on these systems and a foreign call gives back
 * 32.  The low half is the whole value until 2038, which is where this
 * `time_t' stops anyway. */
#pragma vig import vig_host_time, VIG_LIBC, "time"
extern int vig_host_time(void *store);

time_t time(time_t *store) {
	time_t result = (time_t)vig_host_time(NULL);

	if (store != NULL)
		*store = result;
	return result;
}

#else

/* Either the clock was not asked for or no host was named, so there is none to
 * read.  C allows exactly this answer, and a program built this way carries no
 * foreign import and runs on any VIG host. */
time_t time(time_t *store) {
	if (store != NULL)
		*store = (time_t)-1;
	return (time_t)-1;
}

#endif

/* There is no processor-time counter to read, and C says to return -1 when the
 * value is unavailable. */
clock_t clock(void) {
	return (clock_t)-1;
}

#endif
