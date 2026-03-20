/*
 * stat.c — Minimal stat() implementation for AmigaOS
 *
 * Uses Lock()/Examine() to fill a simplified stat structure.
 */

#include <amiport/sys/stat.h>
#include <amiport/sys/time.h>
#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <dos/dos.h>

#include <errno.h>
#include <string.h>

int amiport_stat(const char *path, struct amiport_stat *buf)
{
    BPTR lock;
    struct FileInfoBlock fib;

    if (!buf) {
        errno = EINVAL;
        return -1;
    }

    memset(buf, 0, sizeof(struct amiport_stat));

    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (!lock) {
        amiport_map_errno();
        return -1;
    }

    if (!Examine(lock, &fib)) {
        amiport_map_errno();
        UnLock(lock);
        return -1;
    }

    UnLock(lock);

    /* Fill stat structure */
    if (fib.fib_DirEntryType > 0) {
        buf->st_mode = AMIPORT_S_IFDIR | 0755;
        buf->st_isdir = 1;
    } else {
        buf->st_mode = AMIPORT_S_IFREG | 0644;
        buf->st_isdir = 0;
    }

    buf->st_size = fib.fib_Size;

    /* Convert Amiga DateStamp to Unix timestamp */
    buf->st_mtime = (fib.fib_Date.ds_Days * 86400L) +
                    (fib.fib_Date.ds_Minute * 60L) +
                    (fib.fib_Date.ds_Tick / TICKS_PER_SECOND) +
                    AMIGA_EPOCH_OFFSET;

    return 0;
}

/* amiport_fstat() is implemented in file_io.c where it has access to the fd table */
