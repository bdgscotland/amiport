# Port: wget

## Overview

| Field | Value |
|-------|-------|
| Program | wget |
| Version | 1.20.3 (port revision: 1) |
| Source | https://ftp.gnu.org/gnu/wget/wget-1.20.3.tar.gz (GNU wget 1.20.3, Nov 2018) |
| Category | 4 — Network |
| License | GPL-3.0-or-later |
| Original Author | Free Software Foundation |
| Port Date | 2026-04-05 |

## Description

GNU Wget is a free utility for non-interactive download of files from the Web. It supports HTTP, HTTPS (via AmiSSL), and FTP protocols, with features including recursive downloading, conversion of links, resume of interrupted transfers, cookies, authentication, and proxy support.

## Prior Art on Aminet

An old wget 1.8.2 (2003) exists on Aminet in `comm/tcp` but requires `ixemul.library` — a heavy Unix emulation layer adding ~500KB overhead. The known "wget + bsdsocket emu = crash" issue on EAB forums suggests compatibility problems. A source-only wget 1.9.1 (2006) was also uploaded but never compiled for 68k. An unmaintained PPC/MorphOS effort exists on GitHub (diegocr/wget-amiga, 2013). Our port provides a modern version (1.20.3) with native AmigaOS integration, no ixemul dependency, and proper bsdsocket.library support.

## Portability Analysis

Verdict: **MODERATE** (core HTTP/FTP) — **COMPLEX** (full feature set with AmiSSL)

35 source files (~53K lines) + 25 gnulib support files. Category 4 network port requiring bsdsocket-shim. Phase 1: HTTP-only. Phase 2: HTTPS via AmiSSL 5.26.

| Issue | Tier | Resolution |
|-------|------|------------|
| BSD socket calls (socket/connect/send/recv/select) | 1 | bsdsocket-shim |
| DNS resolution (gethostbyname) | 1 | bsdsocket-shim (IPv6/getaddrinfo disabled) |
| Signal handling (SIGPIPE/SIGHUP/SIGWINCH) | 1 | amiport_signal stubs; signals compile out via #ifdef guards |
| File operations (stat/unlink/rename/mkdir) | 1 | posix-shim |
| Terminal size (ioctl TIOCGWINSZ) | 1 | amiport_ioctl |
| getopt_long | 1 | amiport/getopt.h (libnix broken — crash-patterns #17) |
| POSIX regex (regcomp/regexec) | 2 | gnulib regex bundled |
| run_with_timeout (SIGALRM+setjmp) | 3 | No-timeout stub (same as Windows path) |
| fork_to_background (fork+setsid) | 3 | Stub with "use Run >NIL:" message |
| run_use_askpass (posix_spawn+pipe) | 3 | Disabled; use --password instead |
| OpenSSL/TLS (Phase 2) | Special | AmiSSL 5.26 shared library |

## Transformations Applied

*To be filled by code-transformer agent*

## Shim Functions Exercised

- amiport_socket(), amiport_connect(), amiport_bind(), amiport_listen(), amiport_accept()
- amiport_send(), amiport_recv(), amiport_closesocket()
- amiport_gethostbyname(), amiport_setsockopt()
- amiport_net_select() / WaitSelect()
- amiport_stat(), amiport_lstat(), amiport_fstat()
- amiport_gettimeofday()
- amiport_signal() (SIGPIPE, SIGHUP, SIGWINCH stubs)
- amiport_getenv() (HOME, WGETRC, etc.)
- amiport_getpwuid() (home directory fallback)
- amiport_fnmatch() (accept/reject patterns)
- amiport_ioctl() (terminal window size)
- amiport_getopt_long() (command line parsing)
- amiport_check_break() (Ctrl-C support)
- amiport_expand_argv() (AmigaOS wildcard expansion)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68020+ |
| CFLAGS | `-std=gnu99 -O2 -noixemul -m68020 -Wall` |
| Libraries | `-lamiport` (posix-shim), `-lamiport-net` (bsdsocket-shim) |
| Binary size | 527KB (stripped) |

## Test Results

**vamos smoke tests (no networking):**

| Test | Command | Expected | Result |
|------|---------|----------|--------|
| Version | `wget --version` | "GNU Wget 1.20.3 built on AmigaOS." | PASS |
| Help | `wget --help` | Help text with options | PASS |
| No args | `wget` | "wget: missing URL" | PASS |
| Bad option | `wget --invalid` | "unrecognized option" | PASS |
| DNS fail | `wget http://example.com/` | Graceful DNS error | PASS |

**FS-UAE network testing:** Pending — requires Roadshow TCP/IP stack.

**Test suite:** 65 TEST + 3 ITEST blocks in test-fsemu-cases.txt.

## Platform Compatibility Notes

- **68020 minimum**: Required for AmiSSL 5.26 (Phase 2). Phase 1 HTTP could run on 68000 but we target 68020 for consistency.
- **wgint = long long**: 64-bit file size counters emulated via 68k multiply/divide sequences. Acceptable for network I/O dominated workload.
- **No SIGALRM timeouts**: Connections to unreachable hosts block until TCP stack timeout (~30-60s). Same trade-off as Windows wget.
- **Stack**: `__stack = 65536` — wget has deep call chains through HTTP/FTP/HTML parsing.

## Known Limitations

- `--background` (`-b`): Not supported. Use `Run >NIL: wget [URL]` instead.
- `--use-askpass`: Not supported. Use `--password` instead.
- `--timeout`: No per-operation timeouts (no SIGALRM). TCP-level timeouts still work via select().
- No gzip transfer encoding (zlib disabled). Servers will send uncompressed.
- No internationalized domain names (libidn2 disabled). ASCII URLs only.
- English only (NLS disabled).
- NTLM proxy auth disabled.
- WARC output: No compression, no UUID (uses timestamp-based IDs).
- Phase 1: HTTP and FTP only. Phase 2 adds HTTPS via AmiSSL.

## Memory Safety

**Critical fix applied:** Upstream wget's `cleanup()` function is gated behind `#if defined DEBUG_MALLOC || defined TESTING` — meaning ALL memory cleanup is disabled in production builds. On AmigaOS with `-noixemul`, this means permanent leaks until reboot.

**Fix:** Removed the `#ifdef` guards from cleanup() in init.c. All 50+ xfree() calls and subsystem cleanup functions (host_cleanup, http_cleanup, cookie cleanup, etc.) now always run on exit. Also registered `atexit(cleanup)` in main.c to ensure cleanup runs on all exit paths including err()/errx().

## Performance Notes

wget is network I/O bound. The bottleneck is Ethernet throughput (10Mbit typical on X-Surf 100), not CPU. No critical 68k performance issues expected — the download loop uses buffered I/O and the progress bar updates are infrequent.

## Knowledge Capture

- **GNU optind=0 restart extension**: Added to known-pitfalls.md. wget uses a two-pass getopt_long pattern with optind=0 reset. Required adding the GNU extension to amiport_getopt/amiport_getopt_long in lib/posix-shim/src/getopt.c.
- **cleanup() gated on DEBUG_MALLOC**: Common GNU pattern where production builds skip cleanup. Must always ungated for AmigaOS -noixemul ports. Added to PORT.md.

## Review

*To be filled by review-amiga*
