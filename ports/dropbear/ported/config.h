/* config.h -- Hand-crafted for AmigaOS 3.x (bebbo-gcc)
 * Replaces autotools-generated config.h
 */
#ifndef DROPBEAR_CONFIG_H_
#define DROPBEAR_CONFIG_H_

#define PACKAGE_NAME "Dropbear"
#define PACKAGE_STRING "Dropbear 2025.89"
#define PACKAGE_VERSION "2025.89"
#define PACKAGE_TARNAME "dropbear"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_URL ""

/* Standard C headers available in libnix */
#define STDC_HEADERS 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1

/* Integer types available in bebbo-gcc */
#define HAVE_UINT8_T 1
#define HAVE_UINT16_T 1
#define HAVE_UINT32_T 1
#define HAVE_U_INT8_T 1
#define HAVE_U_INT16_T 1
#define HAVE_U_INT32_T 1

/* Functions available in libnix or amiport shims */
#define HAVE_BASENAME 1
#define HAVE_STRLCPY 1
#define HAVE_STRLCAT 1
#define HAVE_PUTENV 1
#define HAVE_LIBGEN_H 1
#define HAVE_LIBZ 1

/* Network: available via bsdsocket-shim */
#define HAVE_GETADDRINFO 1
#define HAVE_FREEADDRINFO 1
#define HAVE_GAI_STRERROR 1
#define HAVE_GETNAMEINFO 1
#define HAVE_STRUCT_ADDRINFO 1
#define HAVE_NETDB_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETINET_TCP_H 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_SOCKADDR_STORAGE_SS_FAMILY 1

/* select() types for bsdsocket WaitSelect */
#define SELECT_TYPE_ARG1 int
#define SELECT_TYPE_ARG234 fd_set *
#define SELECT_TYPE_ARG5 struct timeval *

/* NOT available on AmigaOS */
/* #undef HAVE_FORK */
/* #undef HAVE_DAEMON */
/* #undef HAVE_CLEARENV */
/* #undef HAVE_GETRANDOM */
/* #undef HAVE_SYS_RANDOM_H */
/* #undef HAVE_WRITEV */
/* #undef HAVE_CLOCK_GETTIME */
/* #undef HAVE_EXPLICIT_BZERO */
/* #undef HAVE_MEMSET_S */
/* #undef HAVE_OPENPTY */
/* #undef HAVE_FEXECVE */
/* #undef HAVE_GETPASS */
/* #undef HAVE_GETGROUPLIST */
/* #undef HAVE_GETSPNAM */
/* #undef HAVE_GETUSERSHELL */
/* #undef HAVE_SYS_UIO_H */
/* #undef HAVE_SYS_WAIT_H */
/* #undef HAVE_SYS_SELECT_H */
/* #undef HAVE_SYS_PRCTL_H */
/* #undef HAVE_SYS_ENDIAN_H */
/* #undef HAVE_ENDIAN_H */
/* #undef HAVE_PATHS_H */
/* #undef HAVE_SHADOW_H */
/* #undef HAVE_CRYPT_H */
/* #undef HAVE_CRYPT */
/* #undef HAVE_LIBUTIL_H */
/* #undef HAVE_UTIL_H */
/* #undef HAVE_PTY_H */
/* #undef HAVE_STROPTS_H */
/* #undef HAVE_UTMP_H */
/* #undef HAVE_UTMPX_H */
/* #undef HAVE_LASTLOG_H */
/* #undef HAVE_LINUX_PKT_SCHED_H */
/* #undef HAVE_MACH_MACH_TIME_H */
/* #undef HAVE_MACH_ABSOLUTE_TIME */
/* #undef HAVE_PAM_PAM_APPL_H */
/* #undef HAVE_SECURITY_PAM_APPL_H */
/* #undef HAVE_LIBPAM */
/* #undef HAVE_STATIC_ASSERT */
/* #undef HAVE_UNDERSCORE_STATIC_ASSERT */
/* NDK provides IPv6 structs (even though we don't use IPv6) */
#define HAVE_STRUCT_IN6_ADDR 1
#define HAVE_STRUCT_SOCKADDR_IN6 1

/* Using our standalone libs, not bundled */
#define BUNDLED_LIBTOM 0

/* No syslog on AmigaOS */
#define DISABLE_SYSLOG 1

/* Disable zlib compression (saves RAM, SSH works fine without it) */
#define DISABLE_ZLIB 1

/* No utmp/wtmp/lastlog on AmigaOS */
#define DISABLE_UTMP 1
#define DISABLE_UTMPX 1
#define DISABLE_WTMP 1
#define DISABLE_WTMPX 1
#define DISABLE_LASTLOG 1
#define DISABLE_PUTUTLINE 1
#define DISABLE_PUTUTXLINE 1

/* No PAM on AmigaOS */
#define DISABLE_PAM 1

/* No fuzzing */
/* #undef DROPBEAR_FUZZ */
/* #undef DROPBEAR_PLUGIN */

/* AmigaOS type definitions — libnix provides pid_t, uid_t, gid_t,
 * mode_t, socklen_t via sys/types.h and sys/socket.h.
 * No typedefs needed here. */

#endif /* DROPBEAR_CONFIG_H_ */
