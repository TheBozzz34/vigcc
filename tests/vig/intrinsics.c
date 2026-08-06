/* The intrinsics themselves: each is a VM instruction, not a call. */
#include <vig.h>

char greeting[] = "hello";

int main(void) {
    __vig_print(42);
    __vig_print(-7);
    __vig_print_hex(255);
    __vig_print_string(greeting);
    __vig_print_string("literal");
    __vig_write('o');
    __vig_write('k');
    __vig_write('\n');
    return 0;
}
