/*
 * amiport/utsname.h -- System identification for AmigaOS
 *
 * Provides uname() returning static AmigaOS/m68k info, gethostname()
 * reading ENV:HOSTNAME, sysconf() with hardcoded values, getrusage()
 * returning zeroes, and setproctitle() as a no-op.
 *
 * amiport: gethostname uses GetVar("HOSTNAME") from dos.library.
 * uname values are hardcoded for AmigaOS 3.1 / m68k.
 */

#ifndef AMIPORT_UTSNAME_H
#define AMIPORT_UTSNAME_H

/* --- uname --- */

#define AMIPORT_UTSNAME_LEN 65

struct amiport_utsname {
    char sysname[AMIPORT_UTSNAME_LEN];     /* "AmigaOS" */
    char nodename[AMIPORT_UTSNAME_LEN];    /* hostname or "amiga" */
    char release[AMIPORT_UTSNAME_LEN];     /* "3.1" */
    char version[AMIPORT_UTSNAME_LEN];     /* "40.68" */
    char machine[AMIPORT_UTSNAME_LEN];     /* "m68k" */
};

int amiport_uname(struct amiport_utsname *buf);

/* --- gethostname --- */

int amiport_gethostname(char *name, int namelen);

/* --- sysconf --- */

/* sysconf constants (matching common POSIX values) */
#define AMIPORT_SC_PAGESIZE         30
#define AMIPORT_SC_ARG_MAX           1
#define AMIPORT_SC_OPEN_MAX          4
#define AMIPORT_SC_CLK_TCK           2
#define AMIPORT_SC_NPROCESSORS_ONLN 84
#define AMIPORT_SC_LINE_MAX         43

long amiport_sysconf(int name);

/* --- getrusage --- */

#define AMIPORT_RUSAGE_SELF     0
#define AMIPORT_RUSAGE_CHILDREN -1

struct amiport_rusage {
    long ru_utime_sec;      /* user time seconds */
    long ru_utime_usec;     /* user time microseconds */
    long ru_stime_sec;      /* system time seconds */
    long ru_stime_usec;     /* system time microseconds */
    long ru_maxrss;         /* max resident set size (0) */
};

int amiport_getrusage(int who, struct amiport_rusage *usage);

/* --- setproctitle --- */

void amiport_setproctitle(const char *fmt, ...);

/* --- getprogname / setprogname --- */

/* amiport: __progname is set by amiport_expand_argv() from argv[0].
 * getprogname() just returns it. setprogname() updates it. */
extern char *__progname;

/* Convenience macros */
#ifndef AMIPORT_NO_UTSNAME_MACROS
#define utsname         amiport_utsname
#define uname(buf)      amiport_uname(buf)
#define gethostname(n, l) amiport_gethostname(n, l)

#define _SC_PAGESIZE        AMIPORT_SC_PAGESIZE
#define _SC_ARG_MAX         AMIPORT_SC_ARG_MAX
#define _SC_OPEN_MAX        AMIPORT_SC_OPEN_MAX
#define _SC_CLK_TCK         AMIPORT_SC_CLK_TCK
#define _SC_NPROCESSORS_ONLN AMIPORT_SC_NPROCESSORS_ONLN
#define _SC_LINE_MAX        AMIPORT_SC_LINE_MAX
#define sysconf(n)      amiport_sysconf(n)

#define RUSAGE_SELF     AMIPORT_RUSAGE_SELF
#define RUSAGE_CHILDREN AMIPORT_RUSAGE_CHILDREN
#define rusage          amiport_rusage
#define getrusage(w, u) amiport_getrusage(w, u)

/* amiport: setproctitle is a no-op. Use function directly --
 * variadic macros require C99 so no macro provided. */

#define getprogname()   (__progname)
#define setprogname(s)  (__progname = (char *)(s))
#endif

#endif /* AMIPORT_UTSNAME_H */
