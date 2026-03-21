/*
 * amiport/sys/stat.h — Minimal stat() shim for AmigaOS
 *
 * Provides amiport_stat() using Lock()/Examine().
 */

#ifndef AMIPORT_SYS_STAT_H
#define AMIPORT_SYS_STAT_H

#include <exec/types.h>

/* Minimal stat structure */
struct amiport_stat {
    ULONG st_mode;      /* File type and permissions */
    LONG  st_size;      /* File size in bytes */
    ULONG st_mtime;     /* Modification time (Unix timestamp approx) */
    int   st_isdir;     /* Non-zero if directory */
    ULONG st_dev;       /* Device ID (from volume lock) */
    ULONG st_ino;       /* Inode number (from fib_DiskKey — unique per file on a volume) */
};

/* POSIX compatibility alias */
typedef struct amiport_stat stat;

/* Mode flags */
#define AMIPORT_S_IFMT   0170000  /* Mask for file type bits */
#define AMIPORT_S_IFDIR  0040000
#define AMIPORT_S_IFREG  0100000

int amiport_stat(const char *path, struct amiport_stat *buf);
int amiport_fstat(int fd, struct amiport_stat *buf);

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
#endif

#endif /* AMIPORT_SYS_STAT_H */
