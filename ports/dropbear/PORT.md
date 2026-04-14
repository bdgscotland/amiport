# Dropbear SSH Client (dbclient) — AmigaOS 3.x Port

## Overview

| Field | Value |
|-------|-------|
| **Upstream** | Dropbear SSH 2025.89 |
| **Port binary** | `dbclient` |
| **Category** | 4 (Network) |
| **CPU target** | 68020+ (Vampire V2, A1200+030/040/060) |
| **Binary size** | ~308 KB |
| **Dependencies** | LibTomCrypt 1.18.2, LibTomMath 1.3.0, posix-shim, bsdsocket-shim |
| **TCP/IP stack** | Roadshow, AmiTCP, or Miami required |
| **PDR** | PDR-013 |
| **Status** | Phase 2 — Foundation built, compiles and links. Not yet runtime tested. |

## Build Phases

### Phase 0: Shim Extensions (DONE)
- getaddrinfo/freeaddrinfo (bsdsocket-shim, wraps gethostbyname)
- inet_ntop/inet_pton (pure C, IPv4 only)
- fcntl non-blocking sockets (IoctlSocket FIONBIO)
- 29/29 shim tests pass

### Phase 1a: LibTomMath (DONE)
- 93 KB archive, -O0 with 9 hot-path files at -O1
- 25/25 tests pass, memory CLEAN, perf optimized

### Phase 1b: LibTomCrypt (DONE)
- 259 KB archive, stripped SSH config + 68k endian fast-path
- 6 hot-path files at -O1 (AES, ChaCha20, SHA-256, Poly1305, CTR, Fortuna)
- 15/15 tests pass, memory CLEAN, perf optimized
- Added LTC_SHA384, LTC_BASE64, LTC_CLEAN_STACK

### Phase 2: dbclient Port (IN PROGRESS)
- All 62 .c files compile clean
- 308 KB binary links against all libraries
- Entropy source rewritten for AmigaOS (DateStamp, clock, stack address, seed file)
- Signal pipe replaced with no-op (no fork/signals on AmigaOS)

## Portability Analysis

| POSIX Dependency | Tier | Resolution |
|------------------|------|------------|
| `getaddrinfo/freeaddrinfo` | 1 | bsdsocket-shim (Phase 0) |
| `inet_ntop/inet_pton` | 1 | bsdsocket-shim (Phase 0, pure C) |
| `fcntl(F_SETFL, O_NONBLOCK)` | 1 | bsdsocket-shim (IoctlSocket FIONBIO) |
| `socket/connect/bind/listen/accept` | 1 | bsdsocket-shim (macro mapping) |
| `select()` | 2 | bsdsocket-shim `amiport_select` wraps `WaitSelect()` |
| `setsockopt/getsockopt` | 1 | bsdsocket-shim |
| `getsockname/getpeername` | 1 | bsdsocket-shim |
| `getnameinfo` | 2 | Local stub (numeric-only, IPv4) |
| `fork/exec` | 3 | Disabled via localoptions.h (no ProxyCommand, no ASKPASS) |
| `pipe()` | 3 | Replaced with no-op (signal pipe for SIGCHLD not needed) |
| `signal(SIGPIPE)` | 3 | Disabled (no POSIX signals on AmigaOS) |
| `SIGWINCH` | 3 | Not yet implemented (pending: poll ioctl TIOCGWINSZ) |
| `syslog` | 2 | Stubbed to no-op (no syslog daemon on AmigaOS) |
| `getpwuid/getpwnam` | 2 | Return NULL (single-user OS) |
| `tcgetattr/tcsetattr` | 1 | amiport/termios.h shim |
| `/dev/urandom` | 3 | Replaced with AmigaOS entropy (DateStamp + clock + stack) |
| `utmp/wtmp/lastlog` | N/A | Server-only, excluded from build |
| `AF_UNIX` | N/A | Disabled (no UNIX domain sockets on AmigaOS) |
| `AF_INET6` | N/A | IPv4 only |

## Transformations Applied

| File | Change | Rationale |
|------|--------|-----------|
| dbrandom.c | AmigaOS entropy rewrite | No /dev/urandom; use DateStamp + clock + stack address |
| common-session.c | Signal pipe disabled | No pipe() on AmigaOS; signal pipe for SIGCHLD not needed |
| localoptions.h | fork/exec disabled | No fork on AmigaOS; disables ProxyCommand and ASKPASS |
| localoptions.h | Post-quantum disabled | sntrup761/mlkem768 have large stack arrays; saves 43 KB |
| sysoptions.h | Server auth guard | Client-only build; gate server checks with `#if DROPBEAR_SERVER` |
| amigaos_compat.h | syslog constants | LOG_* macros for code that references syslog priority levels |
| amigaos_stubs.h | syslog/user/group stubs | No syslog daemon, single-user OS |
| amigaos_stubs.h | termios constants | Full POSIX terminal mode constants for SSH negotiation |
| dropbear_stubs.c | getnameinfo | Numeric-only IPv4 conversion; disabled host lookups |
| dropbear_stubs.c | strlcat/getpass/setsid | Missing libnix functions |
| includes.h | Header routing | `#ifdef __AMIGA__` for socket, netdb, arpa/inet, termios |
| cli-main.c | SIGPIPE disabled | No POSIX signals on AmigaOS |
| cli-main.c | `__MEMORY_STEP` | Reduced libnix malloc pool from 4MB to 256KB |

## Algorithms Enabled

| Type | Algorithm | Notes |
|------|-----------|-------|
| Cipher | ChaCha20-Poly1305 | Preferred — no data cache on 68020 |
| Cipher | AES-128-CTR | Compatibility fallback |
| Cipher | AES-256-CTR | Common default |
| HMAC | SHA-256 | Standard |
| Key exchange | Curve25519 | Fast, small |
| Key exchange | ECDH (NIST) | Shared code with ECDSA |
| Key exchange | DH group14-SHA256 | Wide compatibility |
| Host key | Ed25519 | Fastest signature |
| Host key | RSA | Universal compatibility |
| Host key | ECDSA | Fast, NIST curves |
| Auth | Password | Direct console input |
| Auth | Public key | Ed25519/RSA/ECDSA |

## Test Results

| Test | Result | Notes |
|------|--------|-------|
| Compilation (62 files) | PASS | All .c files compile clean with `-O0 -m68020` |
| Linking | PASS | 308 KB binary, valid Amiga hunk format |
| vamos startup | FAIL | Exit 252 (libnix init issue or entropy crash in vamos) |
| FS-UAE functional | PENDING | Needs Roadshow TCP/IP stack |
| FS-UAE interactive SSH | PENDING | Needs live SSH server |

## Remaining Work

### Tier 3 Blockers (Must Fix Before Testing)
- [ ] Verify select() → WaitSelect() works via bsdsocket-shim
- [ ] SIGWINCH handling (window resize notification)
- [ ] Terminal I/O for interactive session (console.device raw mode)

### Testing
- [ ] FS-UAE + Roadshow TCP/IP setup
- [ ] Connect to host SSH server
- [ ] Password authentication
- [ ] Public key authentication
- [ ] Interactive shell session
- [ ] Arrow keys / terminal escape sequences
- [ ] Ctrl-C disconnect
- [ ] TCP forwarding (-L, -R)

## Known Limitations

1. No ProxyCommand (-J) — requires fork/exec
2. No SSH agent forwarding — requires AF_UNIX
3. No X11 forwarding
4. No post-quantum key exchange (sntrup761/mlkem768 disabled)
5. Entropy is not cryptographically secure without seed file
6. IPv4 only (no IPv6)
7. No zlib compression (disabled to save RAM)
