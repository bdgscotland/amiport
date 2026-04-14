/*
 * fcntl_socket.c -- Minimal fcntl() for AmigaOS sockets
 *
 * Only supports F_SETFL/F_GETFL with O_NONBLOCK for socket fds.
 * Maps to IoctlSocket(fd, FIONBIO, &flag) via bsdsocket.library.
 *
 * File fds return -1 with errno EINVAL (fcntl on files not supported).
 *
 * amiport: maps fcntl(F_SETFL, O_NONBLOCK) -> IoctlSocket(FIONBIO)
 */

#include <amiport-net/socket.h>

#include <errno.h>

#ifdef __AMIGA__
#include <proto/socket.h>
extern struct Library *SocketBase;
#endif

/* Forward declarations from socket.c / sockfd.c */
extern int amiport_socket_init(void);
extern int amiport_is_socket(int fd);

/* Track non-blocking state per socket (bitmap, same as sockfd_map) */
#define FCNTL_MAP_SIZE 256
static unsigned char nonblock_map[FCNTL_MAP_SIZE / 8];

static int get_nonblock(int fd)
{
    if (fd < 0 || fd >= FCNTL_MAP_SIZE) return 0;
    return (nonblock_map[fd / 8] >> (fd % 8)) & 1;
}

static void set_nonblock(int fd, int val)
{
    if (fd < 0 || fd >= FCNTL_MAP_SIZE) return;
    if (val)
        nonblock_map[fd / 8] |= (unsigned char)(1 << (fd % 8));
    else
        nonblock_map[fd / 8] &= (unsigned char)~(1 << (fd % 8));
}

#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif
#ifndef FIONBIO
#define FIONBIO 0x8004667E
#endif

int amiport_fcntl(int fd, int cmd, int arg)
{
    if (!amiport_is_socket(fd)) {
        errno = EINVAL;
        return -1;
    }

    switch (cmd) {
    case F_GETFL:
        return get_nonblock(fd) ? O_NONBLOCK : 0;

    case F_SETFL:
        {
#ifdef __AMIGA__
            long nb;
            int rc;

            if (amiport_socket_init() != 0) {
                errno = EBADF;
                return -1;
            }

            nb = (arg & O_NONBLOCK) ? 1L : 0L;
            rc = IoctlSocket(fd, FIONBIO, (char *)&nb);
            if (rc < 0) {
                errno = EINVAL;
                return -1;
            }
            set_nonblock(fd, nb ? 1 : 0);
            return 0;
#else
            (void)arg;
            errno = ENOSYS;
            return -1;
#endif
        }

    default:
        errno = EINVAL;
        return -1;
    }
}
