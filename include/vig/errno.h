#ifndef VIG_ERRNO_H
#define VIG_ERRNO_H

/* The error number.
 *
 * C requires `errno' to be a macro that expands to a modifiable `int' lvalue,
 * so the storage is a variable of another name and `errno' names it.  A program
 * may therefore have its own `errno' member or field without collision.
 *
 * `errno' starts at zero and nothing here ever clears it: a value only means
 * something after a function that documents setting it has just failed.  Check
 * the failure first, then read `errno'.
 *
 * The numbers are the ones glibc uses, so a value seen here is the one a Unix
 * programmer expects.  Nothing in VIG depends on the particular numbers.
 *
 * Only `malloc', `calloc' and `realloc' set `errno' in this library, and only
 * to ENOMEM.  There are no math functions to raise EDOM or ERANGE, and no file
 * system to raise ENOENT; those are defined so that a program which handles
 * them still compiles.
 */

extern int vig_errno;

#define errno vig_errno

#define ENOENT   2	/* no such file or directory */
#define EIO      5	/* input or output error */
#define EBADF    9	/* bad file descriptor */
#define ENOMEM  12	/* not enough memory: the heap is full */
#define EACCES  13	/* permission denied */
#define EFAULT  14	/* bad address */
#define EEXIST  17	/* file exists */
#define EINVAL  22	/* invalid argument */
#define ENOSPC  28	/* no space left */
#define EDOM    33	/* argument outside a function's domain */
#define ERANGE  34	/* result too large to represent */
#define EILSEQ  84	/* illegal byte sequence */

#endif
