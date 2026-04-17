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
 * Not in AmigaOS NDK. Needed by OpenTTD network code. */
#ifndef _STRUCT_SOCKADDR_STORAGE
#define _STRUCT_SOCKADDR_STORAGE
#include <sys/socket.h>
struct sockaddr_storage {
    unsigned char ss_len;
    unsigned char ss_family;
    char _ss_pad[126];
};
#endif

/* AI_ADDRCONFIG -- not available on AmigaOS */
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif

/* select() stub for network code -- maps to no-op for now.
 * Real implementation would use bsdsocket.library WaitSelect(). */
#ifndef __AMIGA_SELECT_STUB
#define __AMIGA_SELECT_STUB
#include <sys/time.h>
#ifdef __cplusplus
extern "C" {
#endif
static inline int select(int nfds, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    (void)nfds; (void)r; (void)w; (void)e; (void)t;
    return 0;
}
#ifdef __cplusplus
}
#endif
#endif
