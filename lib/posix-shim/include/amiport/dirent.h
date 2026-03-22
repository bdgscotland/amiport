/*
 * amiport/dirent.h — POSIX directory operations shim for AmigaOS
 *
 * Provides opendir/readdir/closedir using AmigaDOS Lock/Examine/ExNext.
 */

#ifndef AMIPORT_DIRENT_H
#define AMIPORT_DIRENT_H

/* Maximum filename length on AmigaOS */
#define AMIPORT_MAXNAMLEN   108

/* Directory entry — named as both amiport_dirent and dirent via macro below */
struct amiport_dirent {
    char d_name[AMIPORT_MAXNAMLEN];
    int  d_type;    /* 0 = file, 1 = directory */
    int  d_fileno;  /* Inode-like number (always 1 on AmigaOS) */
};

/* Opaque directory handle */
typedef struct _AMIPORT_DIR AMIPORT_DIR;

AMIPORT_DIR           *amiport_opendir(const char *path);
struct amiport_dirent *amiport_readdir(AMIPORT_DIR *dir);
int                    amiport_closedir(AMIPORT_DIR *dir);

/*
 * amiport: mkdir() — create directory via AmigaDOS CreateDir().
 * Mode bits are ignored (Amiga protection bits have inverted semantics).
 */
int amiport_mkdir(const char *path, unsigned int mode);

/*
 * amiport: rmdir() — remove directory via AmigaDOS DeleteFile().
 * On AmigaOS, files and directories are deleted with the same call.
 */
int amiport_rmdir(const char *path);

/* d_type values */
#define AMIPORT_DT_REG  0
#define AMIPORT_DT_DIR  1

/* Drop-in POSIX compatibility macros */
#ifndef AMIPORT_NO_DIRENT_MACROS
#define dirent      amiport_dirent
#define DIR         AMIPORT_DIR
#define opendir     amiport_opendir
#define readdir     amiport_readdir
#define closedir    amiport_closedir
#define DT_REG      AMIPORT_DT_REG
#define DT_DIR      AMIPORT_DT_DIR
#endif

#ifndef AMIPORT_NO_MKDIR_MACROS
#define mkdir(p, m)  amiport_mkdir(p, m)
#define rmdir(p)     amiport_rmdir(p)
#endif

#endif /* AMIPORT_DIRENT_H */
