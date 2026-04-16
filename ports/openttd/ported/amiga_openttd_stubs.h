#ifndef AMIGA_THREAD_STUBS_H
#define AMIGA_THREAD_STUBS_H

/* Provide uint for UNIX mode (sys/types.h on libnix may not have it) */
#ifndef _UINT_T_DEFINED
#define _UINT_T_DEFINED
typedef unsigned int uint;
#endif

#ifdef __cplusplus
#ifndef _GLIBCXX_HAS_GTHREADS

namespace std {
    class mutex {
    public:
        void lock() {}
        void unlock() {}
        bool try_lock() { return true; }
    };

    class recursive_mutex {
    public:
        void lock() {}
        void unlock() {}
        bool try_lock() { return true; }
    };

    class condition_variable {
    public:
        void notify_one() {}
        void notify_all() {}
        template<typename Lock> void wait(Lock&) {}
        template<typename Lock, typename Pred>
        void wait(Lock& lk, Pred p) { while (!p()) {} }
    };
}

#endif
#endif
#endif

/* POSIX stubs missing from libnix */
#ifndef _AMIGA_POSIX_STUBS
#define _AMIGA_POSIX_STUBS
#ifdef __cplusplus
extern "C" {
#endif
static inline const char *strsignal(int sig) { (void)sig; return "unknown signal"; }
static inline int kill(int pid, int sig) { (void)pid; (void)sig; return 0; }
#ifdef __cplusplus
}
#endif
#endif

/* sockaddr_storage -- large enough for any socket address family.
 * Not in AmigaOS NDK. Defined standalone (no sys/socket.h include)
 * to avoid pulling in exec/nodes.h which defines struct Node and
 * collides with OpenTTD's linkgraph Node typedef. */
#ifndef _STRUCT_SOCKADDR_STORAGE
#define _STRUCT_SOCKADDR_STORAGE
struct sockaddr_storage {
    unsigned char ss_len;
    unsigned char ss_family;
    char _ss_pad[126];
};
#endif

/* Fix: SDL Sint32 (=int32_t=long on 68k) vs int mismatch in std::max.
 * Provide overloads that accept mixed long/int arguments. */
#ifdef __cplusplus
#include <algorithm>
namespace std {
    inline long max(long a, int b) { return a > b ? a : b; }
    inline long max(int a, long b) { return a > b ? a : b; }
    inline long min(long a, int b) { return a < b ? a : b; }
    inline long min(int a, long b) { return a < b ? a : b; }
}
#endif

/* AI_ADDRCONFIG -- not available on AmigaOS */
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif

/* IPv6 stubs -- no IPv6 on classic AmigaOS */
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 0
#endif
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
#endif
#ifndef AF_INET6
#define AF_INET6 26
#endif

/* Network resolution stubs -- getaddrinfo/freeaddrinfo are in our
 * bsdsocket-shim but need the declaration visible. getnameinfo and
 * gai_strerror are stubs. */
#ifdef __cplusplus
extern "C" {
#endif

struct addrinfo;
struct sockaddr;

#ifndef _HAVE_GETADDRINFO_DECL
#define _HAVE_GETADDRINFO_DECL
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
#endif

static inline int getnameinfo(const struct sockaddr *sa, unsigned int salen,
    char *host, unsigned int hostlen, char *serv, unsigned int servlen,
    int flags)
{
    (void)sa; (void)salen; (void)serv; (void)servlen; (void)flags;
    if (host && hostlen > 0) host[0] = '\0';
    return -1;
}

static inline const char *gai_strerror(int errcode)
{
    (void)errcode;
    return "address resolution error";
}

#ifdef __cplusplus
}
#endif

/* AmigaOS NDK <exec/nodes.h> defines 'struct Node' which collides with
 * OpenTTD's linkgraph typedef Node. OpenTTD never uses struct Node directly
 * (no ln_Succ, ln_Pred, etc.). Rename it before any NDK header loads. */
#define Node AmigaExecNode
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <sys/socket.h>
#include <sys/select.h>
#undef Node
