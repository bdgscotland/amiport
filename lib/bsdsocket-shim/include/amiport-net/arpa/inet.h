/*
 * amiport-net/arpa/inet.h -- IP address conversion for AmigaOS
 *
 * Drop-in replacement for <arpa/inet.h>.
 *
 * amiport: On Amiga, include the NDK's arpa/inet.h FIRST before defining
 * any macros.  The NDK's arpa/inet.h declares inet_addr/inet_ntoa/inet_aton
 * as extern functions referencing bsdsocket.library.  If AMIPORT_NET_MACROS
 * macros are defined first, the NDK's extern declarations get
 * macro-expanded into invalid syntax (double-paren function signatures).
 * By including the NDK header first we get clean declarations, then our
 * macros redirect the names to the amiport wrapper functions.
 */

#ifndef AMIPORT_NET_ARPA_INET_H
#define AMIPORT_NET_ARPA_INET_H

#include <amiport-net/netinet/in.h>

#ifdef __AMIGA__
/* Include NDK arpa/inet.h before defining any macros */
#ifndef _ARPA_INET_H
#include <arpa/inet.h>
#endif
#endif /* __AMIGA__ */

#ifdef __cplusplus
extern "C" {
#endif

/* amiport wrapper functions (implemented in bsdsocket-shim) */
unsigned long  amiport_inet_addr(const char *cp);
char          *amiport_inet_ntoa(struct in_addr in);
int            amiport_inet_aton(const char *cp, struct in_addr *inp);

/* Modern address conversion (pure C, IPv4 only) */
const char *amiport_inet_ntop(int af, const void *src, char *dst, int size);
int         amiport_inet_pton(int af, const char *src, void *dst);

/* Convenience macros -- safe to define now that NDK headers are included */
#ifdef AMIPORT_NET_MACROS
#ifndef inet_addr
#define inet_addr(cp)     amiport_inet_addr((cp))
#endif
#ifndef inet_ntoa
#define inet_ntoa(in)     amiport_inet_ntoa((in))
#endif
#ifndef inet_aton
#define inet_aton(cp,inp) amiport_inet_aton((cp),(inp))
#endif
#endif

/* inet_ntop/inet_pton macros — always active (no NDK conflict) */
#ifndef AMIPORT_NO_INET_MACROS
#define inet_ntop(af,src,dst,sz) amiport_inet_ntop((af),(src),(dst),(sz))
#define inet_pton(af,src,dst)    amiport_inet_pton((af),(src),(dst))
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_NET_ARPA_INET_H */
