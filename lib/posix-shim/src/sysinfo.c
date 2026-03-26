/*
 * sysinfo.c -- System information stubs for AmigaOS
 *
 * Provides gethostname, sysconf, uname, getrusage, setproctitle.
 *
 * amiport: gethostname reads ENV:HOSTNAME via GetVar().
 * uname returns static AmigaOS/m68k info.
 * sysconf returns hardcoded values for common constants.
 * getrusage returns zeroes (no per-process accounting on AmigaOS).
 */

#include <amiport/utsname.h>
#include <amiport/unistd.h>
#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/var.h>

#include <errno.h>
#include <string.h>

/* --- gethostname --- */

int amiport_gethostname(char *name, int namelen)
{
    LONG len;

    if (!name || namelen <= 0) {
        errno = EINVAL;
        return -1;
    }

    /* amiport: Try ENV:HOSTNAME first (set by TCP/IP stacks like
     * AmiTCP, Roadshow, Miami). Fall back to "amiga" if not set. */
    len = GetVar((CONST_STRPTR)"HOSTNAME", (STRPTR)name, namelen - 1,
                 GVF_GLOBAL_ONLY);
    if (len < 0) {
        /* Try local scope (vamos doesn't support GVF_GLOBAL_ONLY) */
        len = GetVar((CONST_STRPTR)"HOSTNAME", (STRPTR)name, namelen - 1, 0);
    }

    if (len <= 0) {
        /* No HOSTNAME variable set (or empty) -- use default */
        if (namelen < 6) {
            errno = ENAMETOOLONG;
            return -1;
        }
        strcpy(name, "amiga");
    } else {
        name[len] = '\0';
    }

    return 0;
}

/* --- sysconf --- */

long amiport_sysconf(int name)
{
    switch (name) {
    case AMIPORT_SC_PAGESIZE:
        /* amiport: AmigaOS has no virtual memory / paging.
         * Return 4096 as a reasonable block size. */
        return 4096;

    case AMIPORT_SC_ARG_MAX:
        /* amiport: AmigaDOS command line limit is ~512 bytes
         * for the default shell. Return a conservative value. */
        return 4096;

    case AMIPORT_SC_OPEN_MAX:
        /* amiport: amiport fd table has 64 slots */
        return 64;

    case AMIPORT_SC_CLK_TCK:
        /* amiport: Amiga system clock is 50 Hz (PAL) */
        return 50;

    case AMIPORT_SC_NPROCESSORS_ONLN:
        /* amiport: single CPU always */
        return 1;

    case AMIPORT_SC_LINE_MAX:
        return 2048;

    default:
        errno = EINVAL;
        return -1;
    }
}

/* --- uname --- */

int amiport_uname(struct amiport_utsname *buf)
{
    if (!buf) {
        errno = EINVAL;
        return -1;
    }

    strcpy(buf->sysname, "AmigaOS");
    amiport_gethostname(buf->nodename, sizeof(buf->nodename));
    strcpy(buf->release, "3.1");
    strcpy(buf->version, "40.68");
    strcpy(buf->machine, "m68k");

    return 0;
}

/* --- getrusage --- */

int amiport_getrusage(int who, struct amiport_rusage *usage)
{
    (void)who;

    if (!usage) {
        errno = EINVAL;
        return -1;
    }

    /* amiport: AmigaOS has no per-process resource accounting.
     * Return all zeroes. Programs like md5/time use this for
     * elapsed time reporting -- they'll just see 0. */
    memset(usage, 0, sizeof(struct amiport_rusage));

    return 0;
}

/* --- setproctitle --- */

void amiport_setproctitle(const char *fmt, ...)
{
    /* amiport: AmigaOS has no process title mechanism visible
     * to other processes. pr_Task.tc_Node.ln_Name exists but
     * changing it is unsafe without Forbid()/Permit(). No-op. */
    (void)fmt;
}
