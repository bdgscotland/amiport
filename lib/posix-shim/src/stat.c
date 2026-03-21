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
#include <dos/dosextens.h>

#include <errno.h>
#include <string.h>

int amiport_stat(const char *path, struct amiport_stat *buf)
{
    BPTR lock;
    struct FileInfoBlock *fib;

    if (!buf) {
        errno = EINVAL;
        return -1;
    }

    memset(buf, 0, sizeof(struct amiport_stat));

    /* amiport: debug-agent — use AllocDosObject(DOS_FIB) instead of a stack-allocated
     * FileInfoBlock. AmigaOS Examine() requires the FIB to be longword-aligned (4-byte).
     * With -m68000 and gcc's default stack layout, the compiler may only guarantee 2-byte
     * alignment for stack structs, causing an Address Error trap inside dos.library which
     * manifests as a recoverable Alert (0x0100 000F) rather than an Enforcer hit.
     * AllocDosObject(DOS_FIB, NULL) allocates from chip/fast RAM with guaranteed alignment
     * and zero-initializes the structure. */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib) {
        errno = ENOMEM;
        return -1;
    }

    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (!lock) {
        amiport_map_errno();
        FreeDosObject(DOS_FIB, fib);
        return -1;
    }

    if (!Examine(lock, fib)) {
        amiport_map_errno();
        UnLock(lock);
        FreeDosObject(DOS_FIB, fib);
        return -1;
    }

    UnLock(lock);

    /* Fill stat structure */
    if (fib->fib_DirEntryType > 0) {
        buf->st_mode = AMIPORT_S_IFDIR | 0755;
        buf->st_isdir = 1;
    } else {
        buf->st_mode = AMIPORT_S_IFREG | 0644;
        buf->st_isdir = 0;
    }

    buf->st_size = fib->fib_Size;

    /* amiport: populate st_ino from fib_DiskKey — this is the filesystem
     * block number of the file header, unique per file on a volume.
     * Without this, programs that compare st_dev/st_ino to detect "same file"
     * (like diff's files_differ()) will treat all files as identical. */
    buf->st_ino = (ULONG)fib->fib_DiskKey;

    /* amiport: populate st_dev from the volume's device node.
     * Use the Lock's volume pointer — files on different volumes get
     * different st_dev values. */
    {
        struct DevProc *dp;
        dp = GetDeviceProc((CONST_STRPTR)path, NULL);
        if (dp) {
            buf->st_dev = (ULONG)dp->dvp_Port;
            FreeDeviceProc(dp);
        }
    }

    /* Convert Amiga DateStamp to Unix timestamp */
    buf->st_mtime = (fib->fib_Date.ds_Days * 86400L) +
                    (fib->fib_Date.ds_Minute * 60L) +
                    (fib->fib_Date.ds_Tick / TICKS_PER_SECOND) +
                    AMIGA_EPOCH_OFFSET;

    FreeDosObject(DOS_FIB, fib);
    return 0;
}

/* amiport_fstat() is implemented in file_io.c where it has access to the fd table */
