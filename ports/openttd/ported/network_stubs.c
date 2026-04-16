/*
 * network_stubs.c -- Stubs for symbols missing from libnix in dedicated build
 *
 * For dedicated server build on AmigaOS, we don't actually use the network
 * server features. These stubs satisfy the linker without pulling in libsocket
 * (which would auto-open bsdsocket.library).
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>

/* Newlib's strerror_r is GNU-style; libstdc++ wants XSI-compliant __xpg_strerror_r. */
int __xpg_strerror_r(int errnum, char *buf, size_t buflen) {
    const char *s = strerror(errnum);
    if (!s) s = "Unknown error";
    if (buflen > 0) {
        strncpy(buf, s, buflen - 1);
        buf[buflen - 1] = '\0';
    }
    return 0;
}

/* getpwuid stub -- AmigaOS has no /etc/passwd. */
struct passwd *getpwuid(uid_t uid) {
    (void)uid;
    return NULL;
}
