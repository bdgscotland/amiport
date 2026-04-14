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

/* --- getaddrinfo support --- */

/* amiport_addrinfo: our own struct to avoid NDK header conflicts */
struct amiport_addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    int ai_addrlen;
    char *ai_canonname;
    struct sockaddr *ai_addr;
    struct amiport_addrinfo *ai_next;
};

/* getaddrinfo flags */
#ifndef AI_PASSIVE
#define AI_PASSIVE     0x01
#endif
#ifndef AI_CANONNAME
#define AI_CANONNAME   0x02
#endif
#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 0x04
#endif

/* getaddrinfo error codes */
#ifndef EAI_AGAIN
#define EAI_AGAIN      2
#endif
#ifndef EAI_BADFLAGS
#define EAI_BADFLAGS   3
#endif
#ifndef EAI_FAIL
#define EAI_FAIL       4
#endif
#ifndef EAI_FAMILY
#define EAI_FAMILY     5
#endif
#ifndef EAI_MEMORY
#define EAI_MEMORY     6
#endif
#ifndef EAI_NONAME
#define EAI_NONAME     8
#endif
#ifndef EAI_SERVICE
#define EAI_SERVICE    9
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* DNS resolution */
struct hostent *amiport_gethostbyname(const char *name);
struct hostent *amiport_gethostbyaddr(const void *addr, int len, int type);

/* Error reporting */
int amiport_h_errno(void);

/* Modern address resolution (wraps gethostbyname via bsdsocket.library) */
int amiport_getaddrinfo(const char *node, const char *service,
                        const struct amiport_addrinfo *hints,
                        struct amiport_addrinfo **res);
void amiport_freeaddrinfo(struct amiport_addrinfo *res);
const char *amiport_gai_strerror(int errcode);

/* Convenience macros */
#ifdef AMIPORT_NET_MACROS
#define gethostbyname(n)    amiport_gethostbyname((n))
#define gethostbyaddr(a,l,t) amiport_gethostbyaddr((a),(l),(t))
#endif

/* getaddrinfo macros — always active (no NDK conflict) */
#ifndef AMIPORT_NO_GETADDRINFO_MACROS
#define addrinfo           amiport_addrinfo
#define getaddrinfo(n,s,h,r) amiport_getaddrinfo((n),(s),(h),(r))
#define freeaddrinfo(r)    amiport_freeaddrinfo((r))
#define gai_strerror(e)    amiport_gai_strerror((e))
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_NET_NETDB_H */
