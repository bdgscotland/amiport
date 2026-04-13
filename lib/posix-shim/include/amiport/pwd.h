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

/*
 * amiport_getpwuid_r -- thread-safe getpwuid variant
 *
 * AmigaOS is single-threaded at the user level, so "thread-safe" is
 * moot, but libgit2 and other portable code call this signature to
 * avoid depending on static buffers. The shim copies the static
 * passwd struct and its strings into the caller's buffer.
 *
 * Parameters match POSIX.1-2008:
 *   uid      -- user id to look up (ignored -- only "amiga"/uid 0 exists)
 *   pwd_out  -- caller's passwd struct to fill
 *   buf      -- caller's buffer for string fields
 *   buflen   -- size of buf in bytes
 *   result   -- *result set to pwd_out on success, NULL on not-found
 *
 * Returns 0 on success, ERANGE if buflen is too small, or an errno
 * value on other failures. On success *result points at pwd_out; on
 * not-found *result is NULL and the function still returns 0 (POSIX).
 *
 * Since AmigaOS has a single hardcoded user, this function always
 * succeeds regardless of uid.
 */
int amiport_getpwuid_r(int uid,
                       struct amiport_passwd *pwd_out,
                       char *buf,
                       unsigned long buflen,
                       struct amiport_passwd **result);

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
#define getpwuid_r(u, p, b, l, r) \
    amiport_getpwuid_r((u), (p), (b), (unsigned long)(l), (r))
#define getuid()     amiport_getuid()
#define geteuid()    amiport_geteuid()
#define setuid(u)    amiport_setuid(u)
#define getlogin()   amiport_getlogin()
#define user_from_uid(u, n) amiport_user_from_uid(u, n)
#endif

#endif /* AMIPORT_PWD_H */
