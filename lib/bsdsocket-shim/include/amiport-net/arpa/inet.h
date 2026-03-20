/*
 * amiport-net/arpa/inet.h — IP address conversion for AmigaOS
 *
 * Drop-in replacement for <arpa/inet.h>.
 */

#ifndef AMIPORT_NET_ARPA_INET_H
#define AMIPORT_NET_ARPA_INET_H

#include <amiport-net/netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* IP address conversion */
unsigned long  amiport_inet_addr(const char *cp);
char          *amiport_inet_ntoa(struct in_addr in);
int            amiport_inet_aton(const char *cp, struct in_addr *inp);

/* Convenience macros */
#ifdef AMIPORT_NET_MACROS
#define inet_addr(cp)     amiport_inet_addr((cp))
#define inet_ntoa(in)     amiport_inet_ntoa((in))
#define inet_aton(cp,inp) amiport_inet_aton((cp),(inp))
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_NET_ARPA_INET_H */
