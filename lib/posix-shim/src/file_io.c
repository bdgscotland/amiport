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
#include <dos/dosextens.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <amiport/internal.h>

/* File descriptor table */
#define AMIPORT_MAX_FDS 64

static BPTR fd_table[AMIPORT_MAX_FDS];
static int  fd_used[AMIPORT_MAX_FDS];
static int  fd_closeable[AMIPORT_MAX_FDS]; /* amiport: whether Close() should be called on this fd */
static int  fd_initialized = 0;

/* FILE* to fd mapping (for fileno() support) */
#include <amiport/stdio.h>

amiport_file_entry amiport_files[AMIPORT_MAX_FILE_ENTRIES];
int amiport_file_count = 0;

static void init_fd_table(void)
{
    int i;
    if (fd_initialized) return;

    for (i = 0; i < AMIPORT_MAX_FDS; i++) {
        fd_table[i] = 0;
        fd_used[i] = 0;
        fd_closeable[i] = 0;
    }

    /* Reserve fd 0, 1, 2 for stdin, stdout, stderr.
     * These are not closeable — they belong to the shell. */
    fd_table[0] = Input();
    fd_table[1] = Output();
    fd_table[2] = Output(); /* stderr = stdout on Amiga */
    fd_used[0] = 1;
    fd_used[1] = 1;
    fd_used[2] = 1;
    /* fd_closeable[0..2] remain 0 — never Close() these */

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
            fd_closeable[i] = 1;
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

    /* amiport: Only call Close() if this fd is closeable AND no other fd
     * shares the same BPTR. This supports dup/dup2 — closing one duplicate
     * must not close the underlying file handle while others still use it. */
    if (fd_closeable[fd]) {
        BPTR fh = fd_table[fd];
        int shared = 0;
        int i;
        for (i = 0; i < AMIPORT_MAX_FDS; i++) {
            if (i != fd && fd_used[i] && fd_table[i] == fh) {
                shared = 1;
                break;
            }
        }
        if (!shared) {
            Close(fh);
        }
    }

    fd_table[fd] = 0;
    fd_used[fd] = 0;
    fd_closeable[fd] = 0;
    return 0;
}

/* amiport: dup() — duplicate a file descriptor */
int amiport_dup(int oldfd)
{
    int i;

    init_fd_table();

    if (oldfd < 0 || oldfd >= AMIPORT_MAX_FDS || !fd_used[oldfd]) {
        errno = EBADF;
        return -1;
    }

    /* Find the lowest free fd starting from 3 (0-2 are reserved) */
    for (i = 3; i < AMIPORT_MAX_FDS; i++) {
        if (!fd_used[i]) {
            fd_table[i] = fd_table[oldfd];
            fd_used[i] = 1;
            fd_closeable[i] = fd_closeable[oldfd];
            return i;
        }
    }

    errno = EMFILE;
    return -1;
}

/* amiport: dup2() — duplicate a file descriptor to a specific fd number */
int amiport_dup2(int oldfd, int newfd)
{
    init_fd_table();

    /* Validate oldfd */
    if (oldfd < 0 || oldfd >= AMIPORT_MAX_FDS || !fd_used[oldfd]) {
        errno = EBADF;
        return -1;
    }

    /* Validate newfd range */
    if (newfd < 0 || newfd >= AMIPORT_MAX_FDS) {
        errno = EBADF;
        return -1;
    }

    /* If oldfd == newfd, just return newfd (POSIX spec) */
    if (oldfd == newfd) {
        return newfd;
    }

    /* If newfd is currently open, close it first */
    if (fd_used[newfd]) {
        amiport_close(newfd);
    }

    /* Copy the BPTR and flags from oldfd to newfd */
    fd_table[newfd] = fd_table[oldfd];
    fd_used[newfd] = 1;
    fd_closeable[newfd] = fd_closeable[oldfd];

    return newfd;
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

/*
 * amiport: chmod() — no-op stub returning 0.
 *
 * AmigaOS protection bits have inverted semantics relative to POSIX
 * (set bit = permission DENIED, not granted). Attempting a real mapping
 * would confuse most ported programs. Most callers just want chmod to
 * succeed silently after creating a file. Return 0 always.
 */
int amiport_chmod(const char *path, unsigned int mode)
{
    (void)path;
    (void)mode;
    return 0;
}

/*
 * amiport: realpath() — resolve canonical path using Lock()+NameFromLock().
 *
 * AmigaOS has no symbolic links on classic FFS, so resolution is simply
 * obtaining the full volume-qualified path. If resolved is NULL we
 * malloc a 256-byte buffer (caller must free). Returns NULL on error.
 */
char *amiport_realpath(const char *path, char *resolved)
{
    BPTR lock;
    char *buf;
    BOOL ok;

    if (!path) {
        errno = EINVAL;
        return NULL;
    }

    if (resolved) {
        buf = resolved;
    } else {
        /* amiport: use malloc so the caller can free() it (POSIX realpath semantics) */
        buf = (char *)malloc(256);
        if (!buf) {
            errno = ENOMEM;
            return NULL;
        }
    }

    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (!lock) {
        amiport_map_errno();
        if (!resolved) free(buf);
        return NULL;
    }

    ok = NameFromLock(lock, (STRPTR)buf, 255);
    UnLock(lock);

    if (!ok) {
        amiport_map_errno();
        if (!resolved) free(buf);
        return NULL;
    }

    return buf;
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
    struct FileInfoBlock *fib;
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

    /* amiport: debug-agent — use AllocDosObject(DOS_FIB) for guaranteed longword alignment.
     * ExamineFH() requires the FIB to be longword-aligned; stack allocation with -m68000
     * may only guarantee 2-byte alignment, causing an Address Error inside dos.library. */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib) {
        errno = ENOMEM;
        return -1;
    }

    /* ExamineFH is available on AmigaOS 2.0+ (dos.library 36+) */
    ok = ExamineFH(fd_table[fd], fib);
    if (!ok) {
        amiport_map_errno();
        /* vamos may return failure with IoErr()==0; treat as I/O error */
        if (errno == 0) {
            errno = EIO;
        }
        FreeDosObject(DOS_FIB, fib);
        return -1;
    }

    /* Fill stat structure — same logic as amiport_stat() */
    if (fib->fib_DirEntryType > 0) {
        buf->st_mode = AMIPORT_S_IFDIR | 0755;
        buf->st_isdir = 1;
    } else {
        buf->st_mode = AMIPORT_S_IFREG | 0644;
        buf->st_isdir = 0;
    }

    buf->st_size = fib->fib_Size;

    /* Convert Amiga DateStamp to Unix timestamp */
    buf->st_mtime = (fib->fib_Date.ds_Days * 86400L) +
                    (fib->fib_Date.ds_Minute * 60L) +
                    (fib->fib_Date.ds_Tick / TICKS_PER_SECOND) +
                    AMIGA_EPOCH_OFFSET;

    FreeDosObject(DOS_FIB, fib);
    return 0;
}

/* FILE* wrappers to track fileno() mappings.
 * Undefine the fopen/fclose macros here so we can call the real libc functions
 * without triggering infinite recursion. */
#undef fopen
#undef fclose

FILE *
amiport_fopen(const char *path, const char *mode)
{
    FILE *fp;
    int fd;

    if (!mode || !*mode) {
        errno = EINVAL;
        return NULL;
    }

    /* Use the real libc fopen() for stdio buffering — this avoids opening
     * the file twice (once via amiport_open + once via libc fopen). */
    fp = fopen(path, mode);
    if (!fp)
        return NULL;

    /* Allocate a slot in our fd table so fileno() works.
     * We use a sentinel BPTR of 1 (non-zero = "in use") since we don't
     * have the raw AmigaDOS handle from libc's fopen. */
    init_fd_table();
    fd = -1;
    {
        int i;
        for (i = 3; i < AMIPORT_MAX_FDS; i++) {
            if (!fd_used[i]) {
                fd_table[i] = (BPTR)1; /* sentinel — not a real BPTR */
                fd_used[i] = 1;
                fd = i;
                break;
            }
        }
    }
    if (fd < 0) {
        /* fd table full — still return fp so the caller can use it,
         * but fileno() will return -1 for this handle. */
    }

    /* Register the FILE* -> fd mapping so fileno() can find it */
    if (fd >= 0 && amiport_file_count < AMIPORT_MAX_FILE_ENTRIES) {
        amiport_files[amiport_file_count].fp = fp;
        amiport_files[amiport_file_count].fd = fd;
        amiport_file_count++;
    }

    return fp;
}

int
amiport_fclose(FILE *fp)
{
    int i, fd = -1;
    int result;

    if (!fp)
        return -1;

    /* Find and remove from our table */
    for (i = 0; i < amiport_file_count; i++) {
        if (amiport_files[i].fp == fp) {
            fd = amiport_files[i].fd;
            /* Remove by shifting remaining entries */
            for (; i < amiport_file_count - 1; i++)
                amiport_files[i] = amiport_files[i + 1];
            amiport_file_count--;
            break;
        }
    }

    /* Close the FILE* using the real libc fclose */
    result = fclose(fp);

    /* Release our fd table slot (sentinel BPTR — don't call AmigaDOS Close) */
    if (fd >= 0 && fd < AMIPORT_MAX_FDS) {
        fd_table[fd] = 0;
        fd_used[fd] = 0;
    }

    return result;
}

/*
 * fdopen — associate a FILE* stream with an existing amiport file descriptor
 *
 * amiport: uses NameFromFH() to recover the path from the BPTR, then
 * re-opens via libc fopen(). The original fd remains valid but the file
 * position is NOT shared between the fd and the FILE* (they are independent
 * opens). This covers the common use case of fdopen() after open() on
 * regular files. Does not work on pipe fds (NameFromFH fails on PIPE:).
 */
#undef fdopen
FILE *
amiport_fdopen(int fd, const char *mode)
{
    BPTR fh;
    char namebuf[256];
    FILE *fp;
    LONG len;

    init_fd_table();

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        errno = EBADF;
        return NULL;
    }
    if (!mode || !*mode) {
        errno = EINVAL;
        return NULL;
    }

    fh = fd_table[fd];
    if (fh == 0 || fh == (BPTR)1) {
        /* Sentinel BPTR (from amiport_fopen) or NULL — can't recover path */
        errno = EBADF;
        return NULL;
    }

    /* Recover filename from the BPTR via AmigaDOS */
    len = NameFromFH(fh, (STRPTR)namebuf, sizeof(namebuf));
    if (!len) {
        /* NameFromFH failed — likely a PIPE: or console handle */
        errno = ENOTSUP;
        return NULL;
    }

    /* Re-open via libc fopen for proper FILE* buffering */
    fp = fopen(namebuf, mode);
    if (!fp)
        return NULL;

    /* Register in the file mapping table (reuse existing fd) */
    if (amiport_file_count < AMIPORT_MAX_FILE_ENTRIES) {
        amiport_files[amiport_file_count].fp = fp;
        amiport_files[amiport_file_count].fd = fd;
        amiport_file_count++;
    }

    return fp;
}

/*
 * symlink -- create a soft link via AmigaDOS MakeLink()
 *
 * amiport: MakeLink(name, dest, LINK_SOFT) creates a soft link.
 * LINK_SOFT is 0 (soft link), LINK_HARD is 1 (hard link).
 * Soft links are supported on FFS since AmigaOS 2.0+ but are
 * rarely used. Returns 0 on success, -1 on error.
 */
int amiport_symlink(const char *target, const char *linkpath)
{
    if (!target || !linkpath) {
        errno = EINVAL;
        return -1;
    }

    /* MakeLink() with soft=FALSE (0) creates a soft link,
     * soft=TRUE (non-zero) would need a Lock (hard link).
     * For soft links, the second arg is a pointer to the path string,
     * NOT a Lock. The BCPL ABI treats it as LONG but we cast. */
    if (!MakeLink((CONST_STRPTR)linkpath, (LONG)target, 0)) {
        amiport_map_errno();
        return -1;
    }

    return 0;
}

/*
 * fchmod -- no-op stub
 *
 * amiport: Amiga protection bits have inverted semantics.
 * Most ported code just needs this to succeed silently.
 */
int amiport_fchmod(int fd, unsigned int mode)
{
    (void)fd;
    (void)mode;
    return 0;
}

/*
 * fchown -- no-op stub
 *
 * amiport: AmigaOS is single-user; no ownership concept.
 */
int amiport_fchown(int fd, int owner, int group)
{
    (void)fd;
    (void)owner;
    (void)group;
    return 0;
}

/*
 * lchown -- no-op stub (like fchown but for paths/symlinks)
 */
int amiport_lchown(const char *path, int owner, int group)
{
    (void)path;
    (void)owner;
    (void)group;
    return 0;
}

/* --- Internal API (used by posix-emu and bsdsocket-shim) --- */

BPTR amiport_fd_to_fh(int fd)
{
    init_fd_table();
    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd])
        return 0;
    return fd_table[fd];
}

int amiport_fd_is_valid(int fd)
{
    init_fd_table();
    if (fd < 0 || fd >= AMIPORT_MAX_FDS)
        return 0;
    return fd_used[fd];
}

/*
 * readlink — read the target of an AmigaDOS soft link (OS 2.0+)
 *
 * amiport: AmigaDOS ReadLink() is designed to be called after receiving
 * ERROR_IS_SOFT_LINK from a filesystem operation.  The correct sequence is:
 *
 *   1. Attempt Lock() on the path
 *   2. If Lock() fails with ERROR_IS_SOFT_LINK, call GetDeviceProc() on
 *      the path to get the filesystem MsgPort
 *   3. Call ReadLink(port, parentlock, name, buf, size) to retrieve the
 *      soft-link target string
 *   4. FreeDeviceProc() when done
 *
 * If Lock() succeeds the object is NOT a soft link — return -1/EINVAL.
 * If Lock() fails for any other reason, map and return -1/errno.
 *
 * Returns the number of bytes written to buf (not NUL-terminated), or -1.
 */
LONG
amiport_readlink(const char *path, char *buf, size_t bufsiz)
{
    BPTR lock;
    LONG ioerr;
    struct DevProc *dvp;
    LONG result;

    if (!path || !buf || bufsiz == 0) {
        errno = EINVAL;
        return -1;
    }

    /* Step 1: try to lock the path */
    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (lock) {
        /* Lock succeeded — object is NOT a soft link */
        UnLock(lock);
        errno = EINVAL;
        return -1;
    }

    ioerr = IoErr();

    if (ioerr != ERROR_IS_SOFT_LINK) {
        /* Some other error (not found, permission, etc.) */
        amiport_map_errno();
        /* Guard: if amiport_map_errno() left errno=0 (e.g. vamos quirk
         * where IoErr() returns 0 for OBJECT_NOT_FOUND), default to ENOENT. */
        if (errno == 0)
            errno = ENOENT;
        return -1;
    }

    /* Step 2: get the filesystem handler for this path */
    dvp = GetDeviceProc((CONST_STRPTR)path, NULL);
    if (!dvp) {
        amiport_map_errno();
        return -1;
    }

    /* Step 3: ask the filesystem for the link target */
    result = ReadLink(
        dvp->dvp_Port,
        dvp->dvp_Lock,
        (CONST_STRPTR)path,
        (STRPTR)buf,
        (ULONG)bufsiz
    );

    /* Step 4: release the device proc */
    FreeDeviceProc(dvp);

    if (result == -1) {
        /* Filesystem refused — treat as EINVAL (not actually a soft link) */
        errno = EINVAL;
        return -1;
    }

    if (result == -2) {
        errno = ENAMETOOLONG;
        return -1;
    }

    /* result = number of bytes written (not NUL-terminated, per POSIX) */
    return result;
}

/*
 * ftruncate — truncate or extend an open file to a specified length
 *
 * amiport: uses AmigaDOS SetFileSize() (dos.library 36+, OS 2.0+).
 * Translates the file descriptor to a BPTR via amiport_fd_to_fh().
 *
 * Caveat: when extending, AmigaDOS does NOT guarantee the new bytes are
 * zero-filled (unlike POSIX).  Programs that depend on zero-fill after
 * ftruncate() must write the zeros explicitly.
 */
int
amiport_ftruncate(int fd, LONG length)
{
    BPTR fh;

    if (length < 0) {
        errno = EINVAL;
        return -1;
    }

    fh = amiport_fd_to_fh(fd);
    if (!fh) {
        errno = EBADF;
        return -1;
    }

    /*
     * SetFileSize(fh, offset, mode):
     *   offset — new file size
     *   mode   — OFFSET_BEGINNING means size from start of file
     * Returns -1 on failure (read-only volume, disk full, etc.)
     */
    if (SetFileSize(fh, length, OFFSET_BEGINNING) == -1) {
        amiport_map_errno();
        return -1;
    }

    return 0;
}
