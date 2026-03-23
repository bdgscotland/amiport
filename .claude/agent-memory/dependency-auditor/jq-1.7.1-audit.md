---
name: jq-1.7.1 dependency audit
description: Full dependency classification for jq 1.7.1 (command-line JSON processor) targeting AmigaOS 3.x/m68k
type: project
---

Audited 2026-03-22. Source at /tmp/jq-1.7.1/src/.

## oniguruma

Classification: Optional (can disable with --without-oniguruma)
HAVE_LIBONIG gate: yes, complete. builtin.c wraps the entire regex engine block in #ifdef HAVE_LIBONIG / #else. The #else path returns jv_invalid_with_msg("jq was compiled without ONIGURUMA...") for match(), test(), sub(), gsub(), capture(), scan(). Core JSON processing is unaffected.
The bundled oniguruma submodule IS present in modules/oniguruma/src/ (full C source), so it can be cross-compiled. However it requires its own autoconf configure pass. On first port, disable it.

## pthreads

Classification: Blocker for jv_dtoa_tsd.c as-is, but solvable with small patch.

Three separate uses:
1. jv_alloc.c — has three-path fallback: pthreads -> __thread TLS -> plain static global. The static global path activates when HAVE_PTHREAD_KEY_CREATE, HAVE_PTHREAD_ONCE, and HAVE___THREAD are all absent. This path is safe and correct for single-threaded AmigaOS. No change needed.
2. jv.c — the pthread_key/pthread_mutex/pthread_once block is entirely inside #ifdef WIN32 / #ifndef __MINGW32__. The non-WIN32 path does #include <pthread.h> plus uses pthread_once/pthread_key/pthread_mutex for a TLS implementation and decContext management. This IS compiled on non-Windows. A stub or patch is required.
3. jv_dtoa_tsd.c — unconditionally uses pthread_once, pthread_key_create, pthread_getspecific, pthread_setspecific. No fallback guard. This manages the dtoa (double-to-ascii) context. Called from jv_print.c (tsd_dtoa_context_get) and main.c (jv_tsd_dtoa_ctx_init). MUST be patched to use a single static instance instead.

Fix strategy for pthreads:
- Define stub macros/stubs for pthread_once_t, pthread_key_t, PTHREAD_ONCE_INIT, pthread_once(), pthread_key_create(), pthread_getspecific(), pthread_setspecific(), pthread_mutex_t, PTHREAD_MUTEX_INITIALIZER, pthread_mutex_lock(), pthread_mutex_unlock() as no-ops/trivial implementations in a <pthread.h> shim for AmigaOS.
- Since AmigaOS is single-threaded, pthread_once() can execute the function once unconditionally, pthread_key_t can be an index into a small static array, mutex ops are no-ops.
- jq_test.c (which creates actual pthreads for regression testing) should be excluded from the Amiga build — it is only compiled for `make check`, not for the jq binary itself.

## libm

Classification: Available (libnix)
All math functions in libm.h are guarded by HAVE_xxx ifdefs. The LIBM_DD_NO() macro provides a "not available" stub that returns an error. libnix provides standard C89 math (sin, cos, pow, sqrt, log, exp, floor, ceil, fabs, fmod, atan, atan2, asin, acos). Link with -lm. Advanced GNU math extensions (j0, j1, jn, y0, y1, yn, exp2, exp10, log2, tgamma, lgamma, nearbyint, nextafter, etc.) are NOT in libnix — they will silently become stub errors at runtime, which is acceptable and expected.

## decNumber

Classification: Bundled
Source in src/decNumber/. Compiled as part of libjq when USE_DECNUM=1. Can be disabled with --disable-decnum. No external dependency. The dtoa context management uses pthreads (see above) but the decNumber arithmetic itself does not.

## flex/bison

Classification: Not a runtime dependency
lexer.c and parser.c are pre-generated in the tarball. No flex/bison needed for the build.

## stat()/fstat()

Classification: Needs amiport shim
Used in jv_file.c (fstat + S_ISDIR), linker.c (stat for .jq module file search), util.c. These need amiport_stat()/amiport_fstat(). S_ISDIR is defined in terms of st_mode which amiport_stat() provides.

## realpath()

Classification: Portable concern
Used in util.c's jq_realpath(). Falls back gracefully: if realpath() returns NULL, the original path is used. Since AmigaOS paths are not POSIX-absolute, realpath() will fail and the fallback is used — which is fine for jq's module search.

## isatty()

Classification: Available
libnix has isatty() via unistd.h. The color/tty detection in main.c is guarded by #ifdef USE_ISATTY and is non-critical (affects color output only).

## strptime()/timegm()

Classification: Available (libnix has strptime; timegm has fallback)
strptime is in libnix. timegm has a #ifdef HAVE_TIMEGM guard with mktime() fallback. Both are used by jq's date/time builtins (strptime/1, strftime/1, now/0) which are non-core features.

## gettimeofday()

Classification: Not in libnix, but graceful fallback
The now/0 builtin has #ifdef HAVE_GETTIMEOFDAY; the #else path uses time(NULL). time() is in libnix.

## Summary for porting notes

- Build with: --without-oniguruma --disable-decnum (simplest first-pass)
- pthread shim is the main porting work — write a single-threaded stub pthread.h
- decNumber uses pthreads too (tsd_dec_ctx_get in jv.c) so --disable-decnum also reduces the pthread surface
- stat/fstat need amiport shims
- All math functions degrade gracefully if not present
**Why:** To inform the code-transformer and build-manager agents on the correct porting approach.
**How to apply:** Reference this audit before Stage 1 (analyze-source) and Stage 3 (code-transformer) for the jq port.
