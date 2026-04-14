# Portability Analysis: Dropbear SSH 2025.89 (dbclient only)

## Summary
- **Port category**: Network app (Category 4)
- Total source files: 125 (.c + .h)
- Lines of code: ~39,135 total
- Client-specific code: ~12 files (cli-*.c)
- Tier 1 (shim) issues: ~35 (moderate)
- Tier 2 (emulation) issues: ~8 (significant)
- Tier 3 (redesign) issues: ~6 (BLOCKING)
- Architecture issues: ~8 (requires attention)
- **Required libraries**: posix-shim + bsdsocket-shim + LibTomCrypt + LibTomMath + zlib (already bundled)
- **Link flags**: `-lamiport -lamiport-net -ltomcrypt -ltommath -lz -lm`
- **Test strategy**: FS-UAE + TCP/IP (real network stack required for SSH protocol testing)
- **Portability verdict**: **HARD** (multiple Tier 3 blockers, extensive POSIX dependencies)

**Binary size estimate**: 600-900 KB for minimal client (ChaCha20, Ed25519, RSA, no X11/agent fwd, no post-quantum)

## Key Architectural Challenges

### TIER 3 BLOCKERS (Human Redesign Required)

1. **Entropy Source (dbrandom.c) — COMPLETE REWRITE REQUIRED**
   - Current: `getrandom()` syscall (lines 166-206) → `/dev/urandom` fallback (line 258+)
   - AmigaOS: NO `/dev/urandom`, NO `getrandom()`, NO kernel entropy pool
   - **Fix**: Implement `amiport_getrandom()` using:
     - ReadEClock() (timer.device microsecond counter) — jitter entropy
     - DateStamp() (dos.library) — low-resolution time
     - Stack pointer address (ASLR-like)
     - LibTomCrypt Fortuna PRNG (already linked via our build) — stretch short seeds
   - Pattern: See known-pitfalls "No /dev/urandom on AmigaOS" for reference implementation
   - Impact: ~200 lines, affects ALL crypto operations
   - **CRITICAL**: Without this, SSH keys, nonces, and session keys are predictable

2. **SSH_ASKPASS fork/exec (cli-authpasswd.c:48-115) — DISABLE OR STUB**
   - Current: `fork()` + `execlp()` to run GUI password helper (lines 69-84)
   - AmigaOS: NO `fork()`, NO `exec*()`
   - **Fix**: Define `DROPBEAR_CLI_ASKPASS_HELPER 0` in localoptions.h (already default: 0)
   - Fallback: Direct `getpass()` from console (line 120+) works via `amiport/unistd.h`
   - Impact: GUI password prompt unavailable (acceptable — CLI-only)

3. **ProxyCommand fork/exec (dbutil.c:271-412) — DISABLE**
   - Current: `fork()` + `execv()` to run external proxy program (line 304, 412)
   - Used by: `dbclient -J <cmd>` (DROPBEAR_CLI_PROXYCMD)
   - **Fix**: Define `DROPBEAR_CLI_PROXYCMD 0` in localoptions.h
   - Also disables: `DROPBEAR_CLI_NETCAT` (depends on proxycmd, line 83 default_options.h)
   - Impact: No multihop via jump hosts — direct connections only

4. **Session select() Loop (common-session.c:160-240) — REDESIGN TO WaitSelect()**
   - Current: POSIX `select()` with fd_set on stdin/stdout/socket/signal_pipe (line 210)
   - AmigaOS: `select()` missing from libnix + bsdsocket-shim
   - **Fix**: Implement `amiport_select()` wrapper around bsdsocket.library `WaitSelect()`:
     - WaitSelect() signature: `LONG WaitSelect(LONG nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout, ULONG *signals)`
     - Extra param: signal mask for AmigaOS task signals (console.device CTRL-C)
     - Returns: count + sets signal mask on return
   - **Integration**: Signal pipe (line 95-102) maps to task signal instead of pipe()
   - Impact: ~150 lines shim code, affects main event loop
   - **CRITICAL**: Core of SSH client architecture

5. **SIGWINCH/SIGCHLD/SIGPIPE Signal Handling — REDESIGN TO POLLING**
   - Current signals used:
     - `SIGWINCH` (cli-chansession.c:300) — window size change → send SSH pty-req
     - `SIGPIPE` (cli-main.c:75, dbutil.c:402) — set to SIG_IGN
     - `SIGINT/TERM/HUP` (cli-main.c:86-88) — proxy kill handler
     - `SIGCHLD` (dbutil.c:316) — reaping forked processes
   - AmigaOS: NO POSIX signals except SIGABRT/SIGINT (polled, not async)
   - **Fix for SIGWINCH**:
     - Remove signal handler (line 300)
     - Poll `ioctl(TIOCGWINSZ)` in main loop → cache size → send on change
     - Or: Use console.device IDCMP messages (advanced)
   - **Fix for SIGPIPE**: Already ignored, safe no-op
   - **Fix for SIGINT/TERM/HUP**: Not applicable (no proxy, no fork)
   - **Fix for SIGCHLD**: Not applicable (no fork)
   - Impact: ~50 lines, affects terminal resizing only

6. **loginrec.c — ENTIRE FILE IS SERVER-ONLY (utmp/wtmp/lastlog)**
   - Lines: 1,847 (5% of codebase)
   - Uses: `<utmp.h>`, `<utmpx.h>`, `<lastlog.h>`, `getpwnam()`, `getpwuid()`, tty devices
   - **Fix**: Exclude from client build (not linked by cli-main.c)
   - Impact: Zero — already unused by dbclient

### TIER 2 EMULATION (Approximate Mappings)

| Function | File:Line | Shim Status | Caveats |
|----------|-----------|-------------|---------|
| `select()` | common-session.c:210 | **MISSING** — needs WaitSelect() wrapper | See Tier 3 item #4 above |
| `getaddrinfo()` | netio.c (pervasive) | **PRESENT** (bsdsocket-shim, Phase 0) | IPv4 only, wraps gethostbyname |
| `freeaddrinfo()` | netio.c | **PRESENT** (bsdsocket-shim) | Frees wrapper struct |
| `inet_ntop()` | netio.c | **PRESENT** (bsdsocket-shim) | IPv4 only, pure C |
| `inet_pton()` | netio.c | **PRESENT** (bsdsocket-shim) | IPv4 only, pure C |
| `fcntl(F_SETFL, O_NONBLOCK)` | netio.c, common-session.c | **PRESENT** (bsdsocket-shim) | IoctlSocket(FIONBIO) |
| `syslog()` | dbutil.c + 26 files | **MISSING** — stub or redirect to stderr | No syslog on AmigaOS, use `#define syslog(...) dropbear_log(...)` |
| `tcgetattr()/tcsetattr()` | cli-chansession.c:108,127 | **PRESENT** (amiport/termios.h) | Maps to console.device SetMode() — cooked/raw only |

### TIER 1 SHIM (Direct Mappings)

| Header | Usage Count | Replacement | Notes |
|--------|------------|-------------|-------|
| `<sys/socket.h>` | pervasive | `<amiport-net/socket.h>` | AF_INET only, no AF_UNIX/AF_INET6 |
| `<netdb.h>` | netio.c | `<amiport-net/netdb.h>` | gethostbyname via bsdsocket |
| `<arpa/inet.h>` | netio.c | `<amiport-net/arpa/inet.h>` | inet_ntop/pton shims |
| `<netinet/in.h>` | includes.h:78 | `<amiport-net/netinet/in.h>` | struct sockaddr_in |
| `<netinet/tcp.h>` | includes.h:92 | **STUB** — TCP_NODELAY macro only | setsockopt() not in bsdsocket-shim yet |
| `<sys/un.h>` | includes.h:37 | **EXCLUDE** — AF_UNIX not supported | Used by proxycmd (disabled) |
| `<termios.h>` | includes.h:50 | `<amiport/termios.h>` | tcgetattr/tcsetattr for RAW mode |
| `<pwd.h>` | includes.h:46 | **STUB** — `getpwuid()` returns NULL | Only used in loginrec.c (excluded) |
| `<grp.h>` | includes.h:44 | **STUB** — `getgroups()` returns -1/ENOSYS | Multiuser check (line 77) → stub for single-user |
| `<syslog.h>` | includes.h:52 | **MACRO REDIRECT** | `#define syslog(...) dropbear_log(...)` |
| `<sys/stat.h>` | includes.h:35 | `<amiport/sys/stat.h>` | stat/fstat shims |
| `<sys/time.h>` | includes.h:36 | `<amiport/sys/time.h>` | gettimeofday shim |
| `<sys/ioctl.h>` | includes.h:32 | **PARTIAL** — TIOCGWINSZ only | Via amiport/termios.h |
| `<sys/wait.h>` | includes.h:38 | **NOT NEEDED** — no fork | waitpid unused without proxycmd |
| `<sys/resource.h>` | includes.h:39 | **NOT NEEDED** | No getrlimit/setrlimit calls in client |
| `<sys/random.h>` | includes.h:128 | **EXCLUDE** — `getrandom()` unavailable | See Tier 3 entropy fix |
| `<inttypes.h>` | includes.h:96 | **PRESENT** (libnix) | int64_t typedef + format macros |
| `<libgen.h>` | includes.h:120 | **PRESENT** (libnix) | basename/dirname |
| `<setjmp.h>` | includes.h:58 | **PRESENT** (libnix) | Standard C |
| `<assert.h>` | includes.h:59 | **PRESENT** (libnix) | Standard C |
| `<dirent.h>` | includes.h:56 | `<amiport/dirent.h>` | opendir/readdir shim |

### Missing POSIX Functions (Needs Shim Extension or Stub)

| Function | File:Line | Recommendation |
|----------|-----------|---------------|
| `getgroups()` | common-session.c:77 | Stub: return -1, errno=ENOSYS (single-user check) |
| `pipe()` | common-session.c:95 | Remove: signal_pipe → task signal instead (see Tier 3 #4) |
| `setnonblocking()` | common-session.c:61 | Via fcntl shim (already present) |
| `monotonic_now()` | common-session.c:84 | Add to amiport/time.h: ReadEClock() wrapper |
| `getlogin()` | cli-runopts.c | Stub: return "amiga" (no multiuser) |
| `isatty()` | cli-authpasswd.c:42 | **PRESENT** (libnix) — fileno(stdin) check |
| `getenv()` | cli-authpasswd.c:40,57 | **PRESENT** (libnix) — use directly |
| `gettimeofday()` | dbutil.c | **PRESENT** (amiport/sys/time.h) |

## Architecture-Specific Issues (68k/bebbo-gcc)

| Issue | Severity | Location | Fix |
|-------|----------|----------|-----|
| **x86-64 inline asm** | BLOCKER | sntrup761.c:75-81,94-99 | `#ifdef __x86_64__` already present — fallback to C (line 82+, 100+) |
| **Large local arrays** | HIGH | sntrup761.c:1890 `int16_t fg[p+p-1]` where p=761 → 3044 bytes | Make `static` — single-threaded OK |
| **Large local arrays** | HIGH | sntrup761.c:2098-2122 `uint16_t R[p], M[p]` → 3044 bytes each | Make `static` |
| **Endianness** | LOW | sntrup761.c:42-70 | Explicit little/big-endian load/store functions — AmigaOS is big-endian, use `_bigendian` variants |
| **`__attribute__((__unused__))` pervasive** | SAFE | sntrup761.c:40+ | bebbo-gcc supports this GNU extension |
| **`static inline` pervasive** | SAFE | sntrup761.c | bebbo-gcc supports `__inline__` (equivalent) |
| **C99 features (variadic macros)** | SAFE | libcrux_mlkem768_sha3.h:34 `#define KRML_HOST_EPRINTF(...)` | Supported with `-std=gnu99` |
| **Floating point** | NONE | No FP usage detected in client code | Safe |

### Recommended Compiler Flags

```makefile
CFLAGS = -m68020 -O0 -noixemul -std=gnu99 \
         -DBUNDLED_LIBTOM=0 \
         -I../../lib/libtomcrypt/include \
         -I../../lib/libtommath \
         -I../../lib/posix-shim/include \
         -I../../lib/bsdsocket-shim/include

LDFLAGS = -L../../lib/libtomcrypt -ltomcrypt \
          -L../../lib/libtommath -ltommath \
          -L../../lib/zlib -lz \
          -L../../lib/posix-shim -lamiport \
          -L../../lib/bsdsocket-shim -lamiport-net \
          -lm
```

**Why `-std=gnu99`**: Dropbear uses `<inttypes.h>`, variadic macros, and expects `int64_t` (C99). The code is already C99-clean (no `for (int i` loops detected).

**Why `-O0`**: 68k struct-by-value return corruption risk (crash-patterns #16). Can promote individual scalar-only files to `-O1` after audit.

**Why `-m68020`**: Target is 68020+ (Vampire, A1200+030/040/060). No 68000 compatibility required per PDR-013.

## Required localoptions.h Defines

```c
/* localoptions.h — AmigaOS build configuration */

/* === DISABLE UNSUPPORTED FEATURES === */
#define DROPBEAR_CLI_PROXYCMD 0        /* No fork/exec for proxy */
#define DROPBEAR_CLI_NETCAT 0          /* Depends on proxycmd */
#define DROPBEAR_CLI_ASKPASS_HELPER 0  /* No fork/exec for GUI password */
#define DROPBEAR_SVR_MULTIUSER 0       /* Single-user OS */
#define DO_HOST_LOOKUP 0               /* No reverse DNS (slow) */
#define DO_MOTD 0                      /* No /etc/motd on AmigaOS */
#define DROPBEAR_DELAY_HOSTKEY 0       /* Client-only, no hostkey gen */
#define DROPBEAR_REEXEC 0              /* No fork for re-exec */

/* === DISABLE POST-QUANTUM (OPTIONAL — saves 43 KB) === */
#define DROPBEAR_SNTRUP761 0           /* ~9 KB, large stack locals */
#define DROPBEAR_MLKEM768 0            /* ~34 KB */

/* === DISABLE OPTIONAL ALGOS (SIZE SAVINGS) === */
#define DROPBEAR_3DES 0                /* Old, slow */
#define DROPBEAR_ENABLE_GCM_MODE 0     /* ~6 KB, slower than ChaCha20 on 68k */
#define DROPBEAR_SHA1_HMAC 0           /* Weak */
#define DROPBEAR_SHA2_512_HMAC 0       /* Slower than SHA256 on 32-bit */
#define DROPBEAR_DSS 0                 /* Weak, 1024-bit only */
#define DROPBEAR_RSA_SHA1 0            /* Weak */
#define DROPBEAR_DH_GROUP14_SHA1 0     /* Weak */
#define DROPBEAR_DH_GROUP16 0          /* 4096-bit, very slow */
#define DROPBEAR_DH_GROUP1 0           /* 1024-bit, weak */

/* === ENABLE RECOMMENDED FOR 68K === */
#define DROPBEAR_CHACHA20POLY1305 1    /* Faster than AES on 68k without AES instructions */
#define DROPBEAR_ENABLE_CTR_MODE 1     /* AES-CTR for compatibility */
#define DROPBEAR_AES128 1              /* Minimum compatibility */
#define DROPBEAR_AES256 1              /* Common default */
#define DROPBEAR_SHA2_256_HMAC 1       /* Fast, secure */
#define DROPBEAR_ED25519 1             /* Fastest signature */
#define DROPBEAR_RSA 1                 /* Universal compatibility */
#define DROPBEAR_ECDSA 1               /* Fast, ~30 KB */
#define DROPBEAR_CURVE25519 1          /* Fast KEX, ~2.5 KB */
#define DROPBEAR_ECDH 1                /* NIST curves (shared code with ECDSA) */
#define DROPBEAR_DH_GROUP14_SHA256 1   /* Compatibility KEX */
#define DROPBEAR_SK_KEYS 0             /* U2F security keys (server-only) */

/* === TCP FORWARDING === */
#define DROPBEAR_CLI_LOCALTCPFWD 1     /* -L flag */
#define DROPBEAR_CLI_REMOTETCPFWD 1    /* -R flag */
#define DROPBEAR_CLI_AGENTFWD 0        /* Agent forwarding disabled (no AF_UNIX) */

/* === ZLIB SETTINGS === */
#define DROPBEAR_ZLIB_WINDOW_BITS 8    /* 129 KB compression vs 256 KB at 15 */

/* === DEBUG === */
#define DEBUG_TRACE 0                  /* Disable debug output */

/* === AMIGAOS STUBS === */
#define syslog(...) dropbear_log(__VA_ARGS__)
```

**Size impact**: Minimal client: ~600 KB. Full algos + post-quantum: ~900 KB.

## Files to Exclude from Client Build

| File | Reason | Size Saved |
|------|--------|----------|
| `loginrec.c` | Server-only utmp/wtmp/lastlog | 1,847 lines |
| `compat.c` line 162 | daemon() function (uses fork) | ~50 lines — keep rest of file |
| All `svr-*.c` | Server-only (none in this checkout) | N/A |
| All `fuzz-*.c` | Fuzzing harness | ~500 lines |

**Note**: The `original/` directory appears to be client-focused already (all `cli-*.c`, no `svr-*.c`). This is good.

## Non-ASCII Check

```bash
grep -rPIl '[^\x00-\x7F]' ports/dropbear/original/*.c
```
Result: **NONE FOUND** — source is pure ASCII (checked manually).

## Estimated Porting Effort

| Phase | Task | Effort | Risk |
|-------|------|--------|------|
| 1 | Create localoptions.h + disable unsupported features | 2 hours | LOW |
| 2 | Implement amiport_getrandom() (entropy source) | 8 hours | HIGH — crypto-critical |
| 3 | Implement amiport_select() wrapper (WaitSelect) | 12 hours | HIGH — core event loop |
| 4 | Remove signal_pipe, replace with task signals | 4 hours | MEDIUM |
| 5 | Stub SIGWINCH → poll ioctl(TIOCGWINSZ) | 3 hours | LOW |
| 6 | Stub syslog, getgroups, multiuser checks | 2 hours | LOW |
| 7 | Make large stack arrays static (sntrup761.c, mlkem768.c) | 1 hour | LOW |
| 8 | Test build against bsdsocket-shim + LibTomCrypt/Math | 4 hours | MEDIUM |
| 9 | Create test suite (FS-UAE + real TCP/IP) | 16 hours | HIGH — needs SSH server |
| 10 | Memory audit + perf optimization | 8 hours | MEDIUM |
| **Total** | | **~60 hours** (1.5 weeks) | |

## Recommended Approach

### Phase 1: Foundation (Days 1-2)
1. Set up port directory structure
2. Create `localoptions.h` with all disables/stubs
3. Copy source, create Makefile linking against existing libs
4. **Verify** clean compile with everything stubbed

### Phase 2: Entropy + Event Loop (Days 3-6) — CRITICAL PATH
5. Implement `amiport_getrandom()` in `lib/posix-shim/src/random.c`
   - Use LibTomCrypt Fortuna PRNG (already have LTC)
   - Seed from ReadEClock, DateStamp, stack address
6. Implement `amiport_select()` → `WaitSelect()` wrapper
   - Handle signal mask integration
   - Test with simple echo server first
7. Redesign signal_pipe → task signals
   - Console.device CTRL-C via WaitSelect signals param

### Phase 3: Terminal + Integration (Days 7-8)
8. Verify `tcgetattr/tcsetattr` works via existing amiport/termios.h
9. Stub remaining functions (syslog, getgroups, etc.)
10. Fix large stack arrays in sntrup761.c/mlkem768.c

### Phase 4: Testing (Days 9-10)
11. Build test SSH server (OpenSSH on Linux VM or dropbear server on host)
12. FS-UAE config with AmiTCP/Roadshow
13. Test connect, auth (password + pubkey), command execution
14. Test algorithm negotiation (ChaCha20, Ed25519, RSA)

### Phase 5: Hardening (Days 11-12)
15. Memory audit (atexit cleanup, leak detection)
16. Perf optimization (identify hot loops, consider per-file -O1)
17. Real hardware test (A2000 + Roadshow + X-Surf 100)

## Verdict: HARD

**Reasons:**
- **6 Tier 3 blockers** requiring architectural redesign (entropy, select loop, signals, fork elimination)
- **Heavy POSIX dependency** (sockets, select, signals, terminal I/O)
- **Cryptographic critical path** — entropy bugs = key compromise
- **Event loop redesign** — core of SSH protocol handling
- **Real network testing required** — cannot fake SSH protocol in vamos

**Mitigations:**
- LibTomCrypt/Math already ported + tested (Phase 1 complete)
- bsdsocket-shim extended in Phase 0 (getaddrinfo, inet_ntop/pton, fcntl)
- zlib already bundled in lib/zlib/
- termios shim exists (amiport/termios.h)
- No GUI, no X11, no pty — client is simpler than server

**Why not INFEASIBLE:**
- No threading (single-threaded event loop)
- Fork/exec usage is narrow (proxycmd — can disable)
- Select loop has clean bsdsocket.library equivalent (WaitSelect)
- Entropy can be approximated (not perfect but adequate for SSH client)

**Comparison to prior ports:**
- **Harder than**: jq (no network), awk (no crypto), mg (local I/O only)
- **Similar to**: wget (network + crypto, but wget is HTTP-only, simpler protocol)
- **Easier than**: Full CPython (no fork, smaller scope, no dynamic loading)

**Recommended next step:** Dispatch `dependency-auditor` to verify LibTomCrypt/Math integration assumptions, then proceed to Phase 1 setup.

## Learnings

- [PROCESS] Dropbear's bundled sntrup761/mlkem768 post-quantum implementations use large stack locals (3KB+ arrays). These should be flagged during source-analyzer and either made static or excluded via localoptions.h. The agent should explicitly grep for `int16_t.*\[[0-9]+\]` patterns in crypto code.
- [PITFALL] WaitSelect() is NOT a drop-in replacement for select() — it has an extra signal mask parameter that must be integrated with AmigaOS task signals for CTRL-C handling. The source-analyzer should note this when detecting select() usage in main loops.
- [PROCESS] When analyzing a configurable codebase (Dropbear has localoptions.h), the agent should list BOTH "what's required" AND "what can be disabled" in separate tables — the latter often provides the path to HARD → MODERATE by eliminating Tier 3 blockers.
