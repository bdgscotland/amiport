/*
 * amiport-net/netdb.h — DNS resolution for AmigaOS via bsdsocket.library
 *
 * Drop-in replacement for <netdb.h>.
 */

#ifndef AMIPORT_NET_NETDB_H
#define AMIPORT_NET_NETDB_H

#include <amiport-net/socket.h>

#ifdef __AMIGA__
/*
 * Pull in the NDK's <netdb.h> which defines struct hostent using Amiga types.
 * We must not redefine it.
 */
#ifndef _NETDB_H
#include <netdb.h>
#endif

#else /* !__AMIGA__ — host build */

/* Host entry structure */
struct hostent {
    char  *h_name;
    char **h_aliases;
    int    h_addrtype;
    int    h_length;
    char **h_addr_list;
};

#define h_addr h_addr_list[0]

#endif /* __AMIGA__ */

/* Error codes for DNS resolution — guard in case NDK defines them */
#ifndef HOST_NOT_FOUND
#define HOST_NOT_FOUND 1
#endif
#ifndef TRY_AGAIN
#define TRY_AGAIN      2
#endif
#ifndef NO_RECOVERY
#define NO_RECOVERY    3
#endif
#ifndef NO_DATA
#define NO_DATA        4
#endif
#ifndef NO_ADDRESS
#define NO_ADDRESS     NO_DATA
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
