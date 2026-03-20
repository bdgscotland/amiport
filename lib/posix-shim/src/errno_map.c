/*
 * errno_map.c — AmigaDOS IoErr() to POSIX errno mapping
 */

#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <dos/dos.h>

#include <errno.h>

int amiport_errno_from_ioerr(long ioerr)
{
    switch (ioerr) {
        case 0:
            return 0;

        /* Object errors */
        case ERROR_OBJECT_NOT_FOUND:
            return ENOENT;
        case ERROR_OBJECT_EXISTS:
            return EEXIST;
        case ERROR_OBJECT_IN_USE:
            return EBUSY;
        case ERROR_OBJECT_WRONG_TYPE:
            return EINVAL;
        case ERROR_OBJECT_TOO_LARGE:
            return EFBIG;

        /* Directory/path errors */
        case ERROR_DIR_NOT_FOUND:
            return ENOENT;
        case ERROR_INVALID_COMPONENT_NAME:
            return ENAMETOOLONG;

        /* Disk errors */
        case ERROR_DISK_FULL:
            return ENOSPC;
        case ERROR_DISK_WRITE_PROTECTED:
            return EROFS;
        case ERROR_DEVICE_NOT_MOUNTED:
            return ENODEV;

        /* Access errors */
        case ERROR_READ_PROTECTED:
            return EACCES;
        case ERROR_WRITE_PROTECTED:
            return EACCES;
        case ERROR_DELETE_PROTECTED:
            return EACCES;

        /* Lock errors */
        case ERROR_LOCK_COLLISION:
            return EACCES;
        case ERROR_LOCK_TIMEOUT:
            return EAGAIN;

        /* Seek errors */
        case ERROR_SEEK_ERROR:
            return EINVAL;

        /* General */
        case ERROR_NO_FREE_STORE:
            return ENOMEM;
        case ERROR_ACTION_NOT_KNOWN:
            return ENOSYS;
        case ERROR_NOT_IMPLEMENTED:
            return ENOSYS;

        /* Directory scanning */
        case ERROR_NO_MORE_ENTRIES:
            return 0; /* Not an error — end of directory */

        default:
            return EIO; /* Generic I/O error for unmapped codes */
    }
}

int amiport_map_errno(void)
{
    LONG ioerr = IoErr();
    errno = amiport_errno_from_ioerr(ioerr);
    return errno;
}
