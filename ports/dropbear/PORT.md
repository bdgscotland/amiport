# Dropbear SSH Client (dbclient) -- AmigaOS 3.x Port

## Overview

| Field | Value |
|-------|-------|
| **Upstream** | Dropbear SSH 2025.89 |
| **Port binary** | `dbclient` |
| **Category** | 4 (Network) |
| **CPU target** | 68020+ (Vampire V2, A1200+030/040/060) |
| **Binary size** | 309 KB |
| **Dependencies** | LibTomCrypt 1.18.2, LibTomMath 1.3.0, posix-shim, bsdsocket-shim |
| **TCP/IP stack** | Roadshow, AmiTCP, or Miami required |
| **PDR** | PDR-013 |
| **Status** | Phase 2 complete -- interactive SSH working |

## Build Phases

### Phase 0: Shim Extensions (DONE)
- getaddrinfo/freeaddrinfo (bsdsocket-shim, wraps gethostbyname)
- inet_ntop/inet_pton (pure C, IPv4 only)
- fcntl non-blocking sockets (IoctlSocket FIONBIO)
- WaitSelect stdin polling for console+socket multiplexing
- 29/29 shim tests pass

### Phase 1a: LibTomMath (DONE)
- 93 KB archive, -O0 with 9 hot-path files at -O1
- 25/25 tests pass, memory CLEAN, perf optimized

### Phase 1b: LibTomCrypt (DONE)
- 259 KB archive, stripped SSH config + 68k endian fast-path
- 6 hot-path files at -O1 (AES, ChaCha20, SHA-256, Poly1305, CTR, Fortuna)
- 15/15 tests pass, memory CLEAN, perf optimized

### Phase 2: dbclient Port (DONE)
- All 62 .c files compile clean
- 309 KB binary links against all libraries
- Entropy source rewritten for AmigaOS (DateStamp, clock, stack address, seed file)
- Signal pipe replaced with no-op (no fork/signals on AmigaOS)
- CSI 0x9B to ESC-[ translation for arrow keys over SSH
- amiga_getenv() via direct ENV: file read (libnix getenv/GetVar unreliable)
- 4/4 FS-UAE automated tests pass
- Interactive SSH verified on A3000/030 (FS-UAE)

## Verified Features

| Feature | Status | Method |
|---------|--------|--------|
| Non-interactive commands | WORKING | FS-UAE automated (test 4) |
| Password auth (env var) | WORKING | FS-UAE manual (SetEnv DROPBEAR_PASSWORD) |
| Password auth (prompt) | WORKING | FS-UAE manual (getpass with masked input) |
| Interactive terminal | WORKING | FS-UAE manual (typing, echo, newlines) |
| Arrow keys (history/cursor) | WORKING | FS-UAE manual (CSI 0x9B translation) |
| Tab completion | WORKING | FS-UAE manual (remote zsh completes) |
| Ctrl-C (interrupt remote) | WORKING | FS-UAE manual (kills remote sleep) |
| Backspace | WORKING | FS-UAE manual (character deletion) |
| `~.` SSH escape disconnect | WORKING | FS-UAE manual (clean disconnect) |
| `exit` / Ctrl-D | WORKING | FS-UAE manual (clean session end) |
| Host key auto-accept (-y) | WORKING | FS-UAE automated + manual |
| Version flag (-V) | WORKING | FS-UAE automated (test 1) |
| Help (--help) | WORKING | FS-UAE automated (test 3) |

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
| `getenv` | 1 | amiga_getenv via direct ENV: file read |
| `getpass` | 1 | amiport_getpass with RAW mode console |
| `fork/exec` | 3 | Disabled via localoptions.h (no ProxyCommand, no ASKPASS) |
| `pipe()` | 3 | Replaced with no-op (signal pipe for SIGCHLD not needed) |
| `signal(SIGPIPE)` | 3 | Disabled (no POSIX signals on AmigaOS) |
| `SIGWINCH` | 3 | Not implemented (deferred to v2) |
| `syslog` | 2 | Stubbed to no-op (no syslog daemon on AmigaOS) |
| `getpwuid/getpwnam` | 2 | Return NULL (single-user OS) |
| `tcgetattr/tcsetattr` | 1 | amiport/termios.h shim |
| `/dev/urandom` | 3 | Replaced with AmigaOS entropy (DateStamp + clock + stack) |

## Transformations Applied

| File | Change | Rationale |
|------|--------|-----------|
| dbrandom.c | AmigaOS entropy rewrite | No /dev/urandom; use DateStamp + clock + stack address |
| common-session.c | Signal pipe disabled | No pipe() on AmigaOS; signal pipe for SIGCHLD not needed |
| common-channel.c | amiport_console_read/write bypass | bsdsocket fd 0 hijacks libc read(0,...) |
| packet.c | read()/write() to recv()/send() | Socket I/O must use bsdsocket functions |
| cli-chansession.c | Termcodes override | Hardcode correct c_iflag/c_oflag/c_lflag for SSH negotiation |
| cli-chansession.c | Skip signal(SIGWINCH) | No POSIX signals on AmigaOS |
| cli-main.c | Stack 256KB, MEMORY_STEP 256KB | AmigaOS stack/heap sizing |
| dbutil.c | setnonblocking graceful fail | Non-socket fds can't be set non-blocking |
| dropbear_stubs.c | amiport_getpass (RAW mode) | libnix getpass opens /dev/tty which doesn't exist |
| dropbear_stubs.c | amiga_getenv (ENV: file read) | libnix getenv/GetVar don't reliably read ENV: |
| dropbear_stubs.c | CSI 0x9B to ESC-[ translation | Remote terminals expect VT100, not AmigaOS CSI |
| dropbear_stubs.c | getnameinfo stub | Numeric-only IPv4, no host lookups |
| localoptions.h | fork/exec disabled | No fork on AmigaOS |
| localoptions.h | Post-quantum disabled | Large stack arrays; saves 43 KB |
| localoptions.h | Default key path S:.ssh/ | AmigaOS home directory equivalent |
| amigaos_compat.h | getenv/getpass redirects | Macro redirects to AmigaOS-compatible implementations |
| amigaos_stubs.h | syslog/user/group stubs | No syslog daemon, single-user OS |
| includes.h | Header routing | `#ifdef __AMIGA__` for socket, netdb, arpa/inet, termios |

## Algorithms Enabled

| Type | Algorithm | Notes |
|------|-----------|-------|
| Cipher | ChaCha20-Poly1305 | Preferred -- no data cache on 68020 |
| Cipher | AES-128-CTR | Compatibility fallback |
| Cipher | AES-256-CTR | Common default |
| HMAC | SHA-256 | Standard |
| Key exchange | Curve25519 | Fast, small |
| Key exchange | ECDH (NIST) | Shared code with ECDSA |
| Key exchange | DH group14-SHA256 | Wide compatibility |
| Host key | Ed25519 | Fastest signature |
| Host key | RSA | Universal compatibility |
| Host key | ECDSA | Fast, NIST curves |
| Auth | Password | Direct console input or DROPBEAR_PASSWORD env var |
| Auth | Public key | Ed25519/RSA/ECDSA (needs dropbearkey for key generation) |

## Test Results

| Test | Result | Notes |
|------|--------|-------|
| FS-UAE automated (4 tests) | 4/4 PASS | Version, usage, help, SSH echo |
| FS-UAE manual interactive | ALL PASS | Arrow keys, Ctrl-C, tab, backspace, ~., exit |
| Memory safety audit | CLEAN | Zero critical issues, all exit paths covered |
| Performance review | CLEAN | One buffer resize applied (128->512 CSI temp) |
| vamos startup | N/A | Network port, requires bsdsocket passthrough |

## Known Limitations

1. No ProxyCommand (-J) -- requires fork/exec
2. No SSH agent forwarding -- requires AF_UNIX
3. No X11 forwarding
4. No post-quantum key exchange (sntrup761/mlkem768 disabled)
5. Entropy is not cryptographically secure without seed file at S:.ssh/entropy.dat
6. IPv4 only (no IPv6)
7. No zlib compression (disabled to save RAM)
8. No SIGWINCH -- window resize not detected (deferred to v2)
9. No SCP -- separate binary, deferred to v2
10. Public key auth requires dropbearkey (not yet ported) for key generation
11. `Failed to open .ssh/known_hosts` warning on first connect (harmless with -y flag)
12. `Warning: failed to identify current user` -- no /etc/passwd, harmless
13. TERM defaults to vt100 (set `SetEnv TERM xterm-256color` for better color support)

## Usage

```
; Set password (optional -- will prompt if not set)
SetEnv DROPBEAR_PASSWORD <password>

; Connect with auto-accept host key
WORK:dbclient -y user@host

; Run a single command
WORK:dbclient -y user@host echo hello

; Connect on non-standard port
WORK:dbclient -y -p 2222 user@host
```

## Deferred to v2

- Public key authentication (port dropbearkey utility)
- TCP port forwarding (-L local, -R remote)
- SIGWINCH window resize notification
- SCP file transfer (separate binary)
- Known hosts file management (S:.ssh/known_hosts)
