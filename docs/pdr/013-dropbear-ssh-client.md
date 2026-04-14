# PDR-013: Dropbear SSH Client for AmigaOS 3.x

## Status

Accepted

## Date

2026-04-14

## Problem

No modern, maintained SSH client exists for classic AmigaOS 3.x. Existing ports (OpenSSH from ~2005) are orphaned, require the ixemul runtime layer, and use outdated cipher suites. Duncan's A2000 with Vampire V2 500+ accelerator, X-Surf 100 ethernet, and Roadshow TCP/IP stack has the hardware capability for SSH but no software to use it. SSH connectivity would be a flagship capability for the amiport project.

## Target Users

Amiga users with 68020+ accelerators and TCP/IP networking (Roadshow, AmiTCP, or Miami). Primary target: Vampire V2/V4 users with 100+ MHz equivalent speed. Secondary: 68030/040/060 accelerated Amigas with ethernet.

## Decision

Port Dropbear SSH 2025.89 as a phased project, starting with the SSH client (`dbclient`) and crypto foundation libraries.

### Phased approach

**Phase 0: Shim Extensions (prerequisite)**
- Add `getaddrinfo()`, `inet_ntop()`/`inet_pton()`, `fcntl()` non-blocking sockets to `lib/bsdsocket-shim/` via `/extend-shim`
- Add CSI/VT100 escape sequence translation module to `lib/console-shim/` or `lib/posix-shim/` (outbound 0x9B to ESC [, inbound pass-through)
- Rebuild all shims, regression test affected ports

**Phase 1: Port LibTomMath + LibTomCrypt**
- `lib/libtommath/` -- big integer arithmetic, C89, zero OS dependencies
- `lib/libtomcrypt/` -- crypto primitives (AES-CTR, ChaCha20-Poly1305, SHA-256, Ed25519, RSA), depends on LibTomMath
- Full library pipeline per `.claude/rules/library-pipeline.md`
- `-O0` default, selective `-O1 -m68020` on audited hot math files (mp_mul, mp_sqr, mp_exptmod)

**Phase 2: Port dbclient (SSH client)**
- `ports/dropbear/` with client-only build (`PROGRAMS="dbclient"`)
- Aggressive feature strip via `localoptions.h`: disable DSS, PAM, syslog, utmp/wtmp, X11/agent/TCP forwarding
- Keep: RSA + Ed25519 + ECDSA, ChaCha20-Poly1305 (preferred) + AES-CTR, SHA-256, password + pubkey auth
- 68020+ mandatory, `-m68020` CFLAGS

**Phase 2.5: AmiSSL Backend (enhancement)**
- Runtime detection: try `OpenLibrary("amisslmaster.library", ...)` at startup
- If AmiSSL available, use it for all crypto (gets security updates transparently)
- If not, fall back to bundled LibTomCrypt (self-contained, no install dependency)
- Manual library opening (NOT `libamisslauto.a` -- see known-pitfalls)

**Phase 3: Port dropbearkey + dropbearconvert**
- Key generation and format conversion utilities
- Multi-call binary option

**Phase 4 (deferred): Port dropbear sshd**
- Requires solving: no `fork()`, no pty layer, no user auth model on AmigaOS
- Single-connection `-F` foreground mode is the feasible path
- Console window per session via `CreateNewProc()` + Intuition window

### Key architecture decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| SSH implementation | Dropbear 2025.89 | C89, self-contained crypto, designed for embedded, ~20-40K lines |
| Crypto backend | LibTomCrypt initially, AmiSSL Phase 2.5 | Ship self-contained first, add AmiSSL for security updates later |
| Target CPU | 68020+ mandatory | Native 32x32->64 MULS.L critical for crypto performance |
| Preferred cipher | ChaCha20-Poly1305 | Add/xor/rotate only, no S-box table lookups, best 68k performance (no data cache on 68020) |
| Preferred key type | Ed25519 | ~5-10x faster than RSA-2048 key exchange on 68k |
| Entropy | Hybrid: ReadEClock + DateStamp + timing + optional seed file | Not crypto-grade without seed file, documented tradeoff for retro hobby use |
| Terminal I/O | Signal-based multiplexing | Console.device signal + WaitSelect signal mask, single Wait() for both |
| CSI translation | Shim-level module | Reusable for future terminal network programs (telnet, etc.) |
| Disconnect handling | TCP keepalive + amiport_check_break() | SO_KEEPALIVE via Roadshow, Ctrl-C in event loop |
| Feature strip | Aggressive | Smallest binary, least attack surface, features re-enabled in later revisions |

## Rationale

### Why Dropbear over OpenSSH

- **C89 compatible**: builds with bebbo-gcc without language issues
- **Self-contained crypto**: bundled LibTomCrypt/LibTomMath, no OpenSSL dependency
- **Designed for embedded**: minimal POSIX surface, runs on OpenWrt/BusyBox
- **Small codebase**: ~20-40K lines vs OpenSSH's ~120K+
- **Client-only build**: `PROGRAMS="dbclient"` avoids fork/pty/user-auth entirely
- OpenSSH requires fork(), pipe(), pty, setuid, privilege separation -- none of which exist on AmigaOS

### Why not libssh2 + AmiSSL (the SSHTerm pattern)

SSHTerm on AmigaOS 4 proves libssh2 + AmiSSL works, but:
- AmiSSL becomes a hard runtime dependency (program won't start without it)
- libssh2 is a library, not a complete client -- we'd write more glue code
- Dropbear is self-contained and simpler for the initial port
- Phase 2.5 adds AmiSSL as an optional enhancement anyway

### Why hybrid entropy

AmigaOS has no `/dev/urandom`, no kernel entropy source, no hardware RNG on classic hardware. The KB confirms this (known-pitfalls: "No /dev/urandom on AmigaOS"). Combined sources (ReadEClock microsecond timer + DateStamp ticks + user interaction timing + audio noise) fed into LibTomCrypt's Fortuna CSPRNG provide "good enough for a hobby machine on a home LAN" entropy. Optional `S:.ssh/entropy.dat` seed file allows hardening for the security-conscious. This tradeoff is honest and documented.

### Why ChaCha20 over AES on 68k

The 68020 has a 256-byte instruction cache but NO data cache. AES-CTR requires S-box table lookups (256-byte table) that hit main RAM on every access. ChaCha20 uses only add/xor/rotate operations with no table lookups. On 68k without data cache, ChaCha20 is measurably faster for bulk encryption.

## Success Criteria

1. `ssh user@host` from an Amiga shell opens an interactive remote session
2. Ed25519 key exchange completes in <3 seconds on Vampire V2
3. Arrow keys work in remote vim/nano/less (CSI/VT100 translation)
4. Ctrl-C cleanly disconnects with proper resource cleanup
5. Works with Roadshow and AmiTCP TCP/IP stacks
6. Password and pubkey authentication both work
7. Binary size under 500 KB (client only, without AmiSSL)
8. All automated tests pass on FS-UAE with host sshd

## Alternatives Considered

| Alternative | Verdict | Why rejected |
|-------------|---------|-------------|
| OpenSSH | Rejected | Too large (~120K lines), deeply POSIX-dependent, requires fork/pty/privilege separation |
| libssh2 + AmiSSL | Deferred to Phase 2.5 | Hard AmiSSL dependency, more glue code, SSHTerm proves it works but is OS4-only |
| Port existing AmigaSSH | Rejected | SSH 1.x only (deprecated protocol), non-commercial license |
| Port existing OpenSSH Amiga port | Rejected | Orphaned since ~2005, requires ixemul, outdated ciphers |
| Build from scratch | Rejected | SSH protocol is complex, no reason to reinvent when Dropbear exists |
| AmiSSL only (no LibTomCrypt) | Rejected initially | Hard runtime dependency prevents running on systems without AmiSSL. Added as Phase 2.5 enhancement |

## Estimated Effort

| Phase | Effort | Binary/Lib Size |
|-------|--------|----------------|
| Phase 0: Shim extensions | 1 session | ~5 KB lib growth |
| Phase 1: LibTomMath + LibTomCrypt | 2-4 sessions | ~200-400 KB .a |
| Phase 2: dbclient | 3-5 sessions | ~150-250 KB |
| Phase 2.5: AmiSSL backend | 1-2 sessions | ~10 KB glue |
| Phase 3: dropbearkey | 1 session | ~100-150 KB |
| **Total through Phase 3** | **8-13 sessions** | **~500-800 KB** |

## Parallelization

Phase 0 (shim extensions) and Phase 1a (LibTomMath) are independent and can run in parallel worktrees. Phase 1b (LibTomCrypt) depends on Phase 1a. Phase 2 depends on both Phase 0 and Phase 1b. Phase 3 depends on Phase 1b but not Phase 2.

## Open Questions

1. **OOM during crypto**: LibTomMath malloc failure during key exchange needs an error path. Investigate during Phase 1 implementation.
2. **Console handle loss**: What happens if the console window is closed mid-session? Needs investigation during Phase 2.
3. **Vampire hardware RNG**: Does the Vampire V2 AMMX extension provide any hardware randomness registers? Would improve entropy quality significantly. Research during Phase 2.
