/*
 * amiport-net/netinet/in.h — Internet address structures for AmigaOS
 *
 * Drop-in replacement for <netinet/in.h>.
 */

#ifndef AMIPORT_NET_NETINET_IN_H
#define AMIPORT_NET_NETINET_IN_H

#include <amiport-net/socket.h>

#ifdef __AMIGA__
/*
 * On Amiga, the NDK's <sys/socket.h> (already pulled in above) includes
 * <netinet/in.h> itself via <proto/bsdsocket.h>, so we may already have
 * in_addr, sockaddr_in, INADDR_*, htons/htonl, etc. Pull in the NDK
 * header explicitly to be sure.
 */
#ifndef _NETINET_IN_H
#include <netinet/in.h>
#endif

/* INADDR_LOOPBACK: the NDK does not define it by name, only IN_LOOPBACKNET. */
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK  0x7F000001UL
#endif

#else /* !__AMIGA__ — host build */

/* Internet address */
struct in_addr {
    unsigned long s_addr;
};

/* Internet socket address */
struct sockaddr_in {
    unsigned char  sin_len;
    sa_family_t    sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

/* Special addresses */
#define INADDR_ANY       0x00000000UL
#define INADDR_LOOPBACK  0x7F000001UL
#define INADDR_BROADCAST 0xFFFFFFFFUL
#define INADDR_NONE      0xFFFFFFFFUL

/* Byte order conversion — host build may be little-endian, so use real swaps */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htons(x) ((unsigned short)(((x) >> 8) | ((x) << 8)))
#define htonl(x) ((unsigned long)(((x) >> 24) | (((x) >> 8) & 0xFF00UL) | (((x) & 0xFF00UL) << 8) | ((x) << 24)))
#define ntohs(x) htons(x)
#define ntohl(x) htonl(x)
#else
/* Big-endian or unknown — no-ops */
#define htons(x) ((unsigned short)(x))
#define htonl(x) ((unsigned long)(x))
#define ntohs(x) ((unsigned short)(x))
#define ntohl(x) ((unsigned long)(x))
#endif

#endif /* __AMIGA__ */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_NET_NETINET_IN_H */
