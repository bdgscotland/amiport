# ADR-010: BSD Socket Shim for Network Application Porting

## Status

Accepted

## Date

2026-03-20

## Context

Network applications (curl, wget, irc clients, HTTP servers) use the BSD socket API: `socket()`, `connect()`, `bind()`, `listen()`, `accept()`, `send()`, `recv()`, `select()` on sockets, and DNS resolution (`gethostbyname()`).

AmigaOS provides `bsdsocket.library` (via AmiTCP, Roadshow, or Miami TCP/IP stacks) which is intentionally BSD-socket-compatible. The API is remarkably close to POSIX — the main differences are:

1. **Library lifecycle** — Must `OpenLibrary("bsdsocket.library", 4)` at startup and `CloseLibrary()` at shutdown
2. **Socket close** — Use `CloseSocket(fd)` not `close(fd)` (file fds and socket fds have separate namespaces)
3. **Socket ioctl** — Use `IoctlSocket()` not `ioctl()`
4. **Socket select** — Use `WaitSelect()` not `select()` (adds signal mask parameter)
5. **Headers** — Replace `<sys/socket.h>` etc. with `<proto/socket.h>` + `<bsdsocket/socketbasetags.h>`
6. **Errno** — Socket errno is per-task, accessed via `Errno()` function or `SocketBaseTags()`

Unlike the POSIX-to-AmigaOS gap for process management (fork/exec), the socket gap is narrow and mechanical. This makes it a good candidate for a Tier 1 shim with a thin lifecycle wrapper.

## Decision

Create `lib/bsdsocket-shim/` providing transparent BSD socket compatibility via bsdsocket.library.

### Design

The shim handles three things the application shouldn't need to know about:

1. **Automatic library management** — Opens bsdsocket.library on first socket call, closes at exit via `atexit()`. Fails gracefully if no TCP/IP stack is installed.

2. **Socket fd unification** — Wraps socket fds so `amiport_close()` correctly dispatches to `CloseSocket()` for socket fds and `Close()` for file fds. This is the trickiest part — the shim must track which fds are sockets.

3. **select() routing** — Socket-aware `select()` that uses `WaitSelect()` for socket fds, potentially mixing with `WaitForChar()` for file fds.

### API surface

```c
/* Thin wrappers — same signatures as BSD */
int  amiport_socket(int domain, int type, int protocol);
int  amiport_connect(int sockfd, const struct sockaddr *addr, int addrlen);
int  amiport_bind(int sockfd, const struct sockaddr *addr, int addrlen);
int  amiport_listen(int sockfd, int backlog);
int  amiport_accept(int sockfd, struct sockaddr *addr, int *addrlen);
int  amiport_send(int sockfd, const void *buf, int len, int flags);
int  amiport_recv(int sockfd, void *buf, int len, int flags);
int  amiport_sendto(int sockfd, const void *buf, int len, int flags,
                     const struct sockaddr *dest, int addrlen);
int  amiport_recvfrom(int sockfd, void *buf, int len, int flags,
                       struct sockaddr *src, int *addrlen);
int  amiport_setsockopt(int sockfd, int level, int optname,
                         const void *optval, int optlen);
int  amiport_getsockopt(int sockfd, int level, int optname,
                         void *optval, int *optlen);
int  amiport_shutdown(int sockfd, int how);

/* DNS resolution */
struct hostent *amiport_gethostbyname(const char *name);

/* select with mixed file/socket fd support */
int amiport_socket_select(int nfds, fd_set *readfds, fd_set *writefds,
                           fd_set *exceptfds, struct timeval *timeout);

/* Lifecycle (auto-called, but available for explicit use) */
int  amiport_socket_init(void);
void amiport_socket_cleanup(void);
```

### Library structure

```
lib/bsdsocket-shim/
├── include/amiport-net/
│   ├── socket.h          # Main socket API
│   ├── netinet/in.h      # sockaddr_in, IPPROTO_*, etc.
│   ├── netdb.h           # gethostbyname, etc.
│   └── arpa/inet.h       # inet_addr, inet_ntoa, etc.
├── src/
│   ├── socket.c          # Socket lifecycle + wrappers
│   ├── sockfd.c          # Socket fd tracking (is this fd a socket?)
│   ├── resolve.c         # DNS resolution wrappers
│   └── select_net.c      # WaitSelect wrapper
└── Makefile
```

### Header replacement

```
#include <sys/socket.h>   → #include <amiport-net/socket.h>
#include <netinet/in.h>   → #include <amiport-net/netinet/in.h>
#include <netdb.h>        → #include <amiport-net/netdb.h>
#include <arpa/inet.h>    → #include <amiport-net/arpa/inet.h>
```

### Graceful degradation

If bsdsocket.library is not available:
- `amiport_socket_init()` returns -1
- All socket operations return -1 with errno = ENOTSUP
- The program can detect this and print a user-friendly message

### Testing strategy

Network testing is harder than CLI testing:

1. **Unit tests** — Test library open/close lifecycle, fd tracking, sockaddr construction (can run in vamos with stubbed bsdsocket.library)
2. **Integration tests** — Require a TCP/IP stack. Use FS-UAE with Roadshow or AmiTCP configured, or a loopback setup
3. **Future:** Docker-based test environment with AmiTCP inside UAE

## Consequences

### Positive

- Unlocks an entire category of software (curl, wget, irc, HTTP tools)
- The API mapping is almost 1:1 — lowest-effort shim after posix-shim
- bsdsocket.library is the de facto standard, supported by all major TCP/IP stacks
- Graceful degradation means ports work (with reduced functionality) even without networking

### Negative

- Testing requires a full TCP/IP stack (not available in vamos)
- Socket fd tracking adds complexity and slight overhead
- Mixed file+socket select() is the hardest part to get right
- Not all users will have a TCP/IP stack installed

### Neutral

- bsdsocket.library v4 (AmiTCP 4.x / Roadshow) is the minimum target
- IPv6 is not available on classic AmigaOS — IPv4 only
- SSL/TLS requires AmiSSL library (separate from bsdsocket) — out of scope for the shim, but documented as a known limitation
