/*
 * inet.c — IP address conversion functions
 *
 * Pure C implementations that don't require bsdsocket.library.
 * These work on the address structures directly.
 */

#include <amiport-net/arpa/inet.h>

#include <stdio.h>
#include <string.h>

unsigned long amiport_inet_addr(const char *cp)
{
    unsigned long a, b, c, d;
    int n;

    if (!cp) return INADDR_NONE;

    n = sscanf(cp, "%lu.%lu.%lu.%lu", &a, &b, &c, &d);
    if (n != 4) return INADDR_NONE;
    if (a > 255 || b > 255 || c > 255 || d > 255) return INADDR_NONE;

    return htonl((a << 24) | (b << 16) | (c << 8) | d);
}

char *amiport_inet_ntoa(struct in_addr in)
{
    static char buf[16];
    unsigned long addr = ntohl(in.s_addr);

    snprintf(buf, sizeof(buf), "%lu.%lu.%lu.%lu",
             (addr >> 24) & 0xFF,
             (addr >> 16) & 0xFF,
             (addr >> 8)  & 0xFF,
             addr & 0xFF);
    return buf;
}

int amiport_inet_aton(const char *cp, struct in_addr *inp)
{
    unsigned long addr;

    if (!cp || !inp) return 0;

    addr = amiport_inet_addr(cp);
    if (addr == INADDR_NONE) {
        /* Special case: "255.255.255.255" is valid */
        if (strcmp(cp, "255.255.255.255") == 0) {
            inp->s_addr = INADDR_BROADCAST;
            return 1;
        }
        return 0;
    }

    inp->s_addr = addr;
    return 1;
}
