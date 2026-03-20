/*
 * select_net.c — Socket-aware select() for AmigaOS
 *
 * Uses WaitSelect() from bsdsocket.library for socket fds.
 * For mixed file+socket fd sets, splits the sets and handles
 * each type separately.
 *
 * This is the most complex part of the bsdsocket shim — see ADR-010.
 */

#include <amiport-net/socket.h>

#include <string.h>
#include <errno.h>

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/dos.h>
extern struct Library *SocketBase;
#endif

/* Forward declarations from sockfd.c */
extern int amiport_is_socket(int fd);

/* Forward declaration from socket.c */
extern int amiport_socket_init(void);

int amiport_net_select(int nfds, amiport_net_fd_set *readfds,
                       amiport_net_fd_set *writefds,
                       amiport_net_fd_set *exceptfds,
                       struct amiport_net_timeval *timeout)
{
#ifdef __AMIGA__
    int has_sockets = 0;
    int has_files = 0;
    int fd;
    int result = 0;
    ULONG sigmask = 0;

    if (amiport_socket_init() != 0) {
        errno = ENOTSUP;
        return -1;
    }

    /* Scan to determine if we have sockets, files, or both */
    for (fd = 0; fd < nfds; fd++) {
        int in_set = 0;
        if (readfds && AMIPORT_NET_FD_ISSET(fd, readfds)) in_set = 1;
        if (writefds && AMIPORT_NET_FD_ISSET(fd, writefds)) in_set = 1;
        if (exceptfds && AMIPORT_NET_FD_ISSET(fd, exceptfds)) in_set = 1;

        if (in_set) {
            if (amiport_is_socket(fd)) {
                has_sockets = 1;
            } else {
                has_files = 1;
            }
        }
    }

    if (has_sockets && !has_files) {
        /* Pure socket select — use WaitSelect directly */
        struct timeval tv;
        struct timeval *tvp = NULL;

        if (timeout) {
            tv.tv_sec = timeout->tv_sec;
            tv.tv_usec = timeout->tv_usec;
            tvp = &tv;
        }

        result = WaitSelect(nfds, (fd_set *)readfds, (fd_set *)writefds,
                           (fd_set *)exceptfds, tvp, &sigmask);
        return result;
    }

    if (has_files && !has_sockets) {
        /* Pure file select — poll with WaitForChar */
        long timeout_us = 0;

        if (timeout) {
            timeout_us = timeout->tv_sec * 1000000L + timeout->tv_usec;
        }

        result = 0;
        for (fd = 0; fd < nfds; fd++) {
            if (readfds && AMIPORT_NET_FD_ISSET(fd, readfds)) {
                BPTR fh = (BPTR)fd; /* Simplified — real impl needs fd→BPTR mapping */
                if (WaitForChar(fh, timeout_us)) {
                    result++;
                } else {
                    AMIPORT_NET_FD_CLR(fd, readfds);
                }
            }
            if (writefds && AMIPORT_NET_FD_ISSET(fd, writefds)) {
                /* Files are always write-ready */
                result++;
            }
        }
        return result;
    }

    if (has_sockets && has_files) {
        /* Mixed select — split into socket and file sets, handle separately.
         * This is an approximation: we poll files first (non-blocking),
         * then WaitSelect on sockets with remaining timeout. */
        amiport_net_fd_set sock_read, sock_write, sock_except;
        struct timeval tv;
        struct timeval *tvp = NULL;

        AMIPORT_NET_FD_ZERO(&sock_read);
        AMIPORT_NET_FD_ZERO(&sock_write);
        AMIPORT_NET_FD_ZERO(&sock_except);

        /* Split: sockets into sock_* sets, remove from original */
        for (fd = 0; fd < nfds; fd++) {
            if (amiport_is_socket(fd)) {
                if (readfds && AMIPORT_NET_FD_ISSET(fd, readfds)) {
                    AMIPORT_NET_FD_SET(fd, &sock_read);
                }
                if (writefds && AMIPORT_NET_FD_ISSET(fd, writefds)) {
                    AMIPORT_NET_FD_SET(fd, &sock_write);
                }
                if (exceptfds && AMIPORT_NET_FD_ISSET(fd, exceptfds)) {
                    AMIPORT_NET_FD_SET(fd, &sock_except);
                }
            }
        }

        /* Poll files first (non-blocking) */
        for (fd = 0; fd < nfds; fd++) {
            if (!amiport_is_socket(fd)) {
                if (readfds && AMIPORT_NET_FD_ISSET(fd, readfds)) {
                    BPTR fh = (BPTR)fd;
                    if (WaitForChar(fh, 0)) {
                        result++;
                    } else {
                        AMIPORT_NET_FD_CLR(fd, readfds);
                    }
                }
                if (writefds && AMIPORT_NET_FD_ISSET(fd, writefds)) {
                    result++;
                }
            }
        }

        /* If files are ready, do non-blocking socket check */
        if (result > 0) {
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            tvp = &tv;
        } else if (timeout) {
            tv.tv_sec = timeout->tv_sec;
            tv.tv_usec = timeout->tv_usec;
            tvp = &tv;
        }

        {
            int sock_result;
            sock_result = WaitSelect(nfds,
                (fd_set *)&sock_read, (fd_set *)&sock_write,
                (fd_set *)&sock_except, tvp, &sigmask);

            if (sock_result > 0) {
                result += sock_result;
                /* Merge socket results back into original fd_sets */
                for (fd = 0; fd < nfds; fd++) {
                    if (amiport_is_socket(fd)) {
                        if (readfds) {
                            if (AMIPORT_NET_FD_ISSET(fd, &sock_read)) {
                                AMIPORT_NET_FD_SET(fd, readfds);
                            } else {
                                AMIPORT_NET_FD_CLR(fd, readfds);
                            }
                        }
                        if (writefds) {
                            if (AMIPORT_NET_FD_ISSET(fd, &sock_write)) {
                                AMIPORT_NET_FD_SET(fd, writefds);
                            } else {
                                AMIPORT_NET_FD_CLR(fd, writefds);
                            }
                        }
                        if (exceptfds) {
                            if (AMIPORT_NET_FD_ISSET(fd, &sock_except)) {
                                AMIPORT_NET_FD_SET(fd, exceptfds);
                            } else {
                                AMIPORT_NET_FD_CLR(fd, exceptfds);
                            }
                        }
                    }
                }
            } else if (sock_result < 0) {
                return sock_result; /* Error from WaitSelect */
            }
        }

        return result;
    }

    /* No fds set at all — just a timeout */
    if (timeout) {
        Delay((timeout->tv_sec * 50) + (timeout->tv_usec / 20000));
    }
    return 0;

#else
    /* Host stub */
    (void)nfds; (void)readfds; (void)writefds; (void)exceptfds; (void)timeout;
    errno = ENOSYS;
    return -1;
#endif
}
