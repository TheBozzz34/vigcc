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

/* Native-library imports
 *
 * Map an ordinary, fully-prototyped C function declaration to a VIG foreign
 * import with a pragma immediately before its declaration:
 *
 *   #pragma vig import GetSystemTime, "kernel32.dll", "GetSystemTime"
 *   extern void GetSystemTime(struct systemtime *out);
 *
 * The first name is the C function name.  The strings name the host library
 * and exported symbol, so they may differ.  VIG derives each foreign argument
 * from the C prototype: signed integers become `i32', unsigned integers become
 * `u32', and pointers become `ptr'.  The foreign ABI permits at most four
 * arguments; it supports only void or 32-bit integer results, no varargs,
 * callbacks, floating-point values, aggregates by value, or pointer results.
 */

#endif
