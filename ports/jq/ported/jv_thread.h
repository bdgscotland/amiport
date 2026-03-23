#ifndef JV_THREAD_H
#define JV_THREAD_H

#ifdef WIN32
#ifndef __MINGW32__
#include <windows.h>
#include <winnt.h>
#include <errno.h>

/* Copied from Heimdal: pthread-like mutexes for WIN32 -- see lib/base/heimbase.h in Heimdal */
typedef struct pthread_mutex {
    HANDLE      h;
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER { INVALID_HANDLE_VALUE }

static inline int
pthread_mutex_init(pthread_mutex_t *m)
{
    m->h = CreateSemaphore(NULL, 1, 1, NULL);
    if (m->h == INVALID_HANDLE_VALUE)
        return EAGAIN;
    return 0;
}

static inline int
pthread_mutex_lock(pthread_mutex_t *m)
{
    HANDLE h, new_h;
    int created = 0;

    h = InterlockedCompareExchangePointer(&m->h, m->h, m->h);
    if (h == INVALID_HANDLE_VALUE || h == NULL) {
        created = 1;
        new_h = CreateSemaphore(NULL, 0, 1, NULL);
        if (new_h == INVALID_HANDLE_VALUE)
            return EAGAIN;
        if (InterlockedCompareExchangePointer(&m->h, new_h, h) != h) {
            created = 0;
            CloseHandle(new_h);
        }
    }
    if (!created)
        WaitForSingleObject(m->h, INFINITE);
    return 0;
}

static inline int
pthread_mutex_unlock(pthread_mutex_t *m)
{
    if (ReleaseSemaphore(m->h, 1, NULL) == FALSE)
        return EPERM;
    return 0;
}
static inline int
pthread_mutex_destroy(pthread_mutex_t *m)
{
    HANDLE h;

    h = InterlockedCompareExchangePointer(&m->h, INVALID_HANDLE_VALUE, m->h);
    if (h != INVALID_HANDLE_VALUE)
        CloseHandle(h);
    return 0;
}

typedef unsigned long pthread_key_t;
int pthread_key_create(pthread_key_t *, void (*)(void *));
int pthread_setspecific(pthread_key_t, void *);
void *pthread_getspecific(pthread_key_t);
#else
#include <pthread.h>
#endif
#elif defined(__AMIGA__)
/* amiport: pthread stubs for AmigaOS — single-threaded, no pthreads available */

/* Mutex: no-op (single-threaded) */
typedef int pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER {0}
static inline int pthread_mutex_init(pthread_mutex_t *m, void *attr) { (void)m; (void)attr; return 0; }
static inline int pthread_mutex_lock(pthread_mutex_t *m) { (void)m; return 0; }
static inline int pthread_mutex_unlock(pthread_mutex_t *m) { (void)m; return 0; }
static inline int pthread_mutex_destroy(pthread_mutex_t *m) { (void)m; return 0; }

/* Thread-specific storage: 8-slot static array (single thread, so one "thread" = index 0) */
#define AMIPORT_TSD_SLOTS 8
static void *_amiport_tsd_values[AMIPORT_TSD_SLOTS];
static int   _amiport_tsd_count = 0;
typedef int pthread_key_t;
static inline int pthread_key_create(pthread_key_t *key, void (*dtor)(void *)) {
    (void)dtor;
    if (_amiport_tsd_count >= AMIPORT_TSD_SLOTS) return 1;
    *key = _amiport_tsd_count++;
    _amiport_tsd_values[*key] = NULL;
    return 0;
}
static inline int pthread_setspecific(pthread_key_t key, void *value) {
    if (key < 0 || key >= AMIPORT_TSD_SLOTS) return 1;
    _amiport_tsd_values[key] = value;
    return 0;
}
static inline void *pthread_getspecific(pthread_key_t key) {
    if (key < 0 || key >= AMIPORT_TSD_SLOTS) return NULL;
    return _amiport_tsd_values[key];
}

/* pthread_once: execute init_routine exactly once */
typedef int pthread_once_t;
#define PTHREAD_ONCE_INIT 0
static inline int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    if (*once_control == 0) {
        *once_control = 1;
        init_routine();
    }
    return 0;
}

#else
#include <pthread.h>
#endif
#endif /* JV_THREAD_H */
