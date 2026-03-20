/*
 * amiport/string.h — BSD/POSIX string utility shims for AmigaOS
 *
 * Provides strlcpy, strlcat, reallocarray, and other string/memory
 * utilities commonly used in BSD-derived source code but not available
 * in all Amiga C runtimes.
 */

#ifndef AMIPORT_STRING_H
#define AMIPORT_STRING_H

#include <stddef.h>

/*
 * strlcpy — size-bounded string copy (OpenBSD origin)
 *
 * Copies at most dst_size-1 characters from src to dst,
 * NUL-terminating the result. Returns strlen(src) so truncation
 * can be detected (return value >= dst_size means truncation).
 */
size_t amiport_strlcpy(char *dst, const char *src, size_t dst_size);

/*
 * strlcat — size-bounded string concatenation (OpenBSD origin)
 *
 * Appends src to dst, writing at most dst_size - strlen(dst) - 1
 * characters. Always NUL-terminates (unless dst_size == 0 or the
 * original dst string was longer than dst_size). Returns
 * strlen(src) + MIN(dst_size, strlen(dst)), so truncation can be
 * detected (return value >= dst_size).
 */
size_t amiport_strlcat(char *dst, const char *src, size_t dst_size);

/*
 * reallocarray — overflow-checked array reallocation (OpenBSD origin)
 *
 * Equivalent to realloc(ptr, nmemb * size) but checks for
 * multiplication overflow. Returns NULL and sets errno = ENOMEM
 * on overflow.
 */
void *amiport_reallocarray(void *ptr, size_t nmemb, size_t size);

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_STRING_MACROS
#define strlcpy      amiport_strlcpy
#define strlcat      amiport_strlcat
#define reallocarray amiport_reallocarray
#endif

#endif /* AMIPORT_STRING_H */
