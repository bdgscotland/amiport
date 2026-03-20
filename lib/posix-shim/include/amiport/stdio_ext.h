/*
 * amiport/stdio_ext.h — Extended stdio shims for AmigaOS
 *
 * Provides vasprintf/asprintf (GNU/BSD dynamic string formatting),
 * mkstemp (secure temp file creation), and pread/pwrite (positional I/O).
 */

#ifndef AMIPORT_STDIO_EXT_H
#define AMIPORT_STDIO_EXT_H

#include <exec/types.h>
#include <stdarg.h>

/*
 * vasprintf/asprintf — dynamic string formatting
 *
 * Allocates a string large enough to hold the formatted output.
 * Caller must free() the returned *strp. Returns character count
 * (excluding NUL) on success, -1 on error.
 */
int amiport_vasprintf(char **strp, const char *fmt, va_list ap);
int amiport_asprintf(char **strp, const char *fmt, ...);

/*
 * mkstemp — create and open a unique temporary file
 *
 * Template must end with "XXXXXX" (modified in place).
 * Returns amiport file descriptor or -1 on error.
 * File is created in the location specified by the template
 * (use "T:tmpXXXXXX" for Amiga temp directory).
 */
int amiport_mkstemp(char *tmpl);

/*
 * pread/pwrite — positional read/write without changing file offset
 *
 * Implemented as seek + read/write + seek-back. Not atomic,
 * but sufficient for single-threaded Amiga programs.
 */
LONG amiport_pread(int fd, void *buf, LONG count, LONG offset);
LONG amiport_pwrite(int fd, const void *buf, LONG count, LONG offset);

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_STDIO_EXT_MACROS
#define vasprintf  amiport_vasprintf
#define asprintf   amiport_asprintf
#define mkstemp    amiport_mkstemp
#define pread      amiport_pread
#define pwrite     amiport_pwrite
#endif

#endif /* AMIPORT_STDIO_EXT_H */
