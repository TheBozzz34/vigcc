#ifndef VIG_H
#define VIG_H

/* The VIG intrinsics.
 *
 * Each of these is a VM instruction and nothing else, so a C program cannot
 * write one and VIG has no linker to bring one in.  The compiler emits the
 * instruction where the call is.  They are declared here so a program that uses
 * one reads like a program that calls a function.
 *
 * `__vig_print` and `__vig_print_hex` each end their output with a newline, as
 * the instructions behind them do.  `__vig_write` adds nothing, so it is the one
 * to build formatted output out of.
 *
 * `__vig_print_string` takes a NUL-terminated string that lives in the program
 * image -- a literal or a global.  The address of an automatic array is not one:
 * see the foreign-pointer rule in ABI.md.
 */

void __vig_print(int value);
void __vig_print_hex(int value);
void __vig_print_string(const char *text);
void __vig_write(int byte);

/* Stops the program where it stands.  The value is taken for the sake of the
 * call and then dropped: VIG keeps no exit status. */
void __vig_halt(int status);

#endif
