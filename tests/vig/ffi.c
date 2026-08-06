/* C declarations drive VIG's foreign-import ABI tags, while the pragma carries
 * the two details C itself cannot express: the host library and symbol name. */
#include <vig.h>

#pragma vig import MulDiv, "kernel32.dll", "MulDiv"
extern int MulDiv(int number, int numerator, int denominator);

struct systemtime {
	short year;
	short month;
	short day_of_week;
	short day;
	short hour;
	short minute;
	short second;
	short milliseconds;
};

#pragma vig import GetSystemTime, "kernel32.dll", "GetSystemTime"
extern void GetSystemTime(struct systemtime *out);

int main(void) {
	struct systemtime now;

	__vig_print(MulDiv(6, 7, 3));
	GetSystemTime(&now);
	/* This verifies that the native call wrote through a pointer into our frame
	 * without baking the current time into the expected output. */
	__vig_print(now.year >= 2020);
	return 0;
}
