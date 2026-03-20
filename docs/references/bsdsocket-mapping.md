# BSD Socket API Mapping — POSIX to AmigaOS bsdsocket.library

Reference for the `bsdsocket-shim` library — maps POSIX socket operations to AmigaOS bsdsocket.library equivalents.

## Overview

AmigaOS provides BSD socket networking via `bsdsocket.library`, available from:
- **AmiTCP** 4.x (open-source, most common)
- **Roadshow** (commercial, most compatible, actively maintained)
- **Miami/MiamiDx** (commercial, popular in the late 90s)

The API is intentionally BSD-compatible. The main differences are lifecycle management and a few function renames.

## Library Lifecycle

| POSIX | AmigaOS | Notes |
|-------|---------|-------|
| (implicit — always available) | `OpenLibrary("bsdsocket.library", 4)` | Must open before any socket call |
| (implicit — kernel cleanup) | `CloseLibrary(SocketBase)` | Must close at shutdown |
| `errno` (global) | `SocketBaseTags(SBTC_ERRNOPTR, &errno)` | Configure per-task errno |

The bsdsocket-shim handles this automatically via `amiport_socket_init()` (called on first socket use) and `amiport_socket_cleanup()` (registered via `atexit()`).

## Socket Operations

| POSIX Function | bsdsocket.library | amiport Wrapper | Notes |
|---------------|-------------------|-----------------|-------|
| `socket(d,t,p)` | `socket(d,t,p)` | `amiport_socket()` | Identical |
| `connect(s,a,l)` | `connect(s,a,l)` | `amiport_connect()` | Identical |
| `bind(s,a,l)` | `bind(s,a,l)` | `amiport_bind()` | Identical |
| `listen(s,b)` | `listen(s,b)` | `amiport_listen()` | Identical |
| `accept(s,a,l)` | `accept(s,a,l)` | `amiport_accept()` | Returns socket fd |
| `close(s)` | `CloseSocket(s)` | `amiport_closesocket()` | **Different!** Must use CloseSocket, not close() |
| `send(s,b,l,f)` | `send(s,b,l,f)` | `amiport_send()` | Identical |
| `recv(s,b,l,f)` | `recv(s,b,l,f)` | `amiport_recv()` | Identical |
| `sendto(...)` | `sendto(...)` | `amiport_sendto()` | Identical |
| `recvfrom(...)` | `recvfrom(...)` | `amiport_recvfrom()` | Identical |
| `setsockopt(...)` | `setsockopt(...)` | `amiport_setsockopt()` | Identical |
| `getsockopt(...)` | `getsockopt(...)` | `amiport_getsockopt()` | Identical |
| `shutdown(s,h)` | `shutdown(s,h)` | `amiport_shutdown()` | Identical |
| `select(...)` | `WaitSelect(...)` | `amiport_net_select()` | **Different!** Extra signal mask param |
| `ioctl(s,...)` | `IoctlSocket(s,...)` | Not yet wrapped | **Different name** |
| `read(s,b,l)` | `recv(s,b,l,0)` | Use `amiport_recv()` | Socket read via recv |
| `write(s,b,l)` | `send(s,b,l,0)` | Use `amiport_send()` | Socket write via send |

## DNS Resolution

| POSIX Function | bsdsocket.library | amiport Wrapper | Notes |
|---------------|-------------------|-----------------|-------|
| `gethostbyname(n)` | `gethostbyname(n)` | `amiport_gethostbyname()` | Identical |
| `gethostbyaddr(a,l,t)` | `gethostbyaddr(a,l,t)` | `amiport_gethostbyaddr()` | Identical |
| `getaddrinfo(...)` | Not available | Not wrapped | Use gethostbyname instead |
| `freeaddrinfo(...)` | Not available | Not wrapped | — |
| `gai_strerror(...)` | Not available | Not wrapped | — |

**Note:** `getaddrinfo()` is the modern POSIX DNS API but is not available in bsdsocket.library. Ported programs using it must be converted to use `gethostbyname()`.

## IP Address Conversion

| POSIX Function | Implementation | Notes |
|---------------|---------------|-------|
| `inet_addr(cp)` | Pure C in shim | No library needed |
| `inet_ntoa(in)` | Pure C in shim | No library needed |
| `inet_aton(cp,inp)` | Pure C in shim | No library needed |
| `inet_pton(af,src,dst)` | Not wrapped | IPv6 not available |
| `inet_ntop(af,src,dst,len)` | Not wrapped | IPv6 not available |

## Byte Order

Amiga (68k) is **big-endian**, same as network byte order. The conversion macros are no-ops:

```c
#define htons(x) ((unsigned short)(x))  /* No-op */
#define htonl(x) ((unsigned long)(x))   /* No-op */
#define ntohs(x) ((unsigned short)(x))  /* No-op */
#define ntohl(x) ((unsigned long)(x))   /* No-op */
```

## Header Mapping

| POSIX Header | amiport Replacement | Native Amiga Headers |
|-------------|--------------------|--------------------|
| `<sys/socket.h>` | `<amiport-net/socket.h>` | `<proto/socket.h>` |
| `<netinet/in.h>` | `<amiport-net/netinet/in.h>` | `<netinet/in.h>` (from AmiTCP SDK) |
| `<netdb.h>` | `<amiport-net/netdb.h>` | `<netdb.h>` (from AmiTCP SDK) |
| `<arpa/inet.h>` | `<amiport-net/arpa/inet.h>` | `<arpa/inet.h>` (from AmiTCP SDK) |
| `<sys/select.h>` | `<amiport-net/socket.h>` | N/A (use WaitSelect) |

## The close()/CloseSocket() Problem

This is the biggest porting gotcha. On POSIX, `close(fd)` works for both files and sockets. On AmigaOS, you must use:
- `Close(fh)` for AmigaDOS file handles
- `CloseSocket(fd)` for bsdsocket.library sockets

The bsdsocket-shim tracks which fds are sockets (via `sockfd.c`) and routes `amiport_closesocket()` correctly. Programs must be transformed to use `amiport_closesocket()` for socket fds.

## The select()/WaitSelect() Problem

| Feature | POSIX `select()` | AmigaOS `WaitSelect()` |
|---------|-----------------|----------------------|
| Signature | `select(nfds, r, w, e, tv)` | `WaitSelect(nfds, r, w, e, tv, sigmask)` |
| Extra param | — | `ULONG *sigmask` for Amiga signal integration |
| Socket fds | Works | Works |
| File fds | Works | **Does not work** — file fds not supported |
| Mixed | Works | **Does not work** |

The bsdsocket-shim's `amiport_net_select()` handles this by:
1. Scanning fd_sets to classify fds as socket or file
2. Pure socket → direct `WaitSelect()`
3. Pure file → `WaitForChar()` polling
4. Mixed → split, poll files, WaitSelect sockets

## Graceful Degradation

When no TCP/IP stack is installed:
- `amiport_socket_init()` returns -1
- All socket operations return -1 with `errno = ENOTSUP`
- Programs can detect this and display: "Error: TCP/IP stack not available. Install AmiTCP or Roadshow."

## Limitations

1. **IPv4 only** — No IPv6 on classic AmigaOS
2. **No SSL/TLS** — Requires AmiSSL library (separate from bsdsocket.library)
3. **No Unix domain sockets** — `AF_UNIX` not available
4. **No raw sockets** — `SOCK_RAW` may not be available in all stacks
5. **No `socketpair()`** — Not available in bsdsocket.library
6. **No `poll()`** — Use `select()` (WaitSelect) instead
7. **Thread safety** — bsdsocket.library is per-task; each AmigaOS task needs its own `OpenLibrary()` call
8. **No scatter/gather I/O** — `readv()`/`writev()` not available
