/* config.h — AmigaOS 3.x configuration for GNU wget 1.20.3
 * amiport: Hand-crafted for bebbo-gcc (GCC 6.5.0b) + libnix (-noixemul)
 *
 * Phase 1: HTTP-only (no SSL)
 * Phase 2: HTTPS via AmiSSL 5.26 (define HAVE_LIBSSL)
 */

#ifndef CONFIG_H
#define CONFIG_H

/* Package info */
#define PACKAGE "wget"
#define PACKAGE_NAME "GNU Wget"
#define PACKAGE_VERSION "1.20.3"
#define PACKAGE_STRING "GNU Wget 1.20.3"
#define PACKAGE_BUGREPORT "bug-wget@gnu.org"
#define PACKAGE_URL "https://www.gnu.org/software/wget/"
#define VERSION "1.20.3"

/* === Platform detection === */
/* __AMIGA__ is defined by bebbo-gcc */

/* === Type sizes (68020, ILP32) === */
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_SHORT 2
#define SIZEOF_OFF_T 4
#define SIZEOF_VOID_P 4

/* wgint = long long (64-bit file sizes) */
/* Computed in wget.h from SIZEOF_LONG_LONG */

/* === C library features available in libnix === */
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRTOLL 1
#define HAVE_FSEEKO 1
#define HAVE_FTELLO 1
#define HAVE_ISATTY 1
#define HAVE_SLEEP 1
#define HAVE_USLEEP 1
#define HAVE_UTIME 1
#define HAVE_DRAND48 1
/* #undef HAVE_RANDOM */       /* libnix missing — use rand() */
#define HAVE_VASPRINTF 1       /* gnulib provides */
#define HAVE_ALLOCA_H 1        /* gnulib provides */
#define HAVE_ALLOCA 1
/* #undef HAVE_MMAP */         /* No mmap on AmigaOS */
/* #undef HAVE_PATHCONF */     /* No pathconf on AmigaOS */
/* #undef HAVE_STRLCPY */     /* Not in libnix */
#define HAVE_INTPTR_T 1
/* #undef HAVE_MBTOWC */      /* Avoid multibyte paths on AmigaOS */
/* #undef HAVE_WCWIDTH */      /* No wchar support */
/* #undef HAVE_PWD_H */        /* Stubbed via amiport shim */
/* #undef HAVE_NANOSLEEP */    /* Use usleep instead */
#define HAVE_DECL_H_ERRNO 1

/* Struct sockaddr_in available via bsdsocket-shim */
#define HAVE_SOCKADDR_IN6 0
/* #undef HAVE_STRUCT_SOCKADDR_STORAGE */ /* No IPv6 */

/* Signal support */
/* #undef HAVE_SIGSETJMP */    /* No siglongjmp on AmigaOS */
/* #undef HAVE_SIGBLOCK */     /* No sigblock on AmigaOS */
/* #undef USE_SIGNAL_TIMEOUT */ /* No SIGALRM — use no-timeout stub */

/* === Features ENABLED === */
#define ENABLE_DEBUG 1
#define ENABLE_DIGEST 1         /* HTTP digest auth (pure C, no deps) */
#define ENABLE_OPIE 1           /* FTP OPIE auth (pure C) */
/* #undef ENABLE_NTLM */       /* Disable NTLM (Windows proxy auth) */
/* #undef ENABLE_PARTIAL_WRITE */

/* === Features DISABLED === */
/* #undef ENABLE_NLS */         /* No gettext — English only */
/* #undef ENABLE_IRI */         /* No IDN — ASCII URLs only */
/* #undef ENABLE_IPV6 */        /* Use gethostbyname, not getaddrinfo */
/* #undef ENABLE_XATTR */       /* No extended attributes */
/* #undef USE_XATTR */
/* #undef USE_NLS_PROGRESS_BAR */ /* ASCII progress bar only */

/* === External libraries DISABLED === */
/* #undef HAVE_LIBGNUTLS */     /* Using OpenSSL path, not GnuTLS */
/* #undef HAVE_LIBZ */          /* No zlib — no gzip transfer encoding */
/* #undef HAVE_LIBPSL */        /* No public suffix list */
/* #undef HAVE_PSL_LATEST */
/* #undef HAVE_LIBPCRE */       /* Use POSIX regex */
/* #undef HAVE_LIBPCRE2 */
/* #undef HAVE_LIBCARES */      /* No async DNS */
/* #undef HAVE_LIBUUID */       /* No UUID (WARC) */
/* #undef HAVE_UUID_CREATE */
/* #undef HAVE_METALINK */      /* No metalink */
/* #undef HAVE_GPGME */         /* No GPG verification */
/* #undef HAVE_ICONV */         /* No iconv */
/* #undef HAVE_NETTLE */        /* No nettle crypto */
/* #undef HAVE_RAND_EGD */      /* No EGD entropy */

/* === SSL/TLS — Phase 1: DISABLED, Phase 2: AmiSSL === */
/* Uncomment for Phase 2 (HTTPS via AmiSSL 5.26):
#define HAVE_LIBSSL 1
*/
/* #undef HAVE_LIBSSL */        /* Phase 1: HTTP only */

/* === Gnulib compatibility === */
#define _GL_INLINE static inline
#define _GL_INLINE_HEADER_BEGIN
#define _GL_INLINE_HEADER_END
#define _GL_EXTERN_INLINE static inline
#define _GL_ATTRIBUTE_PURE
#define _GL_ATTRIBUTE_CONST
#define _GL_ATTRIBUTE_MALLOC
#define _GL_UNUSED
/* amiport: Do NOT define _GL_ATTRIBUTE_FORMAT here -- error.h provides it
 * with proper GCC __attribute__((format)) support. Defining it here with
 * 3 args would conflict with error.h's 1-arg version. */
#define _Noreturn __attribute__((noreturn))
#define _GL_ARG_NONNULL(x)
/* _GL_ATTRIBUTE_FORMAT_PRINTF needs 2 args (string-idx, first-to-check) */
#define _GL_ATTRIBUTE_FORMAT_PRINTF(a, b)
/* GNULIB_PRINTF_ATTRIBUTE_FLAVOR_GNU -- 0 = use __printf__ not __gnu_printf__ */
#define GNULIB_PRINTF_ATTRIBUTE_FLAVOR_GNU 0

/* Gnulib function availability */
#define HAVE_DECL_GETENV 1
#define HAVE_DECL_STRERROR 1

/* Regex types for POSIX regex translation table */
#define __RE_TRANSLATE_TYPE unsigned char *
#define RE_TRANSLATE_TYPE unsigned char *

/* Block the NDK's sys-include/regex.h -- wget uses gnulib regex from ported/lib/
 * NDK regex.h guard is _REGEX_H_ (note trailing underscore).
 * Define it here so the NDK version is skipped when included via angle brackets. */
#define _REGEX_H_

/* strerror_r is available via POSIX (libnix newlib provides it) */
#define HAVE_DECL_STRERROR_R 1
/* libnix strerror_r is POSIX version (int return, not GNU char* version) */
#define GNULIB_STRERROR_R_POSIX 1
/* strerror_r signature: int strerror_r(int, char *, size_t) */
#define STRERROR_R_CHAR_P 0

/* === Timer === */
/* Force gettimeofday path in ptimer.c (no clock_gettime, no timer_create) */
#define _POSIX_TIMERS (-1)
#define PTIMER_GETTIMEOFDAY 1

/* === File system === */
/* #undef HAVE_SYMLINK */       /* FFS has soft links via MakeLink but rarely used */
#define HAVE_FLOCK 0            /* Stub flock() */

/* Temp directory */
/* amiport redirects tmpdir to T: */

/* === OS identification === */
#define OS_TYPE "AmigaOS"

/* Avoid including spawn.h */
/* #undef HAVE_SPAWN_H */

/* === Suppress unused gnulib features === */
/* #undef HAVE_SYS_UTIME_H */
/* #undef HAVE_PROCESS_H */
/* #undef USE_WATT */           /* DOS networking */

/* O_BINARY does not exist on AmigaOS (Windows/DJGPP-ism) -- treat as no-op */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* stdint.h is available in the toolchain sys-include */
#define HAVE_STDINT_H 1

/* SIZE_MAX for 32-bit ILP32 platform */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)(-1))
#endif

/* Enable sha1_stream / sha256_stream / md5_stream in gnulib crypto files.
 * Without this, only the block-based SHA/MD5 functions are compiled,
 * but warc.c calls sha1_stream() directly. */
#define GL_COMPILE_CRYPTO_STREAM 1

#endif /* CONFIG_H */
