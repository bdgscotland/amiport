/*
 * errno_map.c — AmigaDOS IoErr() to POSIX errno mapping
 */

#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <dos/dos.h>

#include <errno.h>

/*
 * Weak errno storage — makes libamiport.a self-contained w.r.t. errno.
 *
 * PROBLEM: bebbo-gcc's collect2 places -lamiport BEFORE the libnix archive
 * group (-( -lnix20 -lnixmain -lnix -lstubs -lamiga -lgcc -)). The linker
 * processes -lamiport first, and when it does, the errno symbol that libnix
 * provides is not yet available. The group does not re-scan -lamiport after
 * resolving libnix, so errno references inside libamiport stay unresolved.
 *
 * FIX: provide errno-related symbols as weak definitions here. Weak symbols
 * are satisfied immediately (so intra-archive references resolve at scan
 * time), but are overridden by the strong definitions that libnix supplies
 * later. The final binary always uses libnix's authoritative errno storage.
 *
 * Two libnix errno variants are handled:
 *
 *   Variant A — crosstools image (amigadev/crosstools:m68k-amigaos):
 *     errno.h: #define errno (*__errno)
 *     __errno is int * defined in libnix20.a
 *     Fix: weak int * __errno pointing to local fallback storage.
 *
 *   Variant B — m68k-amigaos-gcc image (amigadev/m68k-amigaos-gcc):
 *     errno.h: extern int errno  (direct global, __libnix__ mode)
 *     errno is a common int defined in libnix.a (__errno.o)
 *     Fix: weak int errno (common symbols merge; strong wins).
 *
 * Detection: if errno.h defines errno as a macro, Variant A is in use.
 * Otherwise Variant B applies.
 */
#ifdef __libnix__

#ifdef errno
/* Variant A: errno is #define errno (*__errno)
 *
 * PROBLEM: libc.a defines __errno as a FUNCTION (int *__errno(void))
 * returning __impure_ptr (newlib reent struct, _errno at offset 0).
 * errno.h declares `extern int *__errno` (data pointer). Our weak
 * data variable wins over libc.a's archive function at link time,
 * causing amiport code and user code to use different errno storage.
 *
 * FIX: Keep the weak __errno data pointer (needed for errno.h macro),
 * but in amiport_map_errno() ALSO write through __impure_ptr to
 * hit the real libnix errno storage. Belt-and-suspenders approach:
 * if either path reaches the caller, errno is visible. */
static int __amiport_errno_fallback = 0;
extern int * __errno;
int * __attribute__((weak)) __errno = &__amiport_errno_fallback;

/* Direct access to libnix's real errno storage.
 * __impure_ptr is defined by libc.a startup; errno is at offset 0. */
extern void *__impure_ptr;
void * __attribute__((weak)) __impure_ptr = (void *)0;

#else
/* Variant B: errno is extern int errno — emit weak common definition */
int __attribute__((weak)) errno;
#endif /* errno macro check */

#endif /* __libnix__ */

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
    int val = amiport_errno_from_ioerr(ioerr);
    /*
     * amiport: if IoErr() returned 0 but we got here, the operation DID fail
     * (callers only call map_errno on failure paths). vamos often returns
     * IoErr()==0 for missing files. Default to ENOENT since file-not-found
     * is the most common failure mode. Better than errno==0 (no error) which
     * is always wrong when the operation failed.
     */
    if (val == 0)
        val = ENOENT;

    /* Write errno through the macro (hits our weak __errno pointer) */
    errno = val;

#ifdef __libnix__
    /* Belt-and-suspenders: also write directly to libnix's real errno
     * storage via __impure_ptr. Our weak __errno data variable may point
     * to fallback storage instead of libnix's real errno (because libc.a
     * defines __errno as a FUNCTION but errno.h declares it as a data
     * pointer -- our weak data wins, hiding libc.a's function). Writing
     * through __impure_ptr hits the real storage that calling code reads. */
    if (__impure_ptr)
        *((int *)__impure_ptr) = val;
#endif

    return val;
}
