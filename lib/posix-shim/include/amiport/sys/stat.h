/*
 * amiport/sys/stat.h — Minimal stat() shim for AmigaOS
 *
 * Provides amiport_stat() using Lock()/Examine().
 */

#ifndef AMIPORT_SYS_STAT_H
#define AMIPORT_SYS_STAT_H

#include <exec/types.h>

/* Full POSIX-compatible stat structure for AmigaOS.
 * Fields not natively supported by AmigaOS are zero-filled by amiport_stat().
 * amiport: debug-agent -- extended to match CPython posixmodule.c expectations
 */
struct amiport_stat {
    ULONG st_mode;      /* File type and permissions */
    ULONG st_ino;       /* Inode number (from fib_DiskKey) */
    ULONG st_dev;       /* Device ID (from volume lock) */
    ULONG st_nlink;     /* Number of hard links (always 1 on AmigaOS) */
    ULONG st_uid;       /* User ID of owner (always 0 on AmigaOS) */
    ULONG st_gid;       /* Group ID of owner (always 0 on AmigaOS) */
    LONG  st_size;      /* File size in bytes */
    ULONG st_atime;     /* Time of last access (same as mtime on AmigaOS) */
    ULONG st_mtime;     /* Modification time (Unix timestamp approx) */
    ULONG st_ctime;     /* Time of last change (same as mtime on AmigaOS) */
    ULONG st_blksize;   /* Block size for filesystem I/O (always 512) */
    ULONG st_blocks;    /* Number of 512-byte blocks allocated */
    ULONG st_rdev;      /* Device ID if special file (always 0) */
    int   st_isdir;     /* Non-zero if directory (internal use) */
};

/* amiport: do NOT typedef struct amiport_stat to stat.
 * The NDK's <sys/stat.h> (pulled in by <fcntl.h> or directly) already
 * declares int stat() as a function, so typedef-ing the struct to the same
 * name causes "redeclared as different kind of symbol" errors.
 * We use only the #define stat amiport_stat macro approach below,
 * and code should use struct amiport_stat for the struct type directly. */

/* Mode flags */
#define AMIPORT_S_IFMT   0170000  /* Mask for file type bits */
#define AMIPORT_S_IFDIR  0040000
#define AMIPORT_S_IFREG  0100000

int amiport_stat(const char *path, struct amiport_stat *buf);
int amiport_fstat(int fd, struct amiport_stat *buf);

/*
 * amiport: lstat() — alias to amiport_stat().
 *
 * Classic FFS (OFS/FFS/SFS) has no symbolic links in the POSIX sense;
 * AmigaOS 2.0+ soft-links are rare and most ported code never exercises
 * them. Treating lstat as stat is the correct behaviour for 99% of ports.
 */
#define amiport_lstat(path, buf)  amiport_stat(path, buf)

/* Convenience macros */
#define AMIPORT_S_ISDIR(m)  (((m) & 0170000) == AMIPORT_S_IFDIR)
#define AMIPORT_S_ISREG(m)  (((m) & 0170000) == AMIPORT_S_IFREG)

#ifndef AMIPORT_NO_STAT_MACROS
#define S_IFMT    AMIPORT_S_IFMT
#define S_IFDIR   AMIPORT_S_IFDIR
#define S_IFREG   AMIPORT_S_IFREG
#define S_ISDIR   AMIPORT_S_ISDIR
#define S_ISREG   AMIPORT_S_ISREG
#define fstat     amiport_fstat
#define stat      amiport_stat
#define lstat     amiport_lstat
#endif

#endif /* AMIPORT_SYS_STAT_H */
