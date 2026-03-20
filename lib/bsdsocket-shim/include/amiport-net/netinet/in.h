/*
 * amiport-net/netinet/in.h — Internet address structures for AmigaOS
 *
 * Drop-in replacement for <netinet/in.h>.
 */

#ifndef AMIPORT_NET_NETINET_IN_H
#define AMIPORT_NET_NETINET_IN_H

#include <amiport-net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internet address */
struct in_addr {
    unsigned long s_addr;
};

/* Internet socket address */
struct sockaddr_in {
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

/* Byte order conversion */
/* Amiga is big-endian (68k), so these are no-ops */
#define htons(x) ((unsigned short)(x))
#define htonl(x) ((unsigned long)(x))
#define ntohs(x) ((unsigned short)(x))
#define ntohl(x) ((unsigned long)(x))

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_NET_NETINET_IN_H */
