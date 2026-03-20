/*
 * amiport/dirent.h — POSIX directory operations shim for AmigaOS
 *
 * Provides opendir/readdir/closedir using AmigaDOS Lock/Examine/ExNext.
 */

#ifndef AMIPORT_DIRENT_H
#define AMIPORT_DIRENT_H

/* Maximum filename length on AmigaOS */
#define AMIPORT_MAXNAMLEN   108

/* Directory entry */
struct amiport_dirent {
    char d_name[AMIPORT_MAXNAMLEN];
    int  d_type;    /* 0 = file, 1 = directory */
};

/* Opaque directory handle */
typedef struct _AMIPORT_DIR AMIPORT_DIR;

AMIPORT_DIR          *amiport_opendir(const char *path);
struct amiport_dirent *amiport_readdir(AMIPORT_DIR *dir);
int                    amiport_closedir(AMIPORT_DIR *dir);

/* d_type values */
#define AMIPORT_DT_REG  0
#define AMIPORT_DT_DIR  1

#endif /* AMIPORT_DIRENT_H */
