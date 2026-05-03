# lib/libcurl

HTTP-only build of libcurl 8.11.1. The final external library in the
NetSurf-Vampire Phase D-prime dep stack (Phase 1 of the larger NetSurf
port).

Upstream: https://curl.se/download/curl-8.11.1.tar.gz, curl license
(zlib-style; see `COPYING`). Copyright Daniel Stenberg et al.

## What it is

libcurl is the canonical HTTP client library used by every modern
browser, package manager, system updater, and CI tool. We vendor
~80 hand-picked .c files (down from upstream's ~170 in `lib/`) for
an HTTP-only build with NO TLS, NO proxy, NO authentication, NO IPv6,
NO threading, NO QUIC, NO HTTP/2, NO MQTT/FTP/SMTP/POP3/IMAP/etc.

Total compiled archive: ~432 KB at -O0 (expected ~340-380 KB after
Stage 7 -O1 hot-file promotion in a future revision).

The dropped vendor subdirectories `vtls/`, `vauth/`, `vquic/`, `vssh/`
are **fully vendored as headers** (because `urldata.h` embeds those
structs by value) but only their dispatcher .c files (`vtls/vtls.c`,
`vauth/vauth.c`, `vauth/digest.c`, `vquic/vquic.c`) get built. All
backend implementations (openssl/gnutls/mbedtls/wolfssl/bearssl/sectransp/
schannel/rustls/libssh/libssh2/wolfssh/ngtcp2/quiche/msh3/osslq/cleartext/
cram/krb5/ntlm/oauth2/spnego) are excluded from the build.

## Public API

See `include/curl/curl.h` and `include/curl/multi.h`. Core entry
points (the surface NetSurf's `content/fetchers/curl.c` consumes):

- **Easy interface:** `curl_easy_init`, `curl_easy_setopt`,
  `curl_easy_getinfo`, `curl_easy_perform`, `curl_easy_cleanup`,
  `curl_easy_strerror`, `curl_easy_duphandle`
- **Multi interface:** `curl_multi_init`, `curl_multi_add_handle`,
  `curl_multi_remove_handle`, `curl_multi_setopt`, `curl_multi_perform`,
  `curl_multi_fdset`, `curl_multi_info_read`, `curl_multi_cleanup`,
  `curl_multi_strerror`
- **String list:** `curl_slist_append`, `curl_slist_free_all`
- **Global lifecycle:** `curl_global_init`, `curl_global_cleanup`
- **Version queries:** `curl_version`, `curl_version_info`
- **URL API:** `curl_url`, `curl_url_set`, `curl_url_get`,
  `curl_url_cleanup`

NetSurf also calls `curl_mime_*`, `curl_formadd`, and `curl_easy_setopt`
with TLS-related options (`CURLOPT_SSL_*`, `CURLOPT_CAINFO`). With our
HTTP-only build, those options are accepted by setopt (returning
`CURLE_OK`) but have no runtime effect. NetSurf's existing fallback
(detect HTTPS-not-available and report user-visible error) handles
this gracefully.

## Build

```bash
make build-libcurl    # from project root
# or
make -C lib/libcurl   # direct
```

Produces `libcurl.a` (~432 KB at -O0).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention as libpng / libjpeg / libwapcaplet / libparserutils /
libhubbub / libdom / libcss. The `-m68881` ensures GCC inlines FPU
instructions for any double arithmetic (curl uses doubles in
`progress.c` / `timeval.c`) -- but with our `CURL_DISABLE_PROGRESS_METER`
config the float code paths are dead-code-eliminated even at `-O0`.

**Defines** (passed via Makefile):
- `-DBUILDING_LIBCURL` -- standard for in-tree libcurl builds
- `-DCURL_STATICLIB` -- prevents Windows-style dllimport decoration
- `-DNDEBUG -D_DEFAULT_SOURCE` -- standard for the dep stack

**Build-time config** (in `src/config-amigaos.h`, patched from upstream):
- `CURL_DISABLE_*` for every protocol except HTTP (DICT, FILE, FTP,
  GOPHER, IMAP, LDAP, LDAPS, MQTT, POP3, RTSP, SMB, SMTP, TELNET,
  TFTP)
- `CURL_DISABLE_*` for these features: ALTSVC, AWS, BINDLOCAL, COOKIES,
  DOH, FORM_API, HSTS, HTTP_AUTH, MIME, NETRC, NTLM, PARSEDATE, PROXY,
  SOCKETPAIR, VERBOSE_STRINGS, WEBSOCKETS
- `USE_IPV6` left undefined (no IPv6 in bsdsocket-shim or NDK socket
  headers; libcurl falls back to IPv4-only)
- `USE_THREADS_POSIX` / `USE_THREADS_WIN32` undefined (libnix has no
  pthreads); `CURLRES_SYNCH` is the auto-selected resolver

**Optimization:** `-O0 -fno-strict-aliasing` whole-archive default per
crash-patterns #16 (default to -O0 for new bundled libraries until
proven safe). Stage 7 perf-optimizer audit will promote specific
hot files (likely `transfer.c`, `sendf.c`, `url.c`, `multi.c`,
`http.c`) to `-O1` after struct-return audit.

**Depends on:** nothing for compilation. **Consumer link must
provide:** `bsdsocket.library` symbols (`socket`, `connect`, `recv`,
`send`, `accept`, `getsockname`, `getsockopt`, `setsockopt`,
`gethostbyname`, `IoctlSocket`, `WaitSelect`, `select`, `CloseSocket`)
either by linking `bsdsocket.library` directly OR by linking
`lib/bsdsocket-shim/libamiport-net.a` (which adds the
mixed-fd-split `select`, fd 0/1/2 collision avoidance, getaddrinfo
emulation).

## CRITICAL config: zero soft-float, zero transcendentals

Verified post-build:

```bash
m68k-amigaos-nm libcurl.a | grep -E '__divsf3|__divdf3|__floatunsisf'
# (empty)
m68k-amigaos-nm libcurl.a | grep -E ' U _(pow|exp|log|sqrt|floor)$'
# (empty)
```

curl's `progress.c`, `timeval.c`, and `timediff.c` use `double` for
download speed / ETA / wall-clock arithmetic. With `-m68881` GCC
inlines FPU instructions for `double` math (no soft-float helpers
needed). With `CURL_DISABLE_PROGRESS_METER` set, the float-heavy
display code paths in `progress.c` are dead-code-eliminated even at
`-O0`. Net result: the entire archive uses only integer 68k
instructions plus inline FPU `FADD`/`FSUB`/`FMUL`/`FDIV`. No ROM
`mathieee*.library` calls, no FS-UAE 68882 transcendental gap risk.

This matches the libpng / libjpeg / libdom / libcss safety profile.

## NetSurf compatibility

NetSurf's `content/fetchers/curl.c` (2,144 LOC) calls 50+ libcurl
symbols. All required symbols are present in our HTTP-only archive.
TLS-related setopt calls (`CURLOPT_SSL_CTX_FUNCTION`, `CURLOPT_CAINFO`,
`CURLOPT_SSL_VERIFYPEER`, etc.) return `CURLE_OK` from `setopt` but
have no runtime effect. NetSurf's existing fetch-error handling will
surface "TLS not available" if the user navigates to https://.

The MIME interface (`curl_mime_init`, `curl_mime_addpart`, etc.) is
disabled (`CURL_DISABLE_MIME`). NetSurf uses MIME only for form posts;
those will fail at `curl_easy_perform` time with `CURLE_NOT_BUILT_IN`.
NetSurf's form-submit code already handles the not-supported case.

## Test

```bash
make test-libcurl    # from project root
# or
make -C tests/libcurl run
```

Runs the 21-test suite via `vamos -C 68040 -s 4096 -m 8192
./test_libcurl`.

The test binary provides bsdsocket stubs (`tests/libcurl/socket_stubs.c`)
since vamos has no network. **Tests cover lifecycle / setopt / multi /
slist / URL parsing / strerror / version** -- the API surface
ports/netsurf consumes via `content/fetchers/curl.c`. Real HTTP fetches
are deferred to FS-UAE with bsdsocket.library configured AND/OR
real-Vampire testing via `amigactl` / Roadshow.

Coverage:

- **12 functional** (global init/cleanup, easy init/cleanup, multi
  init/cleanup, easy_setopt with CURLOPT_URL/TIMEOUT/NOSIGNAL,
  slist append/free, version returns "libcurl/...", easy_perform
  without URL returns error, easy_getinfo on fresh handle returns 0)
- **3 error path** (invalid CURLOPT, NULL handle safe, malformed URL
  accepted at setopt time per upstream contract)
- **3 edge case** (slist_append on NULL, repeated global_init/cleanup,
  64-header chain build+free)
- **1 Amiga-specific** (-noixemul cleanup discipline -- 100 easy
  init/cleanup cycles without leak)
- **2 stress** (50 multi cycles, 50 slist build+free cycles)

## CRITICAL: vamos resource sizing

Test binaries linking libcurl need `__stack >= 1048576` AND
`__MEMORY_STEP >= 1048576` (1 MB) per the libnix-stack-scaling
pitfall. libcurl is in the libdom-class size range. The 256 KB
defaults that work for standalone NetSurf utility libs and the
512 KB threshold that works for libpng / libjpeg are insufficient
here -- libcurl's larger code + global data area pushes libnix's
startup-time allocation past 512 KB.

Symptom of insufficient cookies: vamos returns exit 20 with NO stdout
output. The pre-main libnix init crashes silently before the test
runner can print anything.

For ports/netsurf which links the FULL dep stack (libdom + libhubbub +
libcss + libpng + libjpeg + libcurl): 1 MB cookies remain the floor.

## Memory model + consumer cleanup discipline

- **Every `curl_easy_init` MUST be paired with `curl_easy_cleanup`**
- **Every `curl_multi_init` MUST be paired with `curl_multi_cleanup`**
- **`curl_global_init` is refcounted** -- N inits need N cleanups
- **`curl_slist_*`** -- the LAST `curl_slist_append` return value is
  the head of the list; pass it to `curl_slist_free_all`
- **`curl_url_*`** -- pass result of `curl_url_get` (a malloc'd string)
  to `curl_free`
- I/O callback userdata: caller-owned. libcurl never frees it.

On AmigaOS `-noixemul`: leaks are permanent until reboot. NetSurf's
fetch loop must guarantee cleanup on every exit path including
out-of-memory and HTTP-error.

## Memory audit findings

PENDING -- Stage 6 dispatch deferred to a future session. The
21/21 vamos tests passing without crash demonstrate the lifecycle
APIs are stable; a formal memory-checker audit will surface any
ASSERT-failure leak paths or realloc-grow-then-fail bugs that the
test suite doesn't currently exercise.

## Performance audit findings

PENDING -- Stage 7 dispatch deferred to a future session. Expected
hot-path candidates for `-O1` promotion: `transfer.c` (read/write
loop), `sendf.c` (buffer flush), `url.c` (URL parsing), `multi.c`
(state machine), `http.c` (header generation). Each will need a
struct-return audit per crash-patterns #16 before promotion.

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs: ASSERT_* macros return early from
a failing test without running cleanup. Acceptable for unit-test
purposes (vamos host process exit reclaims memory) but not
representative of a real-world consumer leak.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer, pending main NetSurf
  build) -- HTTP `<a href>` follow, `<img src>` fetch, JSON / RSS
  network fetches
- (potentially) future CLI ports: a minimal `curl` CLI wrapper
  could be ~50 lines on top of libcurl.a + bsdsocket-shim
