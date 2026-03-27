/*
 * pyconfig.h — CPython 3.11 configuration for AmigaOS 3.x
 *
 * Hand-crafted for cross-compilation with bebbo-gcc (GCC 6.5.0b)
 * targeting 68030 + 16MB Fast RAM, -noixemul (libnix/newlib).
 *
 * Based on the WASI config.site pattern — same constraints:
 * no fork, no threads, no mmap, no sockets, no dynamic loading.
 *
 * amiport: CPython 3.11.12 AmigaOS port
 */

#ifndef Py_PYCONFIG_H
#define Py_PYCONFIG_H

/* ═══════════════════════════════════════════════════════════════
 * Platform identification
 * ═══════════════════════════════════════════════════════════════ */
/* amiport: Block the system's incomplete pthread type declarations.
 * sys/_pthreadtypes.h forward-declares pthread_mutex_t etc. as
 * incomplete struct types. CPython's pthread_stubs.h provides concrete
 * definitions that can be used as struct fields (required by pycore_gil.h).
 * We must block the system header before stdlib.h pulls it in. */
#define _SYS__PTHREADTYPES_H_

#define PLATFORM "amigaos3"
#define MULTIARCH "m68k-amigaos"

/* ═══════════════════════════════════════════════════════════════
 * Type sizes — 68k is ILP32, big-endian
 * ═══════════════════════════════════════════════════════════════ */
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_VOID_P 4
#define SIZEOF_SIZE_T 4
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_LONG_DOUBLE 8  /* bebbo-gcc: long double == double on 68k */
#define SIZEOF_FPOS_T 4
#define SIZEOF_OFF_T 4
#define SIZEOF_PID_T 4
#define SIZEOF_TIME_T 4
#define SIZEOF_UINTPTR_T 4
#define SIZEOF_WCHAR_T 4
#define SIZEOF__BOOL 1

/* No 128-bit integers on 68k */
/* #undef HAVE_GCC_UINT128_T */

/* Byte order — 68k is big-endian */
#define WORDS_BIGENDIAN 1
#define DOUBLE_IS_BIG_ENDIAN_IEEE754 1
/* #undef DOUBLE_IS_LITTLE_ENDIAN_IEEE754 */
/* #undef DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754 */

/* PY_FORMAT macros for printf */
#define PY_FORMAT_SIZE_T "z"
#define PY_FORMAT_LONG_LONG "ll"

/* Bignum digit size — 15-bit digits avoid 64-bit multiply */
#define PYLONG_BITS_IN_DIGIT 15

/* ═══════════════════════════════════════════════════════════════
 * Standard headers available in bebbo-gcc libnix/newlib
 * ═══════════════════════════════════════════════════════════════ */
#define HAVE_ALLOCA_H 1
#define HAVE_CTYPE_H 1
#define HAVE_DIRENT_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_FLOAT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_LOCALE_H 1
#define HAVE_MATH_H 1
#define HAVE_SETJMP_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1

/* Headers NOT available */
/* #undef HAVE_SYS_RESOURCE_H */
/* #undef HAVE_SYS_SOCKET_H */
/* #undef HAVE_SYS_UN_H */
/* #undef HAVE_SYS_WAIT_H */
/* #undef HAVE_SYS_MMAN_H */
/* #undef HAVE_SYS_SELECT_H */
/* #undef HAVE_SYS_SENDFILE_H */
/* #undef HAVE_SYS_TIME_H */
/* #undef HAVE_SYS_UTSNAME_H */
/* #undef HAVE_SYS_IOCTL_H */
/* #undef HAVE_SYS_PARAM_H */
/* #undef HAVE_SYS_FILE_H */
/* #undef HAVE_SYSEXITS_H */
/* #undef HAVE_SPAWN_H */
/* #undef HAVE_POLL_H */
/* #undef HAVE_SYS_POLL_H */
/* #undef HAVE_SYS_EPOLL_H */
/* #undef HAVE_SYS_EVENT_H */
/* #undef HAVE_SYS_STATVFS_H */
/* #undef HAVE_SYS_RANDOM_H */
/* #undef HAVE_TERMIOS_H */
/* #undef HAVE_PTY_H */
/* #undef HAVE_LIBUTIL_H */
/* #undef HAVE_UTIL_H */
/* #undef HAVE_GRP_H */
/* #undef HAVE_PWD_H */
/* #undef HAVE_SHADOW_H */
/* #undef HAVE_LANGINFO_H */
/* #undef HAVE_LINUX_CAN_H */
/* #undef HAVE_LINUX_CAN_RAW_H */
/* #undef HAVE_LINUX_RANDOM_H */
/* #undef HAVE_LINUX_TIPC_H */
/* #undef HAVE_LINUX_VM_SOCKETS_H */
/* #undef HAVE_NETPACKET_PACKET_H */
/* #undef HAVE_NET_IF_H */
/* #undef HAVE_NETINET_IN_H */
/* #undef HAVE_ARPA_INET_H */
/* #undef HAVE_NETDB_H */
/* #undef HAVE_DLFCN_H */

/* ═══════════════════════════════════════════════════════════════
 * Types available
 * ═══════════════════════════════════════════════════════════════ */
#define HAVE_SSIZE_T 1
#define HAVE_LONG_LONG 1
#define HAVE_UINTPTR_T 1
#define HAVE_INT32_T 1
#define HAVE_INT64_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT64_T 1
/* #undef HAVE_USABLE_WCHAR_T */

/* ═══════════════════════════════════════════════════════════════
 * Functions available in libnix/newlib (-noixemul)
 * Verified via: m68k-amigaos-nm libc.a | grep ' T _funcname'
 * ═══════════════════════════════════════════════════════════════ */

/* File I/O — libnix has unified POSIX fd table */
#define HAVE_DUP 1
#define HAVE_DUP2 1
#define HAVE_FCNTL 1
#define HAVE_FDATASYNC 1
#define HAVE_FILENO 1
#define HAVE_FLOCK 1
#define HAVE_FTRUNCATE 1
#define HAVE_GETCWD 1
#define HAVE_ISATTY 1
#define HAVE_LSEEK 1
#define HAVE_OPENDIR 1
#define HAVE_READDIR 1
#define HAVE_CLOSEDIR 1
/* #undef HAVE_FDOPENDIR */
/* #undef HAVE_OPENAT */
/* #undef HAVE_FSTATAT */
/* #undef HAVE_RENAMEAT */
/* #undef HAVE_UNLINKAT */
/* #undef HAVE_READLINKAT */
/* #undef HAVE_MKDIRAT */
/* #undef HAVE_SYMLINKAT */
/* #undef HAVE_FCHMODAT */
/* #undef HAVE_FCHOWNAT */
/* #undef HAVE_LINKAT */

/* String functions */
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRTOD 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1

/* Math */
#define HAVE_ACOSH 1
#define HAVE_ASINH 1
#define HAVE_ATANH 1
#define HAVE_COPYSIGN 1
#define HAVE_EXPM1 1
#define HAVE_FINITE 1
#define HAVE_HYPOT 1
#define HAVE_ISINF 1
#define HAVE_ISNAN 1
#define HAVE_LOG1P 1
#define HAVE_LOG2 1
#define HAVE_ROUND 1
#define HAVE_LGAMMA 1
#define HAVE_TGAMMA 1
#define HAVE_ERF 1
#define HAVE_ERFC 1

/* Memory */
#define HAVE_ALLOCA 1
#define HAVE_MALLOC 1
#define HAVE_REALLOC 1
#define HAVE_CALLOC 1

/* Time */
#define HAVE_CLOCK 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_MKTIME 1
#define HAVE_STRFTIME 1
#define HAVE_LOCALTIME 1
#define HAVE_GMTIME 1
/* #undef HAVE_TIMEGM */  /* amiport: let timemodule.c use its own fallback */
#define HAVE_DIFFTIME 1
#define HAVE_CLOCK_GETTIME 1  /* amiport: provided by amigaos_stubs.c */
/* #undef HAVE_CLOCK_GETRES */
/* #undef HAVE_CLOCK_SETTIME */
/* #undef HAVE_CLOCK_NANOSLEEP */

/* Locale */
#define HAVE_SETLOCALE 1
/* #undef HAVE_LANGINFO_H */
/* #undef HAVE_NL_LANGINFO */

/* amiport: debug-agent -- libnix mbstowcs() crashes when dest=NULL (used to
 * query the required buffer size). Define HAVE_BROKEN_MBSTOWCS so CPython
 * uses strlen(arg) as the size estimate instead of mbstowcs(NULL, arg, 0).
 * Since AmigaOS libnix is always C locale (ASCII superset), strlen() is a
 * safe upper bound. Without this, _Py_DecodeLocaleEx() -> decode_current_locale()
 * -> _Py_mbstowcs(NULL, arg, 0) writes to address 0 (NULL) and crashes with
 * Guru Meditation #80000006 (ACPU_AddrErr) during Py_Initialize(). */
#define HAVE_BROKEN_MBSTOWCS 1
#define HAVE_MBSTOWCS 1

/* Misc libc */
#define HAVE_NANOSLEEP 1  /* amiport: provided by amigaos_stubs.c */
#define HAVE_ALARM 1
#define HAVE_GETPID 1
#define HAVE_GETENV 1
#define HAVE_PUTENV 1
#define HAVE_SETENV 1
#define HAVE_UNSETENV 1
#define HAVE_TMPFILE 1
#define HAVE_MKSTEMP 1
#define HAVE_RENAME 1
#define HAVE_REMOVE 1
#define HAVE_RMDIR 1
#define HAVE_MKDIR 1
#define HAVE_CHDIR 1
#define HAVE_CHMOD 1
#define HAVE_UMASK 1
#define HAVE_UNLINK 1
#define HAVE_STAT 1
#define HAVE_FSTAT 1
#define HAVE_LSTAT 1
#define HAVE_UTIMES 1
#define HAVE_GETPAGESIZE 1

/* ═══════════════════════════════════════════════════════════════
 * Functions NOT available — process model
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_FORK */
/* #undef HAVE_FORK1 */
/* #undef HAVE_FORKPTY */
/* #undef HAVE_EXECV */
/* #undef HAVE_EXECVE */
/* #undef HAVE_PIPE */
/* #undef HAVE_PIPE2 */
/* #undef HAVE_POPEN */
/* #undef HAVE_SYSTEM */
/* #undef HAVE_WAIT */
/* #undef HAVE_WAIT3 */
/* #undef HAVE_WAIT4 */
/* #undef HAVE_WAITID */
/* #undef HAVE_WAITPID */
/* #undef HAVE_SETPGID */
/* #undef HAVE_SETPGRP */
/* #undef HAVE_SETSID */
/* #undef HAVE_GETPGRP */
/* #undef HAVE_GETPPID */
/* #undef HAVE_KILL */
/* #undef HAVE_KILLPG */
/* #undef HAVE_POSIX_SPAWN */
/* #undef HAVE_POSIX_SPAWNP */

/* ═══════════════════════════════════════════════════════════════
 * Functions NOT available — signals (beyond basic)
 * ═══════════════════════════════════════════════════════════════ */
#define HAVE_SIGNAL 1
#define HAVE_RAISE 1
/* #undef HAVE_SIGACTION */
/* #undef HAVE_SIGALTSTACK */
/* #undef HAVE_SIGFILLSET */
/* #undef HAVE_SIGINTERRUPT */
/* #undef HAVE_SIGPENDING */
/* #undef HAVE_SIGPROCMASK */
/* #undef HAVE_SIGRELSE */
/* #undef HAVE_SIGTIMEDWAIT */
/* #undef HAVE_SIGWAIT */
/* #undef HAVE_SIGWAITINFO */
/* #undef HAVE_PTHREAD_KILL */
/* #undef HAVE_PTHREAD_SIGMASK */

/* ═══════════════════════════════════════════════════════════════
 * Functions NOT available — networking
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_SOCKET */
/* #undef HAVE_SOCKETPAIR */
/* #undef HAVE_GETADDRINFO */
/* #undef HAVE_GETNAMEINFO */
/* #undef HAVE_GETHOSTBYNAME */
/* #undef HAVE_GETHOSTNAME */
/* #undef HAVE_INET_ATON */
/* #undef HAVE_INET_PTON */
/* #undef HAVE_IF_NAMEINDEX */

/* ═══════════════════════════════════════════════════════════════
 * Functions NOT available — mmap / VM
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_MMAP */
/* #undef HAVE_MREMAP */
/* #undef HAVE_MADVISE */
/* #undef HAVE_MPROTECT */

/* ═══════════════════════════════════════════════════════════════
 * Functions NOT available — dynamic loading
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_DLOPEN */
/* #undef HAVE_DYNAMIC_LOADING */

/* ═══════════════════════════════════════════════════════════════
 * Functions NOT available — users/groups (no multi-user on AmigaOS)
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_GETUID */
/* #undef HAVE_GETEUID */
/* #undef HAVE_GETGID */
/* #undef HAVE_GETEGID */
/* #undef HAVE_SETUID */
/* #undef HAVE_SETEUID */
/* #undef HAVE_SETGID */
/* #undef HAVE_SETEGID */
/* #undef HAVE_GETPWUID */
/* #undef HAVE_GETPWNAM */
/* #undef HAVE_GETGRNAM */
/* #undef HAVE_GETGRGID */
/* #undef HAVE_GETLOGIN */
/* #undef HAVE_GETGROUPS */
/* #undef HAVE_SETGROUPS */
/* #undef HAVE_INITGROUPS */

/* ═══════════════════════════════════════════════════════════════
 * Functions NOT available — misc
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_CHROOT */
/* #undef HAVE_CTERMID */
/* #undef HAVE_LINK */
/* #undef HAVE_SYMLINK */
/* #undef HAVE_READLINK */
/* #undef HAVE_REALPATH */
/* #undef HAVE_SYSCONF */
/* #undef HAVE_CONFSTR */
/* #undef HAVE_PATHCONF */
/* #undef HAVE_FPATHCONF */
/* #undef HAVE_TRUNCATE */
/* #undef HAVE_LOCKF */
/* #undef HAVE_STATVFS */
/* #undef HAVE_FSTATVFS */
/* #undef HAVE_FUTIMES */
/* #undef HAVE_FUTIMESAT */
/* #undef HAVE_LUTIMES */
/* #undef HAVE_UTIMENSAT */
/* #undef HAVE_GETRANDOM */
/* #undef HAVE_GETENTROPY */
/* #undef HAVE_SENDFILE */
/* #undef HAVE_PREAD */
/* #undef HAVE_PREADV */
/* #undef HAVE_PWRITE */
/* #undef HAVE_PWRITEV */
/* #undef HAVE_WRITEV */
/* #undef HAVE_READV */
/* #undef HAVE_POSIX_FALLOCATE */
/* #undef HAVE_POSIX_FADVISE */
/* #undef HAVE_MKOSTEMP */
/* #undef HAVE_MKDTEMP */
/* #undef HAVE_COPY_FILE_RANGE */

/* ═══════════════════════════════════════════════════════════════
 * Threading — use CPython's built-in pthread stubs
 * ═══════════════════════════════════════════════════════════════ */
#define HAVE_PTHREAD_STUBS 1
/* #undef HAVE_PTHREAD_H */
/* #undef WITH_THREAD */
/* #undef HAVE_PTHREAD_INIT */
/* #undef HAVE_PTHREAD_DESTRUCTOR */
/* #undef HAVE_PTHREAD_CONDATTR_SETCLOCK */
/* #undef HAVE_PTHREAD_GETCPUCLOCKID */
/* #undef PTHREAD_SYSTEM_SCHED_SUPPORTED */

/* ═══════════════════════════════════════════════════════════════
 * Python feature configuration
 * ═══════════════════════════════════════════════════════════════ */

/* Version info */
#define PY_MAJOR_VERSION 3
#define PY_MINOR_VERSION 11
#define PY_MICRO_VERSION 12
#define PY_RELEASE_LEVEL PY_RELEASE_LEVEL_FINAL
#define PY_RELEASE_SERIAL 0
#define PY_VERSION "3.11.12"

/* Install paths (Amiga-style) */
#define PREFIX "PYTHON:"
#define EXEC_PREFIX "PYTHON:"
#define PYTHONPATH "PYTHON:lib"
#define VERSION "3.11"
#define VPATH ""

/* Module configuration */
/* #undef Py_ENABLE_SHARED */
#define Py_HASH_ALGORITHM 1  /* FNV */

/* /dev/null → Amiga NIL: */
#define _Py_DEVNULL "NIL:"

/* Path separator */
#define DELIM ':'
#define SEP '/'
/* #undef ALTSEP */

/* No computed gotos on 68k — too many registers consumed */
/* #undef HAVE_COMPUTED_GOTOS */
/* #undef USE_COMPUTED_GOTOS */

/* Signal */
#define RETSIGTYPE void

/* Build features */
/* Py_UNICODE_SIZE and PY_UNICODE_TYPE defined by Include/unicodeobject.h
 * based on SIZEOF_WCHAR_T — do not define here (causes redefinition) */

/* No readline */
/* #undef HAVE_RL_CALLBACK */
/* #undef HAVE_RL_CATCH_SIGNAL */
/* #undef HAVE_RL_COMPLETION_APPEND_CHARACTER */
/* #undef HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK */
/* #undef HAVE_RL_COMPLETION_MATCHES */
/* #undef HAVE_RL_COMPLETION_SUPPRESS_APPEND */
/* #undef HAVE_RL_PRE_INPUT_HOOK */
/* #undef HAVE_RL_RESIZE_TERMINAL */
/* #undef HAVE_READLINE_READLINE_H */

/* No curses */
/* #undef HAVE_CURSES_H */
/* #undef HAVE_NCURSES_H */
/* #undef HAVE_CURSES_IS_PAD */

/* No crypt */
/* #undef HAVE_CRYPT_H */
/* #undef HAVE_CRYPT_R */

/* Struct stat fields */
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1
#define HAVE_STRUCT_STAT_ST_BLOCKS 1
#define HAVE_STRUCT_STAT_ST_RDEV 1
/* #undef HAVE_STRUCT_STAT_ST_FLAGS */
/* #undef HAVE_STRUCT_STAT_ST_GEN */
/* #undef HAVE_STRUCT_STAT_ST_BIRTHTIME */
/* #undef HAVE_STAT_TV_NSEC */
/* #undef HAVE_STAT_TV_NSEC2 */
/* #undef HAVE_DIRENT_D_TYPE */

/* ═══════════════════════════════════════════════════════════════
 * Compiler features (bebbo-gcc 6.5.0b)
 * ═══════════════════════════════════════════════════════════════ */
#define HAVE_BUILTIN_ATOMIC 1
#define HAVE_STD_ATOMIC 1
/* #undef HAVE_C_DYNAMIC_LOADING */
/* #undef HAVE_GCC_ASM_FOR_MC68881 */
#define HAVE_LONG_DOUBLE 1
#define HAVE_PROTOTYPES 1
#define HAVE_STDARG_PROTOTYPES 1
#define HAVE_ATTRIBUTE_FORMAT_PARENS 1

/* GCC builtins */
#define HAVE___BUILTIN_EXPECT 1
#define HAVE___BUILTIN_CLZ 1

/* C11 atomics — via local stdatomic.h wrapping GCC builtins */
#define HAVE_BUILTIN_ATOMIC 1
#define HAVE_STD_ATOMIC 1

/* ═══════════════════════════════════════════════════════════════
 * Alignment
 * ═══════════════════════════════════════════════════════════════ */
#define ALIGNOF_LONG 2        /* 68k word-aligns longs */
#define ALIGNOF_SIZE_T 2
#define ALIGNOF_MAX_ALIGN_T 2
#define HAVE_ALIGNED_REQUIRED 1

/* ═══════════════════════════════════════════════════════════════
 * Endianness
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_HTOBE64 */
/* #undef HAVE_HTOLE64 */

/* ═══════════════════════════════════════════════════════════════
 * Do NOT use system ffi, expat, zlib, etc.
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_LIBFFI */
/* #undef HAVE_FFI_PREP_CLOSURE_LOC */
/* #undef HAVE_FFI_PREP_CLOBBER_NONSTANDARD */
/* #undef HAVE_LIBZ */
/* #undef HAVE_ZLIB_COPY */
/* #undef HAVE_LIBBZ2 */
/* #undef HAVE_LIBLZMA */
/* #undef HAVE_LIBSQLITE3 */
/* #undef HAVE_LIBSSL */
/* #undef HAVE_OPENSSL_CRYPTO_H */
/* #undef HAVE_LIBREADLINE */
/* #undef HAVE_LIBEDIT */

/* ═══════════════════════════════════════════════════════════════
 * Misc defines CPython expects
 * ═══════════════════════════════════════════════════════════════ */
/* #undef HAVE_DYNAMIC_LOADING */
#define Py_NO_ENABLE_SHARED 1
#define ABIFLAGS ""
#define SOABI "cpython-311-m68k-amigaos"
#define _PYTHONFRAMEWORK ""
/* #undef HAVE_LINUX_MEMFD_H */
/* #undef HAVE_MEMFD_CREATE */
/* #undef HAVE_DEV_PTC */
/* #undef HAVE_DEV_PTMX */
/* amiport: debug-agent -- WITH_PYMALLOC disabled to diagnose AN_MemCorrupt.
 * All Python allocations fall through to libnix malloc(). If crash disappears,
 * pymalloc pool/arena management is the culprit. */
#define WITH_PYMALLOC 1
/* #undef WITH_VALGRIND */
/* #undef WITH_DOC_STRINGS */  /* Save binary space */
/* #undef WITH_DTRACE */
/* #undef HAVE_LINUX_AUXVEC_H */
/* #undef HAVE_SYS_AUXV_H */

/* Disable some optional features to reduce binary size */
/* #undef HAVE_LINUX_MEMBARRIER_H */
/* #undef HAVE_EVENTFD */
/* #undef HAVE_TIMERFD_CREATE */
/* #undef HAVE_LINUX_WAIT_H */

/* ═══════════════════════════════════════════════════════════════
 * 68k-specific: no computed gotos, limited FPU
 * ═══════════════════════════════════════════════════════════════ */
/* FPU — software float unless 68881/68882 present */
/* #undef HAVE_GCC_ASM_FOR_X87 */    /* x87 FPU — Intel only, not 68k */
/* #undef HAVE_GCC_ASM_FOR_MC68881 */

/* va_list is a pointer on 68k */
#define VA_LIST_IS_ARRAY 0

#endif /* !Py_PYCONFIG_H */
