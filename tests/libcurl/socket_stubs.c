/*
 * socket_stubs.c -- bsdsocket.library stubs for tests/libcurl
 *
 * The libcurl HTTP-only build references socket symbols at link time
 * (socket, connect, gethostbyname, IoctlSocket, etc.) but the test
 * suite never calls curl_easy_perform(), so these are never invoked
 * at runtime. We provide error-return stubs so the test binary links
 * without requiring lib/bsdsocket-shim.
 *
 * If a future test does need real network, link against
 * lib/bsdsocket-shim/libamiport-net.a + bsdsocket.library and remove
 * this TU from the build.
 */

#include <errno.h>

/* IoctlSocket -- fcntl/F_SETFL FIONBIO emulation */
long IoctlSocket(long fd, unsigned long cmd, char *arg) {
    (void)fd; (void)cmd; (void)arg;
    errno = ENOSYS;
    return -1;
}

/* socket() */
long socket(long domain, long type, long protocol) {
    (void)domain; (void)type; (void)protocol;
    errno = ENOSYS;
    return -1;
}

/* connect() */
long connect(long fd, void *addr, long addrlen) {
    (void)fd; (void)addr; (void)addrlen;
    errno = ENOSYS;
    return -1;
}

/* gethostbyname() -- libcurl uses sync resolver since USE_THREADS_POSIX
 * is undefined and asyn-thread.c / asyn-ares.c are not in our SRCS. */
void *gethostbyname(const char *name) {
    (void)name;
    errno = ENOSYS;
    return (void *)0;
}

/* recv() */
long recv(long fd, void *buf, long len, long flags) {
    (void)fd; (void)buf; (void)len; (void)flags;
    errno = ENOSYS;
    return -1;
}

/* send() */
long send(long fd, const void *buf, long len, long flags) {
    (void)fd; (void)buf; (void)len; (void)flags;
    errno = ENOSYS;
    return -1;
}

/* accept() */
long accept(long fd, void *addr, void *addrlen) {
    (void)fd; (void)addr; (void)addrlen;
    errno = ENOSYS;
    return -1;
}

/* getsockname() */
long getsockname(long fd, void *addr, void *addrlen) {
    (void)fd; (void)addr; (void)addrlen;
    errno = ENOSYS;
    return -1;
}

/* getsockopt() */
long getsockopt(long fd, long lvl, long opt, void *val, void *vlen) {
    (void)fd; (void)lvl; (void)opt; (void)val; (void)vlen;
    errno = ENOSYS;
    return -1;
}

/* setsockopt() */
long setsockopt(long fd, long lvl, long opt, const void *val, long vlen) {
    (void)fd; (void)lvl; (void)opt; (void)val; (void)vlen;
    errno = ENOSYS;
    return -1;
}

/* CloseSocket() */
long CloseSocket(long fd) {
    (void)fd;
    return 0;
}

/* WaitSelect() */
long WaitSelect(long n, void *r, void *w, void *e, void *tv, void *sigmask) {
    (void)n; (void)r; (void)w; (void)e; (void)tv; (void)sigmask;
    return 0;
}

/* select() -- POSIX wrapper that the NDK socket headers usually expose
 * separately from WaitSelect. Stub returns 0 (no fds ready). */
long select(long n, void *r, void *w, void *e, void *tv) {
    (void)n; (void)r; (void)w; (void)e; (void)tv;
    return 0;
}
