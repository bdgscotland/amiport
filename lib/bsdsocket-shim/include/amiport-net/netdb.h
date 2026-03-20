/*
 * amiport-net/netdb.h — DNS resolution for AmigaOS via bsdsocket.library
 *
 * Drop-in replacement for <netdb.h>.
 */

#ifndef AMIPORT_NET_NETDB_H
#define AMIPORT_NET_NETDB_H

#include <amiport-net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Host entry structure */
struct hostent {
    char  *h_name;
    char **h_aliases;
    int    h_addrtype;
    int    h_length;
    char **h_addr_list;
};

#define h_addr h_addr_list[0]

/* Error codes for DNS resolution */
#define HOST_NOT_FOUND 1
#define TRY_AGAIN      2
#define NO_RECOVERY    3
#define NO_DATA        4
#define NO_ADDRESS     NO_DATA

/* DNS resolution */
struct hostent *amiport_gethostbyname(const char *name);
struct hostent *amiport_gethostbyaddr(const void *addr, int len, int type);

/* Error reporting */
int amiport_h_errno(void);

/* Convenience macros */
#ifdef AMIPORT_NET_MACROS
#define gethostbyname(n)    amiport_gethostbyname((n))
#define gethostbyaddr(a,l,t) amiport_gethostbyaddr((a),(l),(t))
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_NET_NETDB_H */
