/*
 * Linker stubs for symbols libstdc++ pulls in but libnix doesn't provide.
 * Mirrors the same stubs ports/openttd/ported/network_stubs.c provides;
 * factored separately here to keep the test binary independent of openttd.
 */

#include <errno.h>
#include <string.h>

int
__xpg_strerror_r(int errnum, char *buf, unsigned long buflen)
{
    const char *s = strerror(errnum);
    if (!s) {
        s = "Unknown error";
    }
    if (buflen > 0) {
        strncpy(buf, s, buflen - 1);
        buf[buflen - 1] = '\0';
    }
    return 0;
}
