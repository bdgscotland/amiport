/*
 * sockfd.c — Socket file descriptor tracking
 *
 * Maintains a bitmap of which fds are sockets vs regular files.
 * This is needed because AmigaOS has separate fd namespaces:
 * close(fd) for files, CloseSocket(fd) for sockets.
 *
 * The shim's amiport_closesocket() uses this to route correctly,
 * and amiport_net_select() uses it to split fd_sets between
 * WaitSelect() (sockets) and WaitForChar() (files).
 */

#include <amiport-net/socket.h>

#include <string.h>

/* Bitmap: bit N set = fd N is a socket */
#define SOCKFD_MAP_SIZE (FD_SETSIZE / 8)
static unsigned char sockfd_map[SOCKFD_MAP_SIZE];

void amiport_sockfd_init(void)
{
    memset(sockfd_map, 0, sizeof(sockfd_map));
}

void amiport_sockfd_cleanup(void)
{
    memset(sockfd_map, 0, sizeof(sockfd_map));
}

void amiport_sockfd_register(int fd)
{
    if (fd >= 0 && fd < FD_SETSIZE) {
        sockfd_map[fd / 8] |= (1 << (fd % 8));
    }
}

void amiport_sockfd_unregister(int fd)
{
    if (fd >= 0 && fd < FD_SETSIZE) {
        sockfd_map[fd / 8] &= ~(1 << (fd % 8));
    }
}

int amiport_is_socket(int fd)
{
    if (fd < 0 || fd >= FD_SETSIZE) return 0;
    return (sockfd_map[fd / 8] >> (fd % 8)) & 1;
}
