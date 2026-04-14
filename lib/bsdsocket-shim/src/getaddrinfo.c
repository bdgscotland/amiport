/*
 * getaddrinfo.c -- POSIX getaddrinfo/freeaddrinfo for AmigaOS
 *
 * Wraps gethostbyname() from bsdsocket.library to provide the modern
 * POSIX address resolution interface. IPv4 only (AmigaOS has no IPv6).
 *
 * amiport: maps getaddrinfo -> gethostbyname via bsdsocket.library
 */

#include <amiport-net/netdb.h>
#include <amiport-net/socket.h>
#include <amiport-net/arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Forward declaration from socket.c */
extern int amiport_socket_init(void);

int amiport_getaddrinfo(const char *node, const char *service,
                        const struct amiport_addrinfo *hints,
                        struct amiport_addrinfo **res)
{
    struct hostent *he;
    struct amiport_addrinfo *ai;
    struct sockaddr_in *sa;
    int port;
    int i;
    int socktype;
    int protocol;
    struct amiport_addrinfo *head;
    struct amiport_addrinfo *tail;

    if (!res) return EAI_FAIL;
    *res = NULL;

    socktype = SOCK_STREAM;
    protocol = 0;

    if (hints) {
        if (hints->ai_family != AF_UNSPEC && hints->ai_family != AF_INET) {
            return EAI_FAMILY;
        }
        if (hints->ai_socktype)
            socktype = hints->ai_socktype;
        if (hints->ai_protocol)
            protocol = hints->ai_protocol;
    }

    /* Parse port from service string */
    port = 0;
    if (service) {
        port = atoi(service);
        if (port < 0 || port > 65535) {
            return EAI_SERVICE;
        }
    }

    /* Handle numeric address or NULL node */
    if (!node) {
        if (hints && (hints->ai_flags & AI_PASSIVE)) {
            /* Wildcard address for bind() */
            ai = (struct amiport_addrinfo *)calloc(1,
                sizeof(struct amiport_addrinfo) + sizeof(struct sockaddr_in));
            if (!ai) return EAI_MEMORY;

            sa = (struct sockaddr_in *)(ai + 1);
            sa->sin_family = AF_INET;
            sa->sin_port = htons((unsigned short)port);
            sa->sin_addr.s_addr = INADDR_ANY;

            ai->ai_flags = hints ? hints->ai_flags : 0;
            ai->ai_family = AF_INET;
            ai->ai_socktype = socktype;
            ai->ai_protocol = protocol;
            ai->ai_addrlen = sizeof(struct sockaddr_in);
            ai->ai_addr = (struct sockaddr *)sa;
            ai->ai_canonname = NULL;
            ai->ai_next = NULL;

            *res = ai;
            return 0;
        }
        return EAI_NONAME;
    }

    /* Try numeric address first (no DNS needed) */
    {
        unsigned long addr;
        addr = amiport_inet_addr(node);
        if (addr != INADDR_NONE ||
            strcmp(node, "255.255.255.255") == 0) {
            ai = (struct amiport_addrinfo *)calloc(1,
                sizeof(struct amiport_addrinfo) + sizeof(struct sockaddr_in));
            if (!ai) return EAI_MEMORY;

            sa = (struct sockaddr_in *)(ai + 1);
            sa->sin_family = AF_INET;
            sa->sin_port = htons((unsigned short)port);
            sa->sin_addr.s_addr = addr;

            ai->ai_flags = hints ? hints->ai_flags : 0;
            ai->ai_family = AF_INET;
            ai->ai_socktype = socktype;
            ai->ai_protocol = protocol;
            ai->ai_addrlen = sizeof(struct sockaddr_in);
            ai->ai_addr = (struct sockaddr *)sa;
            ai->ai_canonname = NULL;
            ai->ai_next = NULL;

            *res = ai;
            return 0;
        }

        /* If NUMERICHOST was requested, don't do DNS */
        if (hints && (hints->ai_flags & AI_NUMERICHOST)) {
            return EAI_NONAME;
        }
    }

    /* DNS resolution via bsdsocket.library */
    if (amiport_socket_init() != 0) {
        return EAI_FAIL;
    }

    he = amiport_gethostbyname(node);
    if (!he) {
        return EAI_NONAME;
    }

    /* Build result chain from hostent address list */
    head = NULL;
    tail = NULL;

    for (i = 0; he->h_addr_list[i] != NULL; i++) {
        ai = (struct amiport_addrinfo *)calloc(1,
            sizeof(struct amiport_addrinfo) + sizeof(struct sockaddr_in));
        if (!ai) {
            amiport_freeaddrinfo(head);
            return EAI_MEMORY;
        }

        sa = (struct sockaddr_in *)(ai + 1);
        sa->sin_family = AF_INET;
        sa->sin_port = htons((unsigned short)port);
        memcpy(&sa->sin_addr, he->h_addr_list[i], (size_t)he->h_length);

        ai->ai_flags = hints ? hints->ai_flags : 0;
        ai->ai_family = AF_INET;
        ai->ai_socktype = socktype;
        ai->ai_protocol = protocol;
        ai->ai_addrlen = sizeof(struct sockaddr_in);
        ai->ai_addr = (struct sockaddr *)sa;
        ai->ai_next = NULL;

        /* First entry gets the canonical name */
        if (i == 0 && hints && (hints->ai_flags & AI_CANONNAME)) {
            ai->ai_canonname = strdup(he->h_name);
        } else {
            ai->ai_canonname = NULL;
        }

        if (!head) {
            head = ai;
        } else {
            tail->ai_next = ai;
        }
        tail = ai;
    }

    if (!head) {
        return EAI_NONAME;
    }

    *res = head;
    return 0;
}

void amiport_freeaddrinfo(struct amiport_addrinfo *res)
{
    struct amiport_addrinfo *next;

    while (res) {
        next = res->ai_next;
        if (res->ai_canonname) {
            free(res->ai_canonname);
        }
        /* ai_addr points into the same allocation as ai (ai + 1) */
        free(res);
        res = next;
    }
}

const char *amiport_gai_strerror(int errcode)
{
    switch (errcode) {
    case 0:            return "Success";
    case EAI_AGAIN:    return "Temporary failure in name resolution";
    case EAI_BADFLAGS: return "Invalid value for ai_flags";
    case EAI_FAIL:     return "Non-recoverable failure in name resolution";
    case EAI_FAMILY:   return "ai_family not supported";
    case EAI_MEMORY:   return "Memory allocation failure";
    case EAI_NONAME:   return "Name or service not known";
    case EAI_SERVICE:  return "Servname not supported for ai_socktype";
    default:           return "Unknown error";
    }
}
