/* See LICENSE file for copyright and license details. */
/* amiport: replaced <unistd.h> with amiport shim; capped BUFSIZ to 4096 */

/* amiport: use libnix native unistd.h -- sponge uses libnix fd namespace
 * (stdin fd 0 is passed to concat, crash-patterns #12) */
#include <unistd.h>

/* amiport: removed #include "../util.h" (pulls in <regex.h> unnecessarily)
 * replaced with sponge-util.h which only declares what sponge uses */
#include "sponge-util.h"

/* amiport: BUFSIZ is 65536 on libnix -- a 64KB stack allocation would risk
 * stack overflow on a 16KB stack.  Cap at 4096 bytes. */
#define SPONGE_BUFSIZ 4096

/* amiport: renamed concat -> sponge_concat to avoid clash with libnix string.h */
int
sponge_concat(int f1, const char *s1, int f2, const char *s2)
{
    /* amiport: static buffer -- AmigaOS is single-threaded, concat is not
     * re-entrant, so static is safe and avoids stack pressure */
    static char buf[SPONGE_BUFSIZ];
    /* amiport: ssize_t replaced with long -- ssize_t not guaranteed in C89 libnix */
    long n;

    while ((n = read(f1, buf, sizeof(buf))) > 0) {
        if (writeall(f2, buf, (size_t)n) < 0) {
            weprintf("write %s:", s2);
            return -2;
        }
    }
    if (n < 0) {
        weprintf("read %s:", s1);
        return -1;
    }
    return 0;
}
