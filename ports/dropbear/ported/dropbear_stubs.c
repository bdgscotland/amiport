/* dropbear_stubs.c -- Link-time stubs for POSIX functions missing on AmigaOS
 *
 * These provide the linker symbols that Dropbear references but
 * AmigaOS/libnix don't provide. Functions that need real implementations
 * (entropy, select, etc.) are in separate files.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* --- strlcat: libnix has strlcpy but not strlcat --- */
size_t strlcat(char *dst, const char *src, size_t dsize)
{
    const char *odst = dst;
    const char *osrc = src;
    size_t n = dsize;
    size_t dlen;

    while (n-- != 0 && *dst != '\0')
        dst++;
    dlen = dst - odst;
    n = dsize - dlen;

    if (n-- == 0)
        return dlen + strlen(src);
    while (*src != '\0') {
        if (n != 0) {
            *dst++ = *src;
            n--;
        }
        src++;
    }
    *dst = '\0';

    return dlen + (src - osrc);
}

/* --- getpass: read password from console without echo --- */
static char getpass_buf[128];
char *getpass(const char *prompt)
{
    /* amiport: simple getpass -- no echo suppression on AmigaOS */
    fprintf(stderr, "%s", prompt);
    fflush(stderr);
    if (fgets(getpass_buf, sizeof(getpass_buf), stdin) != NULL) {
        size_t len = strlen(getpass_buf);
        if (len > 0 && getpass_buf[len - 1] == '\n')
            getpass_buf[len - 1] = '\0';
    } else {
        getpass_buf[0] = '\0';
    }
    return getpass_buf;
}

/* --- setsid: no sessions on AmigaOS --- */
int setsid(void)
{
    return 0;
}

/* --- vfork: no fork on AmigaOS --- */
int vfork(void)
{
    return -1;
}

/* --- getgroups: single-user OS --- */
/* amiport: must return -1 with ENOSYS so Dropbear's
 * non-multiuser kernel check passes (common-session.c:77) */
int getgroups(int size, int grouplist[])
{
    (void)size;
    (void)grouplist;
    errno = ENOSYS;
    return -1;
}

/* --- getnameinfo: stub for bsdsocket-shim --- */
/* Converts sockaddr to host+port strings. On AmigaOS, just
 * do numeric conversion since we disabled host lookups. */
#include <sys/socket.h>
#include <netinet/in.h>

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen,
                int flags)
{
    (void)salen;
    (void)flags;

    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        if (host && hostlen > 0) {
            unsigned long addr = sin->sin_addr.s_addr;
            /* Network byte order on 68k (big-endian) is native */
            snprintf(host, hostlen, "%lu.%lu.%lu.%lu",
                     (addr >> 24) & 0xff, (addr >> 16) & 0xff,
                     (addr >> 8) & 0xff, addr & 0xff);
        }
        if (serv && servlen > 0) {
            unsigned short port = sin->sin_port;
            /* big-endian: port is already in host order */
            snprintf(serv, servlen, "%u", (unsigned int)port);
        }
        return 0;
    }
    return -1;
}

/* base64_encode/decode: provided by LibTomCrypt (LTC_BASE64 enabled) */
