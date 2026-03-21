/*
 * process.c — POSIX process and environment wrappers for AmigaOS
 *
 * Provides sleep, getcwd, chdir, getenv, and getpid.
 */

#include <amiport/unistd.h>
#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/*
 * Track whether we've called CurrentDir before. The initial directory
 * lock is owned by the system (CLI or Workbench) — we must not UnLock it.
 */
static BPTR saved_initial_lock = 0;
static int  chdir_initialized = 0;

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

    if (!chdir_initialized) {
        /* First call: save the system's initial lock, don't UnLock it */
        saved_initial_lock = old_lock;
        chdir_initialized = 1;
    } else if (old_lock && old_lock != saved_initial_lock) {
        /* Subsequent calls: free locks we created */
        UnLock(old_lock);
    }

    return 0;
}

/*
 * Returns a dynamically allocated string. Caller must free().
 * Note: This differs from POSIX getenv() which returns a pointer to
 * internal storage. We use dynamic allocation because AmigaOS GetVar()
 * copies into a caller-provided buffer, and a static buffer would be
 * overwritten by subsequent calls.
 */
char *amiport_getenv(const char *name)
{
    char tmp[256];
    LONG len;
    char *result;

    len = GetVar((CONST_STRPTR)name, (STRPTR)tmp, sizeof(tmp) - 1, 0);
    if (len < 0) {
        return NULL;
    }
    tmp[len] = '\0';

    result = (char *)malloc(len + 1);
    if (!result) {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(result, tmp, len + 1);
    return result;
}

LONG amiport_getpid(void)
{
    /* Return task address as a pseudo-PID */
    return (LONG)FindTask(NULL);
}

/*
 * Thread-safe strtok. Pure C — no AmigaOS calls needed.
 * saveptr tracks position between calls.
 */
char *amiport_strtok_r(char *str, const char *delim, char **saveptr)
{
    char *start;
    char *end;

    if (str != NULL) {
        *saveptr = str;
    }

    if (*saveptr == NULL) {
        return NULL;
    }

    /* Skip leading delimiters */
    start = *saveptr;
    while (*start != '\0' && strchr(delim, *start) != NULL) {
        start++;
    }

    if (*start == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    /* Find end of token */
    end = start;
    while (*end != '\0' && strchr(delim, *end) == NULL) {
        end++;
    }

    if (*end != '\0') {
        *end = '\0';
        *saveptr = end + 1;
    } else {
        *saveptr = NULL;
    }

    return start;
}

/*
 * Create a temporary file using the AmigaOS T: assign.
 * Returns FILE* opened for read/write ("w+b"), or NULL on failure.
 * Uses task address + counter for unique filenames.
 */
static int tmpfile_counter = 0;

FILE *amiport_tmpfile(void)
{
    char path[64];
    FILE *fp;
    LONG pid = (LONG)FindTask(NULL);

    /* Generate unique name using task address and counter */
    sprintf(path, "T:amiport_%lx_%d.tmp", (unsigned long)pid, tmpfile_counter++);

    fp = fopen(path, "w+b");
    if (!fp) {
        return NULL;
    }

    /* Delete-on-close: remove the file now, it stays accessible via fp
     * until fclose(). On AmigaOS this may not work perfectly (file is
     * deleted immediately), so we defer deletion. The file lives in T:
     * which is typically RAM: and cleaned on reboot. */

    return fp;
}

/*
 * setlocale — stub returning "C" locale
 *
 * amiport: AmigaOS has locale.library but classic -noixemul libnix
 * does not provide setlocale(). Most ported programs call
 * setlocale(LC_ALL, "") at startup for locale-aware sorting/collation.
 * This stub returns "C" for all categories, which is the POSIX default.
 */
char *
amiport_setlocale(int category, const char *locale)
{
    (void)category;
    (void)locale;
    return "C";
}
