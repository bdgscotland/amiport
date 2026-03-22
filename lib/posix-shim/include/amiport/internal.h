/*
 * amiport/internal.h — Internal API shared between amiport libraries
 *
 * These functions are NOT part of the public API. They are used by
 * lib/posix-emu/ and lib/bsdsocket-shim/ to access posix-shim internals.
 *
 * Do not include this header in ported application code.
 */

#ifndef AMIPORT_INTERNAL_H
#define AMIPORT_INTERNAL_H

#include <exec/types.h>

/*
 * Convert an amiport file descriptor (int) to an AmigaDOS BPTR file handle.
 * Returns 0 (BNULL) if the fd is invalid or not in use.
 *
 * Used by posix-emu (mmap, select) and bsdsocket-shim (mixed select)
 * to translate fd numbers into handles that AmigaDOS functions accept.
 */
BPTR amiport_fd_to_fh(int fd);

/*
 * Check if an fd is valid and in use in the amiport fd table.
 * Returns 1 if valid, 0 if not.
 */
int amiport_fd_is_valid(int fd);

/*
 * Check if a string contains wildcard/glob metacharacters.
 * Detects both Unix (*, ?, [) and AmigaOS (#, ~, () patterns.
 * Returns 1 if wildcards found, 0 if literal.
 *
 * Used by glob.c and argv_expand.c — single source of truth.
 */
int amiport_glob_has_magic(const char *pattern);

#endif /* AMIPORT_INTERNAL_H */
