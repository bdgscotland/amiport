/* See LICENSE file for copyright and license details. */
/* amiport: replaced <unistd.h> with amiport shim; ssize_t -> long */

/* amiport: use libnix native unistd.h -- sponge uses libnix fd namespace */
#include <unistd.h>

/* amiport: removed #include "../util.h" (pulls in <regex.h> unnecessarily)
 * replaced with sponge-util.h which only declares what sponge uses */
#include "sponge-util.h"

/* amiport: ssize_t replaced with long -- ssize_t not guaranteed in C89 libnix.
 * Return type and local variable both updated. */
long
writeall(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    /* amiport: ssize_t -> long */
    long n;

    while (len) {
        n = write(fd, p, len);
        if (n <= 0)
            return n;
        p += n;
        /* amiport: cast to avoid signed/unsigned mismatch warning */
        len -= (size_t)n;
    }

    return (long)(p - (const char *)buf);
}
