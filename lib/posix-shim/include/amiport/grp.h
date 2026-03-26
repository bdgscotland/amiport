/*
 * amiport/grp.h -- Group database stubs for AmigaOS
 *
 * AmigaOS is single-user. All functions return a static structure
 * for the "amiga" group with gid 0.
 *
 * amiport: No ADCD equivalent -- AmigaOS has no user/group concept.
 */

#ifndef AMIPORT_GRP_H
#define AMIPORT_GRP_H

struct amiport_group {
    char  *gr_name;     /* group name */
    char  *gr_passwd;   /* group password (always "*") */
    int    gr_gid;      /* group ID (always 0) */
    char **gr_mem;      /* member list (always NULL) */
};

struct amiport_group *amiport_getgrgid(int gid);
struct amiport_group *amiport_getgrnam(const char *name);

/* GID functions */
int amiport_getgid(void);
int amiport_getegid(void);
int amiport_setgid(int gid);

/* Group membership */
int amiport_getgroups(int size, int *list);
int amiport_getgrouplist(const char *user, int basegid, int *groups, int *ngroups);

/* OpenBSD convenience: gid -> name string */
const char *amiport_group_from_gid(int gid, int noname);

/* TTY name */
char *amiport_ttyname(int fd);

/* Convenience macros */
#ifndef AMIPORT_NO_GRP_MACROS
#define group         amiport_group
#define getgrgid(g)   amiport_getgrgid(g)
#define getgrnam(n)    amiport_getgrnam(n)
#define getgid()       amiport_getgid()
#define getegid()      amiport_getegid()
#define setgid(g)      amiport_setgid(g)
#define getgroups(s, l)  amiport_getgroups(s, l)
#define getgrouplist(u, b, g, n) amiport_getgrouplist(u, b, g, n)
#define group_from_gid(g, n) amiport_group_from_gid(g, n)
#define ttyname(fd)    amiport_ttyname(fd)
#endif

#endif /* AMIPORT_GRP_H */
