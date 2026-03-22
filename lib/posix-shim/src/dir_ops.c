/*
 * dir_ops.c — POSIX directory operations for AmigaOS
 *
 * Implements opendir/readdir/closedir using Lock/Examine/ExNext.
 */

#include <amiport/dirent.h>
#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <errno.h>
#include <string.h>

/* Internal directory structure */
struct _AMIPORT_DIR {
    BPTR                   lock;
    struct FileInfoBlock   fib;
    struct amiport_dirent  entry;
    int                    first;   /* 1 = need Examine, 0 = use ExNext */
};

AMIPORT_DIR *amiport_opendir(const char *path)
{
    AMIPORT_DIR *dir;
    BPTR lock;

    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (!lock) {
        amiport_map_errno();
        return NULL;
    }

    dir = (AMIPORT_DIR *)AllocVec(sizeof(AMIPORT_DIR), MEMF_PUBLIC | MEMF_CLEAR);
    if (!dir) {
        UnLock(lock);
        errno = ENOMEM;
        return NULL;
    }

    dir->lock = lock;
    dir->first = 1;

    /* Initial Examine to set up the FileInfoBlock */
    if (!Examine(lock, &dir->fib)) {
        amiport_map_errno();
        UnLock(lock);
        FreeVec(dir);
        return NULL;
    }

    /* Verify it's actually a directory */
    if (dir->fib.fib_DirEntryType < 0) {
        UnLock(lock);
        FreeVec(dir);
        errno = ENOTDIR;
        return NULL;
    }

    return dir;
}

struct amiport_dirent *amiport_readdir(AMIPORT_DIR *dir)
{
    if (!dir) {
        errno = EBADF;
        return NULL;
    }

    /* Use ExNext to get the next entry */
    if (!ExNext(dir->lock, &dir->fib)) {
        /* ERROR_NO_MORE_ENTRIES is normal end-of-directory */
        if (IoErr() == ERROR_NO_MORE_ENTRIES) {
            return NULL;
        }
        amiport_map_errno();
        return NULL;
    }

    /* Copy filename to our dirent structure */
    strncpy(dir->entry.d_name, dir->fib.fib_FileName, AMIPORT_MAXNAMLEN - 1);
    dir->entry.d_name[AMIPORT_MAXNAMLEN - 1] = '\0';

    /* Set type: positive DirEntryType = directory */
    dir->entry.d_type = (dir->fib.fib_DirEntryType > 0) ?
                         AMIPORT_DT_DIR : AMIPORT_DT_REG;
    dir->entry.d_fileno = 1; /* No real inode on AmigaOS */

    return &dir->entry;
}

int amiport_closedir(AMIPORT_DIR *dir)
{
    if (!dir) {
        errno = EBADF;
        return -1;
    }

    UnLock(dir->lock);
    FreeVec(dir);
    return 0;
}

/*
 * amiport: mkdir() — create a directory using AmigaDOS CreateDir().
 *
 * Mode bits are ignored — AmigaOS protection bits have inverted semantics
 * relative to POSIX. CreateDir() returns a lock on success which must be
 * UnLock'd immediately.
 */
int amiport_mkdir(const char *path, unsigned int mode)
{
    BPTR lock;

    (void)mode; /* amiport: mode bits ignored — no POSIX permission model on AmigaOS */

    lock = CreateDir((CONST_STRPTR)path);
    if (!lock) {
        amiport_map_errno();
        return -1;
    }
    UnLock(lock);
    return 0;
}

/*
 * amiport: rmdir() — remove a directory using AmigaDOS DeleteFile().
 *
 * On AmigaOS, directories and files are both deleted with DeleteFile().
 * Returns -1 with errno set if the path does not exist or is non-empty.
 */
int amiport_rmdir(const char *path)
{
    if (!DeleteFile((CONST_STRPTR)path)) {
        amiport_map_errno();
        return -1;
    }
    return 0;
}
