/*
 * git2_features.h -- hand-written feature flags for amiport libgit2 build
 *
 * Upstream libgit2 generates this file from git2_features.h.in via CMake.
 * amiport does not use CMake, so this file is maintained by hand. It is
 * based on git2_features.h.in at v1.8.5 -- when upgrading libgit2, diff
 * the .in template for new #cmakedefine lines and mirror them here.
 *
 * Build configuration (from PDR-010):
 *   - Single-threaded (GIT_THREADS off)
 *   - No network transports (SSH, HTTPS, WinHTTP, NTLM, GSSAPI off)
 *   - No iconv, no nanosecond stat
 *   - Bundled PCRE 8.x as regex backend
 *   - Bundled sha1dc + rfc6234 as hash backends
 *   - 32-bit architecture (68k is always 32-bit)
 *   - mmap emulated via malloc+pread (NO_MMAP activates posix.c fallback)
 *
 * See PDR-010 for the full rationale. See .claude/rules/library-pipeline.md
 * for the mandatory pipeline stages that govern changes to this file.
 */

#ifndef INCLUDE_features_h__
#define INCLUDE_features_h__

/* ----- Debug options (all off in production) ----- */

/* #undef GIT_DEBUG_POOL */
/* #undef GIT_DEBUG_STRICT_ALLOC */
/* #undef GIT_DEBUG_STRICT_OPEN */

/* ----- Threading (single-threaded AmigaOS 3.x) ----- */

/* #undef GIT_THREADS */

/* ----- Platform debug (Win32 only) ----- */

/* #undef GIT_WIN32_LEAKCHECK */

/* ----- Architecture ----- */

/* 68k is always 32-bit. */
#define GIT_ARCH_32 1
/* #undef GIT_ARCH_64 */

/* ----- stat precision and futimens availability ----- */

/* AmigaOS filesystem timestamps are 2-second granularity on FFS, 1-second
 * on SFS. No nanosecond support anywhere. No futimens. */
/* #undef GIT_USE_NSEC */
/* #undef GIT_USE_STAT_MTIM */
/* #undef GIT_USE_STAT_MTIMESPEC */
/* #undef GIT_USE_STAT_MTIME_NSEC */
/* #undef GIT_USE_FUTIMENS */

/* ----- Character set conversion ----- */

/* #undef GIT_USE_ICONV */

/* ----- Regex backend: bundled PCRE 8.x ----- */

/* #undef GIT_REGEX_REGCOMP_L */
/* #undef GIT_REGEX_REGCOMP */
/* #undef GIT_REGEX_PCRE */
/* #undef GIT_REGEX_PCRE2 */
#define GIT_REGEX_BUILTIN 1

/* ----- qsort_r availability: fall back to libgit2 built-in ----- */

/* #undef GIT_QSORT_BSD */
/* #undef GIT_QSORT_GNU */
/* #undef GIT_QSORT_C11 */
/* #undef GIT_QSORT_MSC */

/* ----- SSH (disabled) ----- */

/* #undef GIT_SSH */
/* #undef GIT_SSH_EXEC */
/* #undef GIT_SSH_LIBSSH2 */
/* #undef GIT_SSH_LIBSSH2_MEMORY_CREDENTIALS */

/* ----- NTLM / Kerberos (disabled) ----- */

/* #undef GIT_NTLM */
/* #undef GIT_GSSAPI */
/* #undef GIT_GSSFRAMEWORK */

/* ----- HTTP/HTTPS transports (all disabled) ----- */

/* #undef GIT_WINHTTP */
/* #undef GIT_HTTPS */
/* #undef GIT_OPENSSL */
/* #undef GIT_OPENSSL_DYNAMIC */
/* #undef GIT_SECURE_TRANSPORT */
/* #undef GIT_MBEDTLS */
/* #undef GIT_SCHANNEL */

/* ----- HTTP parser backend (no transports, but some headers still include) ----- */

/* #undef GIT_HTTPPARSER_HTTPPARSER */
/* #undef GIT_HTTPPARSER_LLHTTP */
/* #undef GIT_HTTPPARSER_BUILTIN */

/* ----- SHA-1 backend: bundled collision-detecting SHA-1 ----- */

#define GIT_SHA1_COLLISIONDETECT 1
/* #undef GIT_SHA1_WIN32 */
/* #undef GIT_SHA1_COMMON_CRYPTO */
/* #undef GIT_SHA1_OPENSSL */
/* #undef GIT_SHA1_OPENSSL_DYNAMIC */
/* #undef GIT_SHA1_MBEDTLS */

/* ----- SHA-256 backend: bundled rfc6234 ----- */

#define GIT_SHA256_BUILTIN 1
/* #undef GIT_SHA256_WIN32 */
/* #undef GIT_SHA256_COMMON_CRYPTO */
/* #undef GIT_SHA256_OPENSSL */
/* #undef GIT_SHA256_OPENSSL_DYNAMIC */
/* #undef GIT_SHA256_MBEDTLS */

/* ----- Random seed source (AmigaOS path in rand.c) ----- */

/* amiport: AmigaOS has no getentropy and no /dev/urandom. rand.c has
 * an explicit #ifdef __AMIGA__ branch that supplements time+pid entropy
 * with stack-address variation. Do NOT enable GIT_RAND_GETENTROPY. */
/* #undef GIT_RAND_GETENTROPY */
/* #undef GIT_RAND_GETLOADAVG */

/* ----- I/O multiplexing (dead code with no transports, but compiles) ----- */

/* #undef GIT_IO_POLL */
/* #undef GIT_IO_WSAPOLL */
/* #undef GIT_IO_SELECT */

#endif /* INCLUDE_features_h__ */
