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

#ifdef __AMIGA__
#include <proto/dos.h>
#endif

/* --- amiga_getenv: read ENV: variables by opening the file directly ---
 * libnix getenv() may not read from ENV: filesystem at runtime.
 * GetVar() also fails in some contexts despite GetEnv working.
 * This version opens ENV:<name> directly via AmigaDOS Open(). */
static char getenv_buf[256];
char *amiga_getenv(const char *name)
{
    char path[300];
    BPTR fh;
    LONG n;

    snprintf(path, sizeof(path), "ENV:%s", name);
    fh = Open((CONST_STRPTR)path, MODE_OLDFILE);
    if (!fh) return NULL;
    n = Read(fh, getenv_buf, sizeof(getenv_buf) - 1);
    Close(fh);
    if (n <= 0) return NULL;
    if (n > 0 && getenv_buf[n - 1] == '\n') n--;
    getenv_buf[n] = '\0';
    return getenv_buf;
}

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

/* --- getpass: read password from console without echo ---
 * amiport: renamed to amiport_getpass to avoid libnix's __stdargs
 * getpass() which tries to open /dev/tty and fails silently.
 * The #define getpass amiport_getpass in amigaos_compat.h ensures
 * all callers use this version. */
static char getpass_buf[128];
char *amiport_getpass(const char *prompt)
{
    int pos = 0;
    char c;

    fprintf(stderr, "%s", prompt);
    fflush(stderr);

#ifdef __AMIGA__
    SetMode(Input(), 1);
    while (pos < (int)sizeof(getpass_buf) - 1) {
        if (Read(Input(), &c, 1) != 1) break;
        if (c == '\r' || c == '\n') break;
        if (c == '\b' || c == 127) {
            if (pos > 0) pos--;
            continue;
        }
        getpass_buf[pos++] = c;
    }
    SetMode(Input(), 0);
    fprintf(stderr, "\n");
#else
    if (fgets(getpass_buf, sizeof(getpass_buf), stdin) != NULL) {
        pos = strlen(getpass_buf);
        if (pos > 0 && getpass_buf[pos - 1] == '\n') pos--;
    }
#endif
    getpass_buf[pos] = '\0';
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

#ifdef __AMIGA__
#include <proto/dos.h>

/* amiport: On FS-UAE, bsdsocket fd 0 hijacks libc read(0, ...) to read
 * from the socket, making stdin inaccessible via read(). Bypass libnix's
 * fd table and use AmigaDOS Read(Input())/Write(Output()) directly for
 * console I/O. See known-pitfalls: bsdsocket fd 0 collision.
 *
 * amiport: Translates AmigaOS CSI (0x9B) to VT100 ESC-[ (0x1B 0x5B).
 * Console.device sends arrow/function keys as 0x9B+params+letter, but
 * remote SSH servers expect ESC-[ sequences (TERM=vt100). Without this,
 * arrow keys are silently ignored by the remote terminal. */
int amiport_console_read(void *buf, int len)
{
    unsigned char tmp[512];
    int maxread, n, i, out;
    unsigned char *dst = (unsigned char *)buf;
    BPTR fh = Input();

    if (!fh || !IsInteractive(fh)) {
        errno = EAGAIN;
        return -1;
    }
    if (!WaitForChar(fh, 0)) {
        errno = EAGAIN;
        return -1;
    }

    /* Read into temp buffer; halve request to leave room for CSI expansion
     * (each 0x9B becomes two bytes: 0x1B 0x5B) */
    maxread = len / 2;
    if (maxread < 1) maxread = 1;
    if (maxread > (int)sizeof(tmp)) maxread = (int)sizeof(tmp);
    n = Read(fh, tmp, maxread);
    if (n <= 0) return n;

    out = 0;
    for (i = 0; i < n && out < len; i++) {
        if (tmp[i] == 0x9B && out + 1 < len) {
            dst[out++] = 0x1B;
            dst[out++] = 0x5B;
        } else {
            dst[out++] = tmp[i];
        }
    }
    return out;
}

int amiport_console_write(const void *buf, int len)
{
    return Write(Output(), buf, len);
}
#endif
