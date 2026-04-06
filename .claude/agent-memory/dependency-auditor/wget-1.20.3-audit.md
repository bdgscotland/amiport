---
name: wget-1.20.3 dependency audit
description: Full dependency classification for GNU wget 1.20.3 targeting AmigaOS 3.x/m68k with AmiSSL for HTTPS
type: project
---

Audited 2026-04-05. Source at ports/wget/original/src/ (35 .c files) and ports/wget/original/lib/ (55 .c files gnulib).

## OpenSSL / AmiSSL

Classification: Available (AmiSSL 5.26 via amisslmaster.library)
wget's openssl.c uses API up to OpenSSL 1.1.x (highest guard: 0x10100000L). AmiSSL 5.26 provides OpenSSL 3.6.1 API which is backward compatible. All version guards will evaluate to "true" (modern path), which is correct.

Key APIs used: SSL_CTX_new, SSL_new, SSL_set_fd, SSL_connect, SSL_read, SSL_write, SSL_free, SSL_CTX_free, X509 cert verification chain, BIO_new/BIO_read/BIO_free, RAND_status, RAND_load_file, OPENSSL_init_ssl, SSL_CTX_set_min_proto_version, TLS_client_method, SSL_CTX_set_cipher_list, SSL_CTX_load_verify_locations, X509_VERIFY_PARAM, X509_STORE_add_lookup, ERR_error_string.

Integration notes:
- Include <amissl/amissl.h> instead of individual <openssl/*.h> headers
- Link with -lamisslauto (auto-opens amisslmaster.library via autoinit)
- Call InitAmiSSL() + CleanupAmiSSL() in ssl_init() / ssl_cleanup() with #ifdef __AMIGA__ guards
- FD_TO_SOCKET(fd): openssl.c already has a #define FD_TO_SOCKET(X) (X) fallback at line 660
  This is correct -- SSL_set_fd() receives the bsdsocket fd number directly, which IS what bsdsocket-shim socket() returns
- NTLM: http-ntlm.c uses OpenSSL DES/MD4 (openssl/des.h, openssl/md4.h). These are legacy APIs deprecated in OpenSSL 3.x. AmiSSL 5.x may or may not include them. Safer: compile WITHOUT ENABLE_NTLM (NTLM is only needed for Windows NTLM proxy auth).
- Engine support (OPENSSL_NO_ENGINE guard): may not be present in AmiSSL. Guard with #ifdef OPENSSL_NO_ENGINE in wget config.

## bsdsocket.library (TCP/IP)

Classification: Available (lib/bsdsocket-shim)
bsdsocket-shim covers: socket, connect, bind, listen, accept, closesocket, send, recv, setsockopt, getsockopt, shutdown, gethostbyname, gethostbyaddr, inet_addr, inet_ntoa, inet_aton.

GAPS requiring code-transformer work:
1. select() -- bsdsocket-shim has amiport_net_select() but no select() alias. connect.c calls select(fd+1, &fdset, NULL, NULL, &tmout) with standard struct fd_set / struct timeval. Solution: add #define select() alias or add a select() wrapper that bridges standard fd_set to amiport_net_fd_set in the port's config.
2. read()/write() on socket fds -- sock_read/sock_write in connect.c use raw read()/write() which call libnix's Read() (AmigaDOS file), not Recv()/Send() (bsdsocket). Solution: replace read(socketfd, buf, n) with recv(socketfd, buf, n, 0) and write(socketfd, buf, n) with send(socketfd, buf, n, 0) in sock_read/sock_write. These 2 functions are the only place this matters.
3. close() on socket fds -- connect.c calls close(socket_fd) in sock_close(). amiport_close() (from posix-shim) does NOT route to CloseSocket(). Solution: replace close(fd) with closesocket(fd) in sock_close(), or add socket-awareness to amiport_close(). The #undef close at line 814 complicates this but sock_close() has direct access to call closesocket() by name.
4. No select() macro when AMIPORT_NET_MACROS is defined. Must add: #define select(n,r,w,e,t) amiport_net_select_compat(n,r,w,e,t) with a wrapper that converts struct fd_set/timeval to amiport types.

NOT NEEDED: getaddrinfo/freeaddrinfo/gai_strerror -- wget uses these only under #ifdef ENABLE_IPV6. Compile without IPv6 (no ENABLE_IPV6 define) and the #ifndef ENABLE_IPV6 path uses gethostbyname only. bsdsocket-shim covers gethostbyname.

## POSIX regex (regcomp/regexec/regfree/regerror)

Classification: Portable (must bundle)
libnix libc.a does NOT provide regcomp/regexec. wget uses POSIX regex as its default (not PCRE2). The gnulib lib/ directory provides regex.h + regex_internal.h but NO regex.c -- wget's autoconf downloads it. Solution: download gnulib regex.c (available from https://git.savannah.gnu.org/cgit/gnulib.git/plain/lib/regex.c) and place it in ports/wget/original/lib/. It is portable ANSI C with wchar/locale paths that can be disabled with -DMB_CUR_MAX=1. About 3800 lines, no external deps.

## getaddrinfo (for IPv6 path)

Classification: Optional -- disable by not defining ENABLE_IPV6.
bsdsocket.library (Roadshow) does NOT provide getaddrinfo() on classic AmigaOS 3.x -- it's not part of the AmiTCP/IP API. Building without ENABLE_IPV6 eliminates the entire getaddrinfo code path in host.c. Use #ifndef ENABLE_IPV6 path throughout.

## gettimeofday / struct timeval

Classification: Available (posix-shim) but API mismatch
posix-shim provides amiport_gettimeofday() with struct amiport_timeval (not struct timeval).
connect.c uses struct timeval for select() timeout. Solution: ensure the bsdsocket-shim select() bridge converts struct timeval -> amiport_net_timeval. Or: provide a proper gettimeofday() macro in the port that uses the correct struct. ptimer.c will use PTIMER_GETTIMEOFDAY path on AmigaOS (since no _POSIX_TIMERS).

## POSIX signals: SIGALRM / alarm() / run_with_timeout

Classification: Optional -- use stub timeout path
posix-shim does NOT define SIGALRM or alarm(). wget's run_with_timeout uses SIGALRM when USE_SIGNAL_TIMEOUT is defined (which requires HAVE_SIGSETJMP or HAVE_SIGBLOCK). Without either, it compiles the stub version at line 2197 that just calls fun(arg) with no timeout enforcement. Compile with -UUSE_SIGNAL_TIMEOUT (or don't define HAVE_SIGSETJMP/HAVE_SIGBLOCK). Effect: --timeout option accepted but not enforced for DNS. HTTP/TCP timeouts via select() still work.

## iconv / IRI (internationalized domain names)

Classification: Optional -- disable with -UHAVE_ICONV
iri.c uses iconv_open/iconv/iconv_close under #ifdef HAVE_ICONV. AmigaOS has no iconv. Compile without HAVE_ICONV. wget still handles ASCII domain names. URL parsing and ASCII HTTP still work fully. Users can't specify non-ASCII hostnames, which is acceptable for classic Amiga.

## zlib (HTTP content-encoding, WARC compression)

Classification: Optional -- disable initially
Used under #ifdef HAVE_LIBZ in retr.c (HTTP gzip decompression), http.c (Accept-Encoding header), and warc.c (WARC gzip). Without it: wget downloads gzip-encoded responses as raw gzip bytes (doesn't decompress). In practice most servers send uncompressed if client doesn't request gzip, and wget won't send Accept-Encoding: gzip without HAVE_LIBZ. WARC is also disabled. z.library exists on Amiga as a shared library; static zlib can be added as a second-pass enhancement.

## wchar_t / wcwidth / mbtowc in progress.c

Classification: Optional -- progress bar ASCII fallback
progress.c includes <wchar.h> and uses wchar_t, mbtowc(), wcwidth() for the progress bar column width calculation. On AmigaOS, bebbo-gcc has wchar_t and mbtowc() in libnix. wcwidth() is NOT available. The MB_CUR_MAX crash-patterns #11 issue applies: MB_CUR_MAX expands to a runtime function call. Solution: add #ifdef __AMIGA__ guard around the wcwidth path in progress.c, use a byte-count fallback (bytes = 1 per character for ASCII). Progress bar will render correctly for ASCII filenames.

## flock() in hsts.c

Classification: Optional stub
hsts.c calls flock(fd, LOCK_EX) for the HSTS cache file. AmigaOS has no flock(). Stub as: #define flock(fd, how) (0). AmigaOS is single-user, single-process, so file locking is unnecessary. HSTS caching still works (just without the advisory lock).

## fork() / fork_to_background()

Classification: Optional stub
fork_to_background() is called in main.c when --background is used. AmigaOS has no fork(). Stub fork_to_background() to return false (pretend backgrounding failed) and print a message. The --background flag will be accepted but will run in foreground. This is minor -- AmigaOS users can use Run/RunBack from the shell instead.

## libuuid (WARC record IDs)

Classification: Optional -- WARC disabled
warc.c uses uuid under #ifdef HAVE_LIBUUID (or HAVE_UUID_CREATE). Since WARC is already disabled (no #define HAVE_WARC or equivalent), this is a non-issue. But even if WARC is enabled, libuuid is not available on AmigaOS. Use rand()-based UUID generation stub.

## gettext / NLS

Classification: Optional -- compile with -DNLS=0
Define #define _(x) (x) and #define N_(x) (x) in wget.h or config. gettext.h from gnulib already handles this when ENABLE_NLS is not defined.

## libpcre2 / PCRE

Classification: Optional -- use POSIX regex default
PCRE2/PCRE are optional alternatives. Don't define HAVE_LIBPCRE or HAVE_LIBPCRE2. Use POSIX regex (bundled gnulib regex.c) as default. wget's regex type defaults to "posix" when PCRE2 is absent.

## c-ares (async DNS)

Classification: Optional -- not used
Don't define HAVE_LIBCARES. wget falls back to synchronous gethostbyname. Fine for AmigaOS where all networking is blocking anyway.

## metalink / gpgme

Classification: Optional -- not compiled
metalink.h is just a header stub. No metalink.c in src/. Only compiled when HAVE_METALINK is defined. Don't define it. gpgme same: only active with HAVE_GPGME. Skip both.

## xattr

Classification: Optional -- compile without USE_XATTR
xattr.c is fully guarded with #ifdef USE_XATTR. Don't define it. Amiga filesystems (OFS/FFS/SFS) don't support extended attributes.

## gnulib modules assessment

Portable (compile normally):
- c-ctype.c, c-strcasecmp.c, c-strcasestr.c, c-strncasecmp.c -- pure ASCII, no deps
- base32.c, md2.c, md4.c, md5.c, sha1.c, sha256.c, sha512.c -- portable hash functions
- xmalloc.c, xsize.c, xstrndup.c -- portable stdlib wrappers
- quotearg.c, stripslash.c, getprogname.c, exitfail.c -- portable
- malloca.c, timespec.c, stat-time.c, gettime.c, utimens.c -- portable with minor shims
- dirname.c, basename.c -- use libnix's basename, but libnix dirname is BUGGY (crash-patterns). Use the gnulib bundled dirname.c instead.
- sockets.c, sys_socket.c -- these are gnulib portability stubs for Windows; EXCLUDE from Amiga build (they conflict with bsdsocket)
- localcharset.c -- needs glthread stub (see below)
- vasnprintf.c equivalent -- uses unlocked-io.h macros; should be fine
- mbchar.c, mbiter.c -- need MB_CUR_MAX guard (crash-patterns #11)

Stub/exclude:
- wait-process.c -- uses waitpid/waitid (no fork/process on AmigaOS). Only used by spawn-pipe.c which is not called by wget src. Exclude from Amiga build.
- spawn-pipe.c -- uses posix_spawn/fork. Not called by wget src. Exclude.
- fatal-signal.c -- uses signal() only (covered by posix-shim). Safe to compile.
- glthread/lock.c, glthread/threadlib.c -- uses pthread. If USE_POSIX_THREADS is not defined (which it won't be), lock.c compiles to no-ops via the #if USE_POSIX_THREADS guard. Add -UUSE_POSIX_THREADS to CFLAGS. The threadlib.c glthread_in_use() returns 0 in non-pthread build.

## select() bridging -- action required

This is the most non-obvious gap. wget's connect.c uses POSIX select() with:
- Standard struct fd_set (not amiport_net_fd_set)
- Standard struct timeval (not amiport_net_timeval)  
- FD numbers are bsdsocket fds

The bsdsocket-shim's amiport_net_select() uses amiport_net_fd_set. These types are different.

Recommended fix: Add a select() compatibility wrapper in the port's amiga-compat.h:

  #include <amiport-net/socket.h>
  static inline int select_compat(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t) {
    /* Convert standard fd_set to amiport_net_fd_set and call amiport_net_select() */
    ...
  }
  #define select(n,r,w,e,t) select_compat(n,r,w,e,t)

Or: extend lib/bsdsocket-shim to add a POSIX-compatible select() wrapper that handles standard fd_set.

## Required config.h defines for Amiga build

  #define PACKAGE "wget"
  #define VERSION "1.20.3"
  #define HAVE_LIBSSL 1        /* AmiSSL */
  #undef ENABLE_NLS
  #define _(x) (x)
  #undef ENABLE_IPV6
  #undef HAVE_ICONV
  #undef HAVE_LIBZ
  #undef USE_XATTR
  #undef HAVE_LIBPCRE
  #undef HAVE_LIBPCRE2
  #undef ENABLE_NTLM          /* or define with AmiSSL DES/MD4 if available */
  #undef HAVE_METALINK
  #undef HAVE_GPGME
  #undef USE_SIGNAL_TIMEOUT
  #undef HAVE_LIBCARES
  #undef USE_POSIX_THREADS
  #define HAVE_ISATTY 1
  #define HAVE_NANOSLEEP 1
  #define HAVE_SELECT 1
  #define HAVE_GETHOSTBYNAME 1
  #define HAVE_SOCKET 1
  #define HAVE_CONNECT 1
