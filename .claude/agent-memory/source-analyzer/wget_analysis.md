---
name: wget_analysis
description: GNU wget 1.20.3 portability analysis for AmigaOS 3.x - MODERATE/HARD verdict, Category 4 Network, run_with_timeout SIGALRM redesign, fork_to_background stub, posix_spawn pipe redesign, bsdsocket shim, AmiSSL
type: project
---

# wget 1.20.3 portability analysis

**Verdict:** MODERATE/HARD
**Category:** 4 (Network)
**Recommended shims:** posix-shim, bsdsocket-shim, AmiSSL

## Key blockers

1. **run_with_timeout** (utils.c:2044-2188) - SIGALRM+setitimer+sigsetjmp timeout system used for DNS (host.c:346,409), TCP connect (connect.c:245), SSL connect (openssl.c:677). Tier 3 redesign: replace with timer.device cooperative polling since SIGALRM cannot interrupt blocking calls.

2. **fork_to_background** (utils.c:497-523) - fork()+setsid() daemon pattern. Tier 3: stub with "Use Run >NIL: wget ..." message and disable --background flag. On AmigaOS, user launches with Run.

3. **posix_spawn --use-askpass** (main.c:1100-1157) - posix_spawn_file_actions + posix_spawnp + pipe for --use-askpass. Tier 3: disable ENABLE_ASKPASS or stub run_use_askpass() to always return NULL.

4. **wgint as long long** (wget.h:144-147) - On 68020, SIZEOF_LONG=4, SIZEOF_LONG_LONG=8, so wgint typedef'd to `long long`. Used pervasively for file sizes and counters. Compiles with -std=gnu99 but has performance penalty. All file size display uses number_to_string() custom formatter (ok for 32-bit path).

5. **getopt_long** (main.c) - needs #include <amiport/getopt.h> per crash-patterns #17

## Tier 1 (shim - automated)
- socket/connect/bind/listen/accept/send/recv/select - bsdsocket-shim (WaitSelect for select on sockets)
- gethostbyname/getaddrinfo/freeaddrinfo - bsdsocket-shim
- getsockname/setsockopt/getsockopt - bsdsocket-shim
- stat/lstat/fstat/unlink/rename/mkdir/access/getcwd/chdir - posix-shim
- utime() for -N timestamping - amiport_utimensat
- lstat+readlink+symlink in ftp.c - lstat=amiport_lstat, symlink=amiport_symlink, readlink=Tier 2
- gettimeofday - amiport_gettimeofday (used by PTIMER_GETTIMEOFDAY path; define _POSIX_TIMERS=0 to use gettimeofday fallback)
- nanosleep/usleep/sleep - posix-shim
- signal(SIGINT/SIGPIPE/SIGTERM) - amiport_signal (SIGPIPE, SIGHUP, SIGUSR1, SIGWINCH all stub to SIG_IGN)
- sigaction/sigemptyset/sigaddset/sigprocmask - posix-shim (no-op stubs for non-SIGINT)
- getenv/setenv - posix-shim
- getpid - posix-shim
- getuid/getpwuid - posix-shim (returns uid=0, struct passwd with "amiga")
- fnmatch - posix-shim
- regex (posix mode) - posix-emu (BRE/ERE)
- tmpfile - amiport_tmpfile (T: assign)
- getpass - libnix provides getpass() already (reads from console)
- ioctl(TIOCGWINSZ) for screen width - amiport_ioctl already handles this
- isatty - posix-shim

## Tier 2 (emulation - semi-automated)
- pipe() in run_use_askpass - posix-emu pipe via PIPE: device (but whole function should be stubbed)
- mmap() - not actually used in wget src (only mentioned in comments as "#ifdef HAVE_MMAP" and never compiled in)
- readlink() in ftp.c:2329 - no AmigaOS equivalent, returns -1/EINVAL

## Tier 3 (redesign required)
- run_with_timeout: SIGALRM+setjmp timeout - replace with NO-TIMEOUT stub version (utils.c:2192-2204 already has `#ifndef WINDOWS` stub). Define `-DUSE_SIGNAL_TIMEOUT=0` or set _POSIX_TIMERS to force gettimeofday path AND define no-timeout fallback. Without real timeout, DNS hangs are possible but wget will still work.
- fork_to_background: Remove entirely, print "Use Run >NIL: wget" message  
- posix_spawn/pipe: Disable --use-askpass feature (stub run_use_askpass)
- setsid(): Remove (part of fork_to_background)
- SIGHUP/SIGUSR1/SIGWINCH: Stub signal handlers to no-ops

## Architecture issues
- static inline (11 uses) - replace with static __inline__ or static
- `_Noreturn` keyword (C11) - replace with __attribute__((noreturn)) or remove
- `__attribute__((no_sanitize("integer")))` on hash.c:639,680,721 and hsts.c:88 - remove (harmless if stripped)
- long long (wgint) pervasive - compiles with -std=gnu99, performance acceptable
- intmax_t in utils.c:1792 ("%j" format) - only reached on non 4/8-byte wgint; on AmigaOS wgint=long long so SIZEOF_WGINT=8, this branch not taken
- static inline in ptimer.c:166,172,179,199,205 - must become static

## Features to disable at configure time
- ENABLE_NLS/gettext: already guarded by #ifdef
- HAVE_LIBIDN2: disable, iri.c can be compiled without idn2.h
- HAVE_LIBPSL: disable
- HAVE_METALINK: disable
- HAVE_LIBCARES (c-ares): disable (uses ares_gethostbyname via select loops)
- HAVE_LIBZ: disable (WARC compression)
- HAVE_LIBPCRE/HAVE_LIBPCRE2: disable (use posix regex only)
- ENABLE_XATTR: disable
- ENABLE_ASKPASS: disable (posix_spawn)
- HAVE_UUID_CREATE/HAVE_LIBUUID: use gnulib's tmpdir-based uuid substitute for WARC
- HAVE_RAND_EGD: disable (uses RAND_egd OpenSSL API)

## AmiSSL integration strategy
- openssl.c uses OpenSSL 1.1/3.x API: SSL_CTX_new, SSL_new, SSL_connect, SSL_read, SSL_write, BIO_*, ERR_*, X509_*
- AmiSSL 5.26 provides OpenSSL 3.6.1 API via shared library - headers in AmiSSL SDK
- Replace: #include <openssl/*.h> -> #include <inline/amissl.h> + <proto/amissl.h>
- All SSL_* calls go through AmiSSL function table - source-level compatible
- init_prng: AmiSSL handles entropy internally on AmigaOS - RAND_status() will return 1
- Connection to RAND_file_name: use getenv("HOME") path

## select() on sockets
- connect.c select_fd() called with socket fds
- Must use bsdsocket WaitSelect() not POSIX select() for socket fds
- bsdsocket-shim WaitSelect wraps this correctly

## wgint (long long) impact
- On AmigaOS 68020+, wgint = long long (64-bit)
- number_to_string uses SIZEOF_WGINT==8 path with custom digit printing (no sprintf)
- str_to_wgint = strtoll (libnix has strtoll)
- WGINT_MAX = TYPE_MAXIMUM(long long) from gnulib intprops.h

## gnulib modules status
- sha1.c, sha256.c, md4.c, md5.c: pure C, portable - compile fine
- c-ctype.c, c-strcase*.c: pure C, compile fine
- base32.c: pure C, fine
- dirname.c, basename.c: pure C, fine (but note libnix dirname bug - use local copy)
- quotearg.c, quote.h: pure C, fine
- regex.h (gnulib): superseded by posix-emu regex - use posix-emu
- spawn-pipe.c: NOT directly called by wget src (only --use-askpass stub will need it; stub entire feature)
- fatal-signal.c: handles SIGINT/SIGTERM/SIGHUP - most signals no-op on AmigaOS; fine
- tempname.c/tmpdir.c: may use /tmp - redirect to T:
- getprogname.c: will conflict with amiport_expand_argv - use shim version

## Stub value warnings
- getpid() used in random_number() seed: always returns same value if called twice in same run - timing XOR will vary, acceptable
- getuid() returns 0: init.c:548 uses getpwuid(getuid()) to find home dir - amiport_getpwuid returns pw_dir="SYS:" which is reasonable fallback

## Summary
- Category 4 Network port
- Requires bsdsocket-shim for all socket operations
- Requires AmiSSL 5.26 SDK for HTTPS
- run_with_timeout needs redesign (use no-timeout stub, DNS/connect may hang but still work)
- fork_to_background disable (stub --background)
- posix_spawn disable (stub --use-askpass)
- getopt_long needs amiport/getopt.h
- static inline -> static, _Noreturn -> attribute, no_sanitize -> strip
- wgint=long long compiles with -std=gnu99

## Confidence
MODERATE confidence. Core HTTP/FTP download will work. Timeouts won't interrupt hanging connections (structural limitation). Background mode not possible. Password helper programs not possible.
