/*
 * socket.c — BSD socket API wrappers for AmigaOS bsdsocket.library
 *
 * Handles automatic library lifecycle (open on first use, close at exit)
 * and wraps socket operations to dispatch through bsdsocket.library.
 *
 * See ADR-010 for design rationale.
 */

#include <amiport-net/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/socket.h>
#include <bsdsocket/socketbasetags.h>
#endif

/* --- Library lifecycle --- */

#ifdef __AMIGA__
struct Library *SocketBase = NULL;
#endif

static int socket_lib_initialized = 0;
static int socket_lib_available = 0;

/* Forward declaration */
extern void amiport_sockfd_init(void);
extern void amiport_sockfd_cleanup(void);
extern void amiport_sockfd_register(int fd);
extern void amiport_sockfd_unregister(int fd);

static void socket_atexit(void)
{
    amiport_socket_cleanup();
}

int amiport_socket_init(void)
{
    if (socket_lib_initialized) {
        return socket_lib_available ? 0 : -1;
    }

    socket_lib_initialized = 1;

#ifdef __AMIGA__
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase) {
        socket_lib_available = 0;
        return -1;
    }

    /* Configure errno handling — get per-task errno */
    SocketBaseTags(
        SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))), (ULONG)&errno,
        TAG_DONE
    );

    socket_lib_available = 1;
#else
    /* Host build — sockets work natively */
    socket_lib_available = 1;
#endif

    amiport_sockfd_init();
    atexit(socket_atexit);

    return 0;
}

void amiport_socket_cleanup(void)
{
    if (!socket_lib_initialized) return;

    amiport_sockfd_cleanup();

#ifdef __AMIGA__
    if (SocketBase) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
#endif

    socket_lib_initialized = 0;
    socket_lib_available = 0;
}

/* --- Internal: ensure library is open --- */

static int ensure_init(void)
{
    if (!socket_lib_initialized) {
        if (amiport_socket_init() != 0) return -1;
    }
    if (!socket_lib_available) {
        errno = ENOTSUP;
        return -1;
    }
    return 0;
}

/* --- Core socket operations --- */

int amiport_socket(int domain, int type, int protocol)
{
    int fd;

    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    fd = socket(domain, type, protocol);
#else
    /* Host fallback — would use native socket() */
    (void)domain; (void)type; (void)protocol;
    fd = -1;
    errno = ENOSYS;
#endif

    if (fd >= 0) {
        amiport_sockfd_register(fd);
    }
    return fd;
}

int amiport_connect(int sockfd, const struct sockaddr *addr, int addrlen)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return connect(sockfd, (struct sockaddr *)addr, addrlen);
#else
    (void)sockfd; (void)addr; (void)addrlen;
    errno = ENOSYS;
    return -1;
#endif
}

int amiport_bind(int sockfd, const struct sockaddr *addr, int addrlen)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return bind(sockfd, (struct sockaddr *)addr, addrlen);
#else
    (void)sockfd; (void)addr; (void)addrlen;
    errno = ENOSYS;
    return -1;
#endif
}

int amiport_listen(int sockfd, int backlog)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return listen(sockfd, backlog);
#else
    (void)sockfd; (void)backlog;
    errno = ENOSYS;
    return -1;
#endif
}

int amiport_accept(int sockfd, struct sockaddr *addr, int *addrlen)
{
    int fd;

    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    fd = accept(sockfd, addr, (LONG *)addrlen);
#else
    (void)sockfd; (void)addr; (void)addrlen;
    fd = -1;
    errno = ENOSYS;
#endif

    if (fd >= 0) {
        amiport_sockfd_register(fd);
    }
    return fd;
}

int amiport_closesocket(int sockfd)
{
    if (ensure_init() != 0) return -1;

    amiport_sockfd_unregister(sockfd);

#ifdef __AMIGA__
    return CloseSocket(sockfd);
#else
    (void)sockfd;
    errno = ENOSYS;
    return -1;
#endif
}

/* --- Data transfer --- */

int amiport_send(int sockfd, const void *buf, int len, int flags)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return send(sockfd, (UBYTE *)buf, len, flags);
#else
    (void)sockfd; (void)buf; (void)len; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
}

int amiport_recv(int sockfd, void *buf, int len, int flags)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return recv(sockfd, (UBYTE *)buf, len, flags);
#else
    (void)sockfd; (void)buf; (void)len; (void)flags;
    errno = ENOSYS;
    return -1;
#endif
}

int amiport_sendto(int sockfd, const void *buf, int len, int flags,
                   const struct sockaddr *dest, int addrlen)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return sendto(sockfd, (UBYTE *)buf, len, flags,
                  (struct sockaddr *)dest, addrlen);
#else
    (void)sockfd; (void)buf; (void)len; (void)flags;
    (void)dest; (void)addrlen;
    errno = ENOSYS;
    return -1;
#endif
}

int amiport_recvfrom(int sockfd, void *buf, int len, int flags,
                     struct sockaddr *src, int *addrlen)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return recvfrom(sockfd, (UBYTE *)buf, len, flags,
                    src, (LONG *)addrlen);
#else
    (void)sockfd; (void)buf; (void)len; (void)flags;
    (void)src; (void)addrlen;
    errno = ENOSYS;
    return -1;
#endif
}

/* --- Socket options --- */

int amiport_setsockopt(int sockfd, int level, int optname,
                       const void *optval, int optlen)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return setsockopt(sockfd, level, optname, (void *)optval, optlen);
#else
    (void)sockfd; (void)level; (void)optname;
    (void)optval; (void)optlen;
    errno = ENOSYS;
    return -1;
#endif
}

int amiport_getsockopt(int sockfd, int level, int optname,
                       void *optval, int *optlen)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return getsockopt(sockfd, level, optname, optval, (LONG *)optlen);
#else
    (void)sockfd; (void)level; (void)optname;
    (void)optval; (void)optlen;
    errno = ENOSYS;
    return -1;
#endif
}

/* --- Shutdown --- */

int amiport_shutdown(int sockfd, int how)
{
    if (ensure_init() != 0) return -1;

#ifdef __AMIGA__
    return shutdown(sockfd, how);
#else
    (void)sockfd; (void)how;
    errno = ENOSYS;
    return -1;
#endif
}

/* --- Error --- */

int amiport_socket_errno(void)
{
#ifdef __AMIGA__
    if (SocketBase) {
        return Errno();
    }
#endif
    return errno;
}
