/*
 * config.h -- hand-written PCRE 8.x config for amiport libgit2 build
 *
 * libgit2 bundles PCRE 8.x in deps/pcre/ as its builtin regex backend.
 * PCRE normally generates config.h from config.h.in via CMake or
 * autoconf. amiport provides this file by hand.
 *
 * Minimal PCRE8 build: no JIT, no UTF, no UCP, no PCREGREP. Used only
 * via the pcreposix wrapper by libgit2's util/regexp.c when
 * GIT_REGEX_BUILTIN is set (see git2_features.h).
 */

#ifndef AMIPORT_PCRE_CONFIG_H
#define AMIPORT_PCRE_CONFIG_H

/* Standard headers available in bebbo-gcc libnix */
#define HAVE_DIRENT_H       1
#define HAVE_SYS_STAT_H     1
#define HAVE_SYS_TYPES_H    1
#define HAVE_UNISTD_H       1
#define HAVE_STDINT_H       1
#define HAVE_INTTYPES_H     1

/* Standard C library functions */
#define HAVE_MEMMOVE        1
#define HAVE_STRERROR       1
#define HAVE_STRTOLL        1
#define HAVE_LONG_LONG      1
#define HAVE_UNSIGNED_LONG_LONG 1

/* PCRE build mode */
#define PCRE_STATIC         1
#define SUPPORT_PCRE8       1

/* Disabled PCRE features:
 *   SUPPORT_PCRE16, SUPPORT_PCRE32  -- libgit2 uses 8-bit only
 *   SUPPORT_JIT                      -- no 68k JIT target in PCRE 8.x
 *   SUPPORT_UTF, SUPPORT_UCP         -- ASCII-only build; pcre_ucd.c
 *                                       (209 KB Unicode data) is excluded
 *                                       from the source tree
 *   EBCDIC                           -- not an EBCDIC platform
 *   BSR_ANYCRLF                      -- stick with upstream default
 *   NO_RECURSE                       -- allow stack-recursive pcre_exec
 *                                       (libgit2 regex inputs are short)
 */

/* Line ending -- LF (10) is standard on AmigaOS text files */
#define NEWLINE             10

/* Internal storage tuning */
#define POSIX_MALLOC_THRESHOLD  10
#define LINK_SIZE               2
#define PARENS_NEST_LIMIT       250
#define MATCH_LIMIT             10000000
#define MATCH_LIMIT_RECURSION   MATCH_LIMIT
#define PCREGREP_BUFSIZE        20480  /* dead code: no pcregrep */
#define MAX_NAME_SIZE           32
#define MAX_NAME_COUNT          10000

#endif /* AMIPORT_PCRE_CONFIG_H */
