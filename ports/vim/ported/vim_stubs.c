/*
 * vim_stubs.c -- AmigaOS stubs for functions not available on this platform.
 *
 * amiport: These stubs replace X11/Unix functions that are referenced by
 * vim source but have no AmigaOS equivalent.
 */

#include "vim.h"
#include <proto/dos.h>

/*
 * Input Method (IM) stubs.
 * AmigaOS has no X11 input method framework.
 */
    int
im_get_status(void)
{
    return FALSE;  /* IM always inactive */
}

    void
im_set_active(int active UNUSED)
{
    /* no-op: no IM support on AmigaOS */
}

    int
set_ref_in_im_funcs(int copyID UNUSED)
{
    return 0;  /* no refs to GC */
}

    char *
did_set_imactivatefunc(optset_T *args UNUSED)
{
    return NULL;  /* no error */
}

    char *
did_set_imstatusfunc(optset_T *args UNUSED)
{
    return NULL;  /* no error */
}

/*
 * mch_rmdir() -- Remove a directory.
 * AmigaOS: use DeleteFile() which works on empty directories.
 */
    int
mch_rmdir(char_u *name)
{
    /* amiport: DeleteFile removes empty dirs on AmigaOS */
    if (DeleteFile((STRPTR)name))
	return 0;
    return -1;
}

/*
 * getpwuid / getgrgid stubs.
 * POSIX user/group database not available on AmigaOS libnix.
 * Used in fileio.c for directory listing metadata -- just return NULL.
 * Note: struct passwd and struct group are declared in <pwd.h>/<grp.h>
 * from the NDK but the actual functions are not in libnix.
 */
#include <pwd.h>
#include <grp.h>

    struct passwd *
getpwuid(uid_t uid UNUSED)
{
    return NULL;
}

    struct group *
getgrgid(gid_t gid UNUSED)
{
    return NULL;
}

    uid_t
getuid(void)
{
    return 0;  /* single-user system */
}
