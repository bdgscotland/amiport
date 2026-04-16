/*
 * amiport/thread_stubs.h -- No-op threading primitives for AmigaOS
 *
 * bebbo-gcc 13.3 with --enable-threads=no provides std::thread,
 * std::lock_guard, and std::unique_lock but NOT std::mutex,
 * std::recursive_mutex, or std::condition_variable (gated behind
 * _GLIBCXX_HAS_GTHREADS).
 *
 * This header provides no-op stubs for those types. Safe on AmigaOS
 * which has no preemptive multitasking for user processes.
 *
 * Usage: -include amiport/thread_stubs.h (before any source file)
 * Requires: C++11 or later, bebbo-gcc 13.3+
 *
 * See known-pitfalls.md: "bebbo-gcc 13.3 std::mutex/condition_variable
 * Missing Without Gthreads"
 */

#ifndef AMIPORT_THREAD_STUBS_H
#define AMIPORT_THREAD_STUBS_H

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

#endif /* _GLIBCXX_HAS_GTHREADS */
#endif /* __cplusplus */

#endif /* AMIPORT_THREAD_STUBS_H */
