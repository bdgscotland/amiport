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

/* AI_ADDRCONFIG -- not available on AmigaOS */
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif

/* NOTE: AmigaOS NDK <exec/nodes.h> defines 'struct Node' which collides
 * with OpenTTD's linkgraph typedefs. Avoid including any NDK exec headers
 * in this stub. The collision is handled by OpenTTD's os_abstraction.h
 * which includes the NDK socket headers AFTER the linkgraph headers. */


/* select() -- declared in libnix <sys/select.h> (pulled in via os_abstraction.h).
 * Do NOT include sys/select.h here -- it transitively pulls in exec/nodes.h
 * which defines struct Node, colliding with OpenTTD's linkgraph typedef Node. */
