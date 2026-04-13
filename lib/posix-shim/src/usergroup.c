/*
 * usergroup.c -- Single-user stubs for AmigaOS
 *
 * AmigaOS is a single-user OS. All user/group functions return
 * hardcoded values for the "amiga" user with uid/gid 0.
 *
 * amiport: These stubs unblock: ls, find, cp, id, whoami, logname,
 * tty, file, date, test, which.
 */

#include <amiport/pwd.h>
#include <amiport/grp.h>
#include <amiport/unistd.h>

#include <proto/dos.h>
#include <string.h>

/* Static passwd structure returned by all getpw* functions */
static struct amiport_passwd s_passwd = {
    "amiga",        /* pw_name */
    "*",            /* pw_passwd */
    0,              /* pw_uid */
    0,              /* pw_gid */
    "",             /* pw_gecos */
    "SYS:",         /* pw_dir */
    "C:Shell"       /* pw_shell */
};

/* Static group structure returned by all getgr* functions */
static struct amiport_group s_group = {
    "amiga",        /* gr_name */
    "*",            /* gr_passwd */
    0,              /* gr_gid */
    NULL            /* gr_mem */
};

/* --- UID/GID functions --- */

int amiport_getuid(void)
{
    return 0;
}

int amiport_geteuid(void)
{
    return 0;
}

int amiport_getgid(void)
{
    return 0;
}

int amiport_getegid(void)
{
    return 0;
}

int amiport_setuid(int uid)
{
    (void)uid;
    return 0;
}

int amiport_setgid(int gid)
{
    (void)gid;
    return 0;
}

/* --- Password database --- */

struct amiport_passwd *amiport_getpwuid(int uid)
{
    (void)uid;
    return &s_passwd;
}

struct amiport_passwd *amiport_getpwnam(const char *name)
{
    (void)name;
    return &s_passwd;
}

int amiport_getpwuid_r(int uid,
                       struct amiport_passwd *pwd_out,
                       char *buf,
                       unsigned long buflen,
                       struct amiport_passwd **result)
{
    /* amiport: AmigaOS has a single hardcoded "amiga" user. Copy the
     * static passwd strings into the caller's buffer per POSIX.1-2008
     * getpwuid_r semantics, then point the returned struct's string
     * fields at the buffer. Uid argument is ignored -- we have no
     * user database, only the one stub user.
     *
     * Total string bytes needed: sum of each field (including NUL)
     * from s_passwd above. Approximate upper bound is 64 bytes for
     * the current stub values; we refuse if buflen < 64.
     */
    static const unsigned long REQUIRED_BYTES = 64UL;

    unsigned long cursor;
    unsigned long name_len;
    unsigned long pass_len;
    unsigned long gecos_len;
    unsigned long dir_len;
    unsigned long shell_len;

    (void)uid;

    if (pwd_out == NULL || result == NULL) {
        *result = NULL;
        return 22;  /* EINVAL */
    }

    if (buf == NULL || buflen < REQUIRED_BYTES) {
        *result = NULL;
        return 34;  /* ERANGE */
    }

    name_len  = (unsigned long)strlen(s_passwd.pw_name)   + 1;
    pass_len  = (unsigned long)strlen(s_passwd.pw_passwd) + 1;
    gecos_len = (unsigned long)strlen(s_passwd.pw_gecos)  + 1;
    dir_len   = (unsigned long)strlen(s_passwd.pw_dir)    + 1;
    shell_len = (unsigned long)strlen(s_passwd.pw_shell)  + 1;

    if (name_len + pass_len + gecos_len + dir_len + shell_len > buflen) {
        *result = NULL;
        return 34;  /* ERANGE */
    }

    cursor = 0;

    memcpy(buf + cursor, s_passwd.pw_name, name_len);
    pwd_out->pw_name = buf + cursor;
    cursor += name_len;

    memcpy(buf + cursor, s_passwd.pw_passwd, pass_len);
    pwd_out->pw_passwd = buf + cursor;
    cursor += pass_len;

    memcpy(buf + cursor, s_passwd.pw_gecos, gecos_len);
    pwd_out->pw_gecos = buf + cursor;
    cursor += gecos_len;

    memcpy(buf + cursor, s_passwd.pw_dir, dir_len);
    pwd_out->pw_dir = buf + cursor;
    cursor += dir_len;

    memcpy(buf + cursor, s_passwd.pw_shell, shell_len);
    pwd_out->pw_shell = buf + cursor;
    /* cursor += shell_len; -- unused after last copy */

    pwd_out->pw_uid = s_passwd.pw_uid;
    pwd_out->pw_gid = s_passwd.pw_gid;

    *result = pwd_out;
    return 0;
}

/* --- Group database --- */

struct amiport_group *amiport_getgrgid(int gid)
{
    (void)gid;
    return &s_group;
}

struct amiport_group *amiport_getgrnam(const char *name)
{
    (void)name;
    return &s_group;
}

/* --- Group membership --- */

int amiport_getgroups(int size, int *list)
{
    if (size == 0) {
        return 1; /* one group available */
    }
    if (size < 1 || !list) {
        return -1;
    }
    list[0] = 0; /* gid 0 */
    return 1;
}

int amiport_getgrouplist(const char *user, int basegid, int *groups, int *ngroups)
{
    (void)user;
    if (!groups || !ngroups) {
        return -1;
    }
    if (*ngroups < 1) {
        *ngroups = 1;
        return -1;
    }
    groups[0] = basegid;
    *ngroups = 1;
    return 1;
}

/* --- Login name --- */

char *amiport_getlogin(void)
{
    return "amiga";
}

/* --- TTY name --- */

char *amiport_ttyname(int fd)
{
    (void)fd;
    /* amiport: AmigaOS console device name. IsInteractive()
     * could be checked but all fds point to the same console. */
    return "CONSOLE:";
}

/* --- OpenBSD convenience functions --- */

const char *amiport_user_from_uid(int uid, int noname)
{
    (void)uid;
    (void)noname;
    return "amiga";
}

const char *amiport_group_from_gid(int gid, int noname)
{
    (void)gid;
    (void)noname;
    return "amiga";
}
