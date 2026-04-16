/* amigaos_stubs.h -- Stub functions for POSIX calls missing on AmigaOS
 *
 * Included from includes.h when __AMIGA__ is defined.
 * These are inline stubs for functions that libnix/NDK don't provide
 * but Dropbear references. Functions that need real implementations
 * (entropy, select, terminal) go in separate .c files.
 */
#ifndef DROPBEAR_AMIGAOS_STUBS_H
#define DROPBEAR_AMIGAOS_STUBS_H

/* --- User/group (single-user OS) --- */
struct passwd {
    char *pw_name;
    char *pw_passwd;
    int pw_uid;
    int pw_gid;
    char *pw_dir;
    char *pw_shell;
};

struct group {
    char *gr_name;
    int gr_gid;
    char **gr_mem;
};

/* amiport: return a static passwd struct so callers don't warn.
 * AmigaOS is single-user; home dir is S: (system directory). */
static struct passwd _amiga_pw = {
    "amiga", "*", 0, 0, "S:", "/bin/sh"
};
static inline struct passwd *getpwuid(int uid) { (void)uid; return &_amiga_pw; }
static inline struct passwd *getpwnam(const char *n) { (void)n; return &_amiga_pw; }

/* libnix unistd.h declares getuid, geteuid, getgid, getegid,
 * getgroups, fork, pipe, daemon -- don't redefine.
 * They'll return -1/0 at link time from libnix stubs. */

/* --- syslog stubs --- */
static inline void syslog(int priority, const char *fmt, ...) {
    (void)priority; (void)fmt;
}
static inline void openlog(const char *ident, int opt, int fac) {
    (void)ident; (void)opt; (void)fac;
}
static inline void closelog(void) {}

/* fork/pipe/daemon declared in libnix unistd.h, stub at link time */

/* --- sys/resource.h stub (no core dumps on AmigaOS) --- */
#ifndef RLIMIT_CORE
#define RLIMIT_CORE 4
#endif
struct rlimit { long rlim_cur; long rlim_max; };
static inline int getrlimit(int r, struct rlimit *l) {
    (void)r; if (l) { l->rlim_cur = 0; l->rlim_max = 0; } return 0;
}
static inline int setrlimit(int r, const struct rlimit *l) {
    (void)r; (void)l; return 0;
}

/* --- sys/wait.h stub --- */
#ifndef WEXITSTATUS
#define WEXITSTATUS(x) ((x) & 0xff)
#define WIFEXITED(x) 1
#define WIFSIGNALED(x) 0
#define WTERMSIG(x) 0
#endif
static inline int waitpid(int p, int *s, int o) {
    (void)p; (void)s; (void)o; return -1;
}

/* sockaddr_storage: NDK might not have it, provide if missing */
#ifndef _SOCKADDR_STORAGE_DEFINED
#define _SOCKADDR_STORAGE_DEFINED
struct sockaddr_storage {
    unsigned char ss_family;
    char _ss_pad[127];
};
#endif

/* --- Terminal constants for SSH termcodes mapping --- */
/* amiport/termios.h provides tcgetattr/tcsetattr but not all
 * POSIX terminal mode constants. SSH negotiates these with the
 * remote side -- we define the constants so termcodes.c compiles,
 * but the local terminal uses console.device directly. */
#ifndef VINTR
#define VINTR    0
#define VQUIT    1
#define VERASE   2
#define VKILL    3
#define VEOF     4
#define VEOL     5
#define VEOL2    6
#define VSTART   7
#define VSTOP    8
#define VSUSP    9
#define VREPRINT 10
#define VWERASE  11
#define VLNEXT   12
#define VDISCARD 13
#define VMIN     14
#define VTIME    15
#define NCCS     16
#endif

/* Input mode flags */
#ifndef IGNPAR
#define IGNPAR   0x0004
#define INPCK    0x0010
#define ISTRIP   0x0020
#define INLCR    0x0040
#define IGNCR    0x0080
#define ICRNL    0x0100
#define IXON     0x0200
#define IXOFF    0x0400
#define IXANY    0x0800
#define IUCLC    0x1000
#define IMAXBEL  0x2000
#define PARMRK   0x0008
#endif

/* Output mode flags */
#ifndef ONLCR
#define ONLCR    0x0004
#define OCRNL    0x0008
#define ONOCR    0x0010
#define ONLRET   0x0020
#define OPOST    0x0001
#endif

/* Local mode flags */
#ifndef ECHOE
#define ECHO     0x0008
#define ECHOE    0x0002
#define ECHOK    0x0004
#define ECHONL   0x0010
#define NOFLSH   0x0080
#define TOSTOP   0x0100
#define IEXTEN   0x0200
#define ISIG     0x0001
#define ICANON   0x0002
#define ECHOCTL  0x0040
#define ECHOKE   0x0800
#endif

/* Character size bits */
#ifndef CS7
#define CS7      0x0020
#define CS8      0x0030
#define CSIZE    0x0030
#define PARENB   0x0100
#define PARODD   0x0200
#endif

/* Baud rates */
#ifndef B0
#define B0       0
#define B9600    9600
#define B19200   19200
#define B38400   38400
#endif

/* termios struct for SSH terminal negotiation */
#ifndef _TERMIOS_H
typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int speed_t;
struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t     c_cc[NCCS];
    speed_t  c_ispeed;
    speed_t  c_ospeed;
};
int tcgetattr(int fd, struct termios *t);
int tcsetattr(int fd, int act, const struct termios *t);
#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2
speed_t cfgetispeed(const struct termios *t);
speed_t cfgetospeed(const struct termios *t);
#endif

/* --- IPPROTO_TCP for setsockopt TCP_NODELAY --- */
#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif
#ifndef TCP_NODELAY
#define TCP_NODELAY 1
#endif
#ifndef IPPROTO_IP
#define IPPROTO_IP 0
#endif
#ifndef IP_TOS
#define IP_TOS 3
#endif
#ifndef SOL_SOCKET
#define SOL_SOCKET 0xffff
#endif
#ifndef SO_REUSEADDR
#define SO_REUSEADDR 0x0004
#endif
#ifndef SO_ERROR
#define SO_ERROR 0x1007
#endif
#ifndef AF_UNSPEC
#define AF_UNSPEC 0
#endif
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif
#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif
#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 2
#endif
#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 1
#endif
#ifndef AI_PASSIVE
#define AI_PASSIVE 1
#endif
#ifndef SOCK_STREAM
#define SOCK_STREAM 1
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* AF_UNIX stub for stream local (disabled via PROXYCMD=0) */
#ifndef AF_UNIX
#define AF_UNIX 1
#endif

/* sockaddr_un stub */
struct sockaddr_un {
    short sun_family;
    char sun_path[108];
};

#endif /* DROPBEAR_AMIGAOS_STUBS_H */
