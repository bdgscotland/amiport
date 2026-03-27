/*
 * stdatomic.h — C11 atomics wrapper for bebbo-gcc (GCC 6.5.0b)
 *
 * amiport: CPython 3.11 requires <stdatomic.h> for reference counting
 * and GIL operations. AmigaOS is single-threaded, so atomics are plain
 * loads/stores. We wrap GCC's __atomic builtins for correctness.
 */

#ifndef _STDATOMIC_H
#define _STDATOMIC_H

#include <stddef.h>  /* size_t */

/* Memory order constants */
typedef enum {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

/* Atomic types — on single-threaded AmigaOS these are just regular types */
#define _Atomic(T) T

typedef _Atomic(int) atomic_int;
typedef _Atomic(unsigned int) atomic_uint;
typedef _Atomic(long) atomic_long;
typedef _Atomic(unsigned long) atomic_ulong;
typedef _Atomic(long long) atomic_llong;
typedef _Atomic(unsigned long long) atomic_ullong;
typedef _Atomic(size_t) atomic_size_t;
typedef _Atomic(void *) atomic_uintptr_t;

/* Atomic operations via GCC builtins */
#define atomic_store_explicit(obj, val, order) \
    __atomic_store_n(obj, val, order)

#define atomic_load_explicit(obj, order) \
    __atomic_load_n(obj, order)

#define atomic_exchange_explicit(obj, val, order) \
    __atomic_exchange_n(obj, val, order)

#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail) \
    __atomic_compare_exchange_n(obj, expected, desired, 0, succ, fail)

#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail) \
    __atomic_compare_exchange_n(obj, expected, desired, 1, succ, fail)

#define atomic_fetch_add_explicit(obj, val, order) \
    __atomic_fetch_add(obj, val, order)

#define atomic_fetch_sub_explicit(obj, val, order) \
    __atomic_fetch_sub(obj, val, order)

#define atomic_fetch_or_explicit(obj, val, order) \
    __atomic_fetch_or(obj, val, order)

#define atomic_fetch_and_explicit(obj, val, order) \
    __atomic_fetch_and(obj, val, order)

/* Convenience macros without explicit ordering (default seq_cst) */
#define atomic_store(obj, val) atomic_store_explicit(obj, val, memory_order_seq_cst)
#define atomic_load(obj) atomic_load_explicit(obj, memory_order_seq_cst)
#define atomic_exchange(obj, val) atomic_exchange_explicit(obj, val, memory_order_seq_cst)
#define atomic_fetch_add(obj, val) atomic_fetch_add_explicit(obj, val, memory_order_seq_cst)
#define atomic_fetch_sub(obj, val) atomic_fetch_sub_explicit(obj, val, memory_order_seq_cst)

/* Init */
#define ATOMIC_VAR_INIT(val) (val)
#define atomic_init(obj, val) (*(obj) = (val))

/* Fence — no-op on single-threaded AmigaOS */
#define atomic_thread_fence(order) __atomic_thread_fence(order)
#define atomic_signal_fence(order) __atomic_signal_fence(order)

/* Flag type */
typedef _Atomic(unsigned char) atomic_flag;
#define ATOMIC_FLAG_INIT { 0 }
#define atomic_flag_test_and_set(obj) __atomic_test_and_set(obj, __ATOMIC_SEQ_CST)
#define atomic_flag_test_and_set_explicit(obj, order) __atomic_test_and_set(obj, order)
#define atomic_flag_clear(obj) __atomic_clear(obj, __ATOMIC_SEQ_CST)
#define atomic_flag_clear_explicit(obj, order) __atomic_clear(obj, order)

#endif /* _STDATOMIC_H */
