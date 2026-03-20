/*
 * amiport/errno_map.h — AmigaDOS IoErr() to POSIX errno mapping
 *
 * Maps AmigaDOS error codes to their closest POSIX errno equivalents.
 */

#ifndef AMIPORT_ERRNO_MAP_H
#define AMIPORT_ERRNO_MAP_H

#include <errno.h>

/*
 * Convert the current AmigaDOS IoErr() value to a POSIX errno.
 * Call this after a failed AmigaDOS operation to set errno appropriately.
 */
int amiport_map_errno(void);

/*
 * Set errno from a specific AmigaDOS error code.
 * Useful when you've already captured the error code.
 */
int amiport_errno_from_ioerr(long ioerr);

#endif /* AMIPORT_ERRNO_MAP_H */
