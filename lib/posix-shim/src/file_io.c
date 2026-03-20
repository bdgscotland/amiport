/*
 * file_io.c — POSIX file I/O wrappers for AmigaOS
 *
 * Maps open/close/read/write/lseek to AmigaDOS equivalents.
 * Uses a simple file descriptor table mapping int fds to BPTRs.
 */

#include <amiport/unistd.h>
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

    /* Map POSIX flags to AmigaDOS modes */
    if (flags & O_WRONLY) {
        if (flags & O_APPEND) {
            mode = MODE_READWRITE;
        } else {
            mode = MODE_NEWFILE;
        }
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

unsigned int amiport_sleep(unsigned int seconds)
{
    /* Delay() takes ticks (1/50s on PAL) */
    Delay(seconds * 50);
    return 0;
}

char *amiport_getcwd(char *buf, int size)
{
    BPTR lock;

    lock = Lock("", SHARED_LOCK);
    if (!lock) {
        amiport_map_errno();
        return NULL;
    }

    if (!NameFromLock(lock, (STRPTR)buf, size)) {
        UnLock(lock);
        amiport_map_errno();
        return NULL;
    }

    UnLock(lock);
    return buf;
}

int amiport_chdir(const char *path)
{
    BPTR lock;
    BPTR old_lock;

    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (!lock) {
        amiport_map_errno();
        return -1;
    }

    old_lock = CurrentDir(lock);
    if (old_lock) {
        UnLock(old_lock);
    }
    return 0;
}

char *amiport_getenv(const char *name)
{
    static char env_buf[256];
    LONG len;

    len = GetVar((CONST_STRPTR)name, (STRPTR)env_buf, sizeof(env_buf) - 1, 0);
    if (len < 0) {
        return NULL;
    }
    env_buf[len] = '\0';
    return env_buf;
}

LONG amiport_getpid(void)
{
    /* Return task address as a pseudo-PID */
    return (LONG)FindTask(NULL);
}

int amiport_isatty(int fd)
{
    init_fd_table();

    if (fd < 0 || fd >= AMIPORT_MAX_FDS || !fd_used[fd]) {
        return 0;
    }

    return IsInteractive(fd_table[fd]) ? 1 : 0;
}
