/*
 * file_io.c — POSIX file I/O wrappers for AmigaOS
 *
 * Maps open/close/read/write/lseek to AmigaDOS equivalents.
 * Uses a simple file descriptor table mapping int fds to BPTRs.
 */

#include <amiport/unistd.h>
#include <amiport/sys/stat.h>
#include <amiport/sys/time.h>
#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>

#include <errno.h>
#include <string.h>

/* File descriptor table */
#define AMIPORT_MAX_FDS 64

static BPTR fd_table[AMIPORT_MAX_FDS];
static int  fd_used[AMIPORT_MAX_FDS];
static int  fd_initialized = 0;

static void init_fd_table(void)
{
    int i;
    if (fd_initialized) return;

    for (i = 0; i < AMIPORT_MAX_FDS; i++) {
        fd_table[i] = 0;
        fd_used[i] = 0;
    }

    /* Reserve fd 0, 1, 2 for stdin, stdout, stderr */
    fd_table[0] = Input();
    fd_table[1] = Output();
    fd_table[2] = Output(); /* stderr = stdout on Amiga */
    fd_used[0] = 1;
    fd_used[1] = 1;
    fd_used[2] = 1;

    fd_initialized = 1;
}

static int alloc_fd(BPTR fh)
{
    int i;
    init_fd_table();

    for (i = 3; i < AMIPORT_MAX_FDS; i++) {
        if (!fd_used[i]) {
            fd_table[i] = fh;
            fd_used[i] = 1;
            return i;
        }
    }
    errno = EMFILE;
    return -1;
}

int amiport_open(const char *path, int flags)
{
    BPTR fh;
    LONG mode;

    init_fd_table();

    /*
     * Map POSIX flags to AmigaDOS modes:
     *
     *   POSIX pattern              AmigaDOS mode
     *   ─────────────────────────  ─────────────────
     *   O_RDONLY                   MODE_OLDFILE
     *   O_WRONLY | O_CREAT|O_TRUNC  MODE_NEWFILE
     *   O_WRONLY (no O_CREAT)     MODE_NEWFILE
     *   O_WRONLY | O_APPEND       MODE_READWRITE + Seek(END)
     *   O_RDWR                    MODE_READWRITE
     *   O_CREAT without O_TRUNC   try MODE_OLDFILE, fallback MODE_NEWFILE
     */
    if (flags & O_APPEND) {
        mode = MODE_READWRITE;
    } else if (flags & O_TRUNC) {
        mode = MODE_NEWFILE;
    } else if ((flags & O_CREAT) && !(flags & O_TRUNC)) {
        /* O_CREAT without O_TRUNC: open existing, or create if missing */
        fh = Open((CONST_STRPTR)path, MODE_OLDFILE);
        if (!fh) {
            if (IoErr() == ERROR_OBJECT_NOT_FOUND) {
                fh = Open((CONST_STRPTR)path, MODE_NEWFILE);
                if (!fh) {
                    amiport_map_errno();
                    return -1;
                }
            } else {
                amiport_map_errno();
                return -1;
            }
        }
        return alloc_fd(fh);
    } else if (flags & O_WRONLY) {
        mode = MODE_NEWFILE;
    } else if (flags & O_RDWR) {
        mode = MODE_READWRITE;
    } else {
        mode = MODE_OLDFILE;
    }

    fh = Open((CONST_STRPTR)path, mode);
    if (!fh) {
        amiport_map_errno();
        return -1;
    }

    /* Handle O_APPEND: seek to end */
    if (flags & O_APPEND) {
        Seek(fh, 0, OFFSET_END);
    }

    return alloc_fd(fh);
}

int amiport_close(int fd)
{
    init_fd_table();

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        errno = EBADF;
        return -1;
    }

    /* Don't close stdin/stdout/stderr */
    if (fd > 2) {
        Close(fd_table[fd]);
    }

    fd_table[fd] = 0;
    fd_used[fd] = 0;
    return 0;
}

LONG amiport_read(int fd, void *buf, LONG count)
{
    LONG result;

    init_fd_table();

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        errno = EBADF;
        return -1;
    }

    result = Read(fd_table[fd], buf, count);
    if (result == -1) {
        amiport_map_errno();
    }
    return result;
}

LONG amiport_write(int fd, const void *buf, LONG count)
{
    LONG result;

    init_fd_table();

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        errno = EBADF;
        return -1;
    }

    result = Write(fd_table[fd], (APTR)buf, count);
    if (result == -1) {
        amiport_map_errno();
    }
    return result;
}

LONG amiport_lseek(int fd, LONG offset, int whence)
{
    LONG amiga_whence;
    LONG old_pos;
    LONG new_pos;

    init_fd_table();

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        errno = EBADF;
        return -1;
    }

    /* Map POSIX whence to AmigaDOS */
    switch (whence) {
        case SEEK_SET: amiga_whence = OFFSET_BEGINNING; break;
        case SEEK_CUR: amiga_whence = OFFSET_CURRENT; break;
        case SEEK_END: amiga_whence = OFFSET_END; break;
        default:
            errno = EINVAL;
            return -1;
    }

    /* AmigaDOS Seek returns the OLD position, not the new one */
    old_pos = Seek(fd_table[fd], offset, amiga_whence);
    if (old_pos == -1) {
        amiport_map_errno();
        return -1;
    }

    /* Get the new position by seeking 0 from current */
    new_pos = Seek(fd_table[fd], 0, OFFSET_CURRENT);
    return new_pos;
}

int amiport_unlink(const char *path)
{
    if (!DeleteFile((CONST_STRPTR)path)) {
        amiport_map_errno();
        return -1;
    }
    return 0;
}

int amiport_rename(const char *oldpath, const char *newpath)
{
    if (!Rename((CONST_STRPTR)oldpath, (CONST_STRPTR)newpath)) {
        amiport_map_errno();
        return -1;
    }
    return 0;
}

int amiport_access(const char *path, int mode)
{
    BPTR lock;

    (void)mode; /* Amiga doesn't have Unix permission bits */

    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (!lock) {
        amiport_map_errno();
        return -1;
    }
    UnLock(lock);
    return 0;
}

int amiport_isatty(int fd)
{
    init_fd_table();

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        return 0;
    }

    return IsInteractive(fd_table[fd]) ? 1 : 0;
}

int amiport_fstat(int fd, struct amiport_stat *buf)
{
    struct FileInfoBlock fib;
    BOOL ok;

    init_fd_table();

    if (!buf) {
        errno = EINVAL;
        return -1;
    }

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        errno = EBADF;
        return -1;
    }

    memset(buf, 0, sizeof(struct amiport_stat));

    /* ExamineFH is available on AmigaOS 2.0+ (dos.library 36+) */
    ok = ExamineFH(fd_table[fd], &fib);
    if (!ok) {
        amiport_map_errno();
        return -1;
    }

    /* Fill stat structure — same logic as amiport_stat() */
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
