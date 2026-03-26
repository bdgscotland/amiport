/*
 * amiport/pwd.h -- Password database stubs for AmigaOS
 *
 * AmigaOS is single-user. All functions return a static structure
 * for the "amiga" user with uid 0, gid 0.
 *
 * amiport: No ADCD equivalent -- AmigaOS has no user/group concept.
 * These are pure stubs to satisfy POSIX code that queries user info.
 */

#ifndef AMIPORT_PWD_H
#define AMIPORT_PWD_H

struct amiport_passwd {
    char *pw_name;      /* username */
    char *pw_passwd;    /* password (always "*") */
    int   pw_uid;       /* user ID (always 0) */
    int   pw_gid;       /* group ID (always 0) */
    char *pw_gecos;     /* real name */
    char *pw_dir;       /* home directory (SYS:) */
    char *pw_shell;     /* shell (C:Shell) */
};

struct amiport_passwd *amiport_getpwuid(int uid);
struct amiport_passwd *amiport_getpwnam(const char *name);

/* UID/GID functions */
int amiport_getuid(void);
int amiport_geteuid(void);
int amiport_setuid(int uid);

/* Login name */
char *amiport_getlogin(void);

/* OpenBSD convenience: uid -> name string */
const char *amiport_user_from_uid(int uid, int noname);

/* Convenience macros */
#ifndef AMIPORT_NO_PWD_MACROS
#define passwd       amiport_passwd
#define getpwuid(u)  amiport_getpwuid(u)
#define getpwnam(n)  amiport_getpwnam(n)
#define getuid()     amiport_getuid()
#define geteuid()    amiport_geteuid()
#define setuid(u)    amiport_setuid(u)
#define getlogin()   amiport_getlogin()
#define user_from_uid(u, n) amiport_user_from_uid(u, n)
#endif

#endif /* AMIPORT_PWD_H */
