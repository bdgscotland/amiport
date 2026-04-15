/* amigaos_compat.h -- AmigaOS compatibility layer for Dropbear SSH
 *
 * Force-included via -include amigaos_compat.h to provide missing
 * POSIX functionality. Does NOT redefine types/structs that libnix
 * already provides.
 */
#ifndef DROPBEAR_AMIGAOS_COMPAT_H
#define DROPBEAR_AMIGAOS_COMPAT_H

#define __AMIGA__ 1

/* --- POSIX signal constants (libnix signal.h has some but not all) --- */
#ifndef SIGWINCH
#define SIGWINCH 28
#endif
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIGCHLD
#define SIGCHLD 17
#endif
#ifndef SIGHUP
#define SIGHUP 1
#endif

/* --- syslog constants (no syslog.h on AmigaOS) --- */
#define _SYSLOG_H_ 1
#ifndef LOG_WARNING
#define LOG_WARNING 4
#endif
#ifndef LOG_ERR
#define LOG_ERR 3
#endif
#ifndef LOG_INFO
#define LOG_INFO 6
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG 7
#endif
#ifndef LOG_AUTH
#define LOG_AUTH 32
#endif
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV LOG_AUTH
#endif
#ifndef LOG_DAEMON
#define LOG_DAEMON 24
#endif
#ifndef LOG_PID
#define LOG_PID 0x01
#endif
#ifndef LOG_NDELAY
#define LOG_NDELAY 0x08
#endif

/* /dev/urandom path -- will be intercepted by our seedrandom() rewrite */
#define DROPBEAR_URANDOM_DEV "T:dropbear-entropy"

/* amiport: override libnix getpass() which opens /dev/tty and fails.
 * Our amiport_getpass() reads from stdin directly. */
char *amiport_getpass(const char *prompt);
#define getpass(p) amiport_getpass(p)

/* amiport: console I/O bypassing libnix fd table.
 * bsdsocket fd 0 hijacks libc read(0,...) to the socket on FS-UAE.
 * These use AmigaDOS Read(Input())/Write(Output()) directly. */
int amiport_console_read(void *buf, int len);
int amiport_console_write(const void *buf, int len);

#endif /* DROPBEAR_AMIGAOS_COMPAT_H */
