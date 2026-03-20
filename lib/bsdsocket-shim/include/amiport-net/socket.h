/*
 * amiport-net/socket.h — BSD socket API for AmigaOS via bsdsocket.library
 *
 * Drop-in replacement for <sys/socket.h>. Wraps bsdsocket.library (AmiTCP,
 * Roadshow, Miami) with automatic library lifecycle management.
 *
 * See ADR-010 for design and API surface.
 *
 * Usage:
 *   #include <amiport-net/socket.h>
 *   // Use amiport_socket(), amiport_connect(), etc.
 *   // Or define AMIPORT_NET_MACROS for transparent name mapping.
 */

#ifndef AMIPORT_NET_SOCKET_H
#define AMIPORT_NET_SOCKET_H

#ifdef __AMIGA__
/*
 * On Amiga, pull in the NDK's sys/socket.h FIRST. It defines sa_family_t,
 * socklen_t, struct sockaddr, struct linger, and all the SO_* / AF_* macros.
 * We must not redefine any of these.
 */
#include <exec/types.h>
#include <sys/socket.h>
#else
/*
 * Non-Amiga host build: provide the types ourselves so the code compiles
 * for testing on Linux/macOS without the NDK.
 */

#ifndef _SOCKLEN_T
#define _SOCKLEN_T
typedef unsigned long socklen_t;
#endif

#ifndef _SA_FAMILY_T
#define _SA_FAMILY_T
typedef unsigned char sa_family_t;
#endif

/* Generic socket address */
#ifndef _STRUCT_SOCKADDR
#define _STRUCT_SOCKADDR
struct sockaddr {
    unsigned char sa_len;
    sa_family_t   sa_family;
    char          sa_data[14];
};
#endif

/* Socket storage (large enough for any address family) */
#ifndef _STRUCT_SOCKADDR_STORAGE
#define _STRUCT_SOCKADDR_STORAGE
struct sockaddr_storage {
    sa_family_t ss_family;
    char        _ss_pad[126];
};
#endif

/* --- Address families --- */
#define AF_UNSPEC  0
#define AF_INET    2

#define PF_UNSPEC  AF_UNSPEC
#define PF_INET    AF_INET

/* --- Socket types --- */
#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3

/* --- Protocol numbers --- */
#define IPPROTO_IP     0
#define IPPROTO_TCP    6
#define IPPROTO_UDP    17

/* --- Socket options --- */
#define SOL_SOCKET     0xFFFF

#define SO_REUSEADDR   0x0004
#define SO_KEEPALIVE   0x0008
#define SO_BROADCAST   0x0020
#define SO_LINGER      0x0080
#define SO_SNDBUF      0x1001
#define SO_RCVBUF      0x1002
#define SO_SNDTIMEO    0x1005
#define SO_RCVTIMEO    0x1006
#define SO_ERROR       0x1007
#define SO_TYPE        0x1008

/* --- Shutdown modes --- */
#define SHUT_RD        0
#define SHUT_WR        1
#define SHUT_RDWR      2

/* --- Message flags --- */
#define MSG_OOB        0x01
#define MSG_PEEK       0x02
#define MSG_DONTROUTE  0x04
#define MSG_DONTWAIT   0x40

/* --- Linger structure --- */
#ifndef _STRUCT_LINGER
#define _STRUCT_LINGER
struct linger {
    int l_onoff;
    int l_linger;
};
#endif

#endif /* __AMIGA__ */

#ifdef __cplusplus
extern "C" {
#endif

/* --- fd_set for select --- */

#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

#ifndef _AMIPORT_FD_SET
#define _AMIPORT_FD_SET
typedef struct {
    unsigned long fds_bits[FD_SETSIZE / 32];
} amiport_net_fd_set;

#define AMIPORT_NET_FD_ZERO(set) \
    do { int _i; for (_i = 0; _i < (int)(FD_SETSIZE/32); _i++) (set)->fds_bits[_i] = 0; } while(0)
#define AMIPORT_NET_FD_SET(fd, set)   ((set)->fds_bits[(fd)/32] |= (1UL << ((fd) % 32)))
#define AMIPORT_NET_FD_CLR(fd, set)   ((set)->fds_bits[(fd)/32] &= ~(1UL << ((fd) % 32)))
#define AMIPORT_NET_FD_ISSET(fd, set) ((set)->fds_bits[(fd)/32] & (1UL << ((fd) % 32)))
#endif

/* --- Timeval for select --- */

#ifndef _AMIPORT_TIMEVAL
#define _AMIPORT_TIMEVAL
struct amiport_net_timeval {
    long tv_sec;
    long tv_usec;
};
#endif

/* ================================================================
 * Socket API — thin wrappers over bsdsocket.library
 * ================================================================ */

/* Lifecycle (auto-called on first socket use, but available for explicit use) */
int  amiport_socket_init(void);
void amiport_socket_cleanup(void);

/* Core socket operations */
int amiport_socket(int domain, int type, int protocol);
int amiport_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int amiport_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int amiport_listen(int sockfd, int backlog);
int amiport_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int amiport_closesocket(int sockfd);

/* Data transfer */
int amiport_send(int sockfd, const void *buf, int len, int flags);
int amiport_recv(int sockfd, void *buf, int len, int flags);
int amiport_sendto(int sockfd, const void *buf, int len, int flags,
                   const struct sockaddr *dest, socklen_t addrlen);
int amiport_recvfrom(int sockfd, void *buf, int len, int flags,
                     struct sockaddr *src, socklen_t *addrlen);

/* Socket options */
int amiport_setsockopt(int sockfd, int level, int optname,
                       const void *optval, socklen_t optlen);
int amiport_getsockopt(int sockfd, int level, int optname,
                       void *optval, socklen_t *optlen);

/* Shutdown */
int amiport_shutdown(int sockfd, int how);

/* Select — handles mixed file/socket fd sets */
int amiport_net_select(int nfds, amiport_net_fd_set *readfds,
                       amiport_net_fd_set *writefds,
                       amiport_net_fd_set *exceptfds,
                       struct amiport_net_timeval *timeout);

/* Query whether a fd is a socket */
int amiport_is_socket(int fd);

/* Get last socket error */
int amiport_socket_errno(void);

/* ================================================================
 * Convenience macros for transparent name mapping
 * ================================================================ */

#ifdef AMIPORT_NET_MACROS
#define socket(d,t,p)       amiport_socket((d),(t),(p))
#define connect(s,a,l)      amiport_connect((s),(a),(l))
#define bind(s,a,l)         amiport_bind((s),(a),(l))
#define listen(s,b)         amiport_listen((s),(b))
#define accept(s,a,l)       amiport_accept((s),(a),(l))
#define closesocket(s)      amiport_closesocket((s))
#define send(s,b,l,f)       amiport_send((s),(b),(l),(f))
#define recv(s,b,l,f)       amiport_recv((s),(b),(l),(f))
#define sendto(s,b,l,f,d,n) amiport_sendto((s),(b),(l),(f),(d),(n))
#define recvfrom(s,b,l,f,a,n) amiport_recvfrom((s),(b),(l),(f),(a),(n))
#define setsockopt(s,l,o,v,n) amiport_setsockopt((s),(l),(o),(v),(n))
#define getsockopt(s,l,o,v,n) amiport_getsockopt((s),(l),(o),(v),(n))
#define shutdown(s,h)       amiport_shutdown((s),(h))
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_NET_SOCKET_H */
