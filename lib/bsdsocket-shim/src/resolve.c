/*
 * resolve.c — DNS resolution wrappers for AmigaOS bsdsocket.library
 *
 * Wraps gethostbyname/gethostbyaddr to go through bsdsocket.library.
 */

#include <amiport-net/netdb.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/socket.h>
extern struct Library *SocketBase;
#endif

static int last_h_errno = 0;

/* Forward declaration from socket.c */
extern int amiport_socket_init(void);

struct hostent *amiport_gethostbyname(const char *name)
{
#ifdef __AMIGA__
    struct hostent *he;

    if (amiport_socket_init() != 0) {
        last_h_errno = NO_RECOVERY;
        return NULL;
    }

    he = gethostbyname((STRPTR)name);
    if (!he) {
        last_h_errno = HOST_NOT_FOUND;
    }
    return he;
#else
    (void)name;
    last_h_errno = NO_RECOVERY;
    return NULL;
#endif
}

struct hostent *amiport_gethostbyaddr(const void *addr, int len, int type)
{
#ifdef __AMIGA__
    struct hostent *he;

    if (amiport_socket_init() != 0) {
        last_h_errno = NO_RECOVERY;
        return NULL;
    }

    he = gethostbyaddr((UBYTE *)addr, len, type);
    if (!he) {
        last_h_errno = HOST_NOT_FOUND;
    }
    return he;
#else
    (void)addr; (void)len; (void)type;
    last_h_errno = NO_RECOVERY;
    return NULL;
#endif
}

int amiport_h_errno(void)
{
    return last_h_errno;
}
