/*
 * stdio_ext.c — Extended stdio functions for AmigaOS
 *
 * Provides vasprintf/asprintf (dynamic string formatting) and
 * mkstemp (secure temporary file creation).
 */

#include <amiport/stdio_ext.h>
#include <amiport/unistd.h>
#include <amiport/errno_map.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*
 * vasprintf — formatted print to dynamically allocated string
 *
 * The caller must free() the returned *strp on success.
 * Returns the number of characters printed (excluding NUL),
 * or -1 on error.
 */
int
amiport_vasprintf(char **strp, const char *fmt, va_list ap)
{
    va_list ap_copy;
    int len;
    char *buf;

    if (!strp || !fmt) {
        errno = EINVAL;
        return -1;
    }

    /* First pass: determine required length.
     * C89 does not guarantee vsnprintf returns the needed length,
     * but bebbo-gcc's newlib does. Use a small stack buffer to probe. */
    va_copy(ap_copy, ap);
    len = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);

    if (len < 0) {
        *strp = NULL;
        return -1;
    }

    buf = (char *)malloc(len + 1);
    if (!buf) {
        *strp = NULL;
        errno = ENOMEM;
        return -1;
    }

    vsnprintf(buf, len + 1, fmt, ap);
    *strp = buf;
    return len;
}

/*
 * asprintf — formatted print to dynamically allocated string
 */
int
amiport_asprintf(char **strp, const char *fmt, ...)
{
    va_list ap;
    int result;

    va_start(ap, fmt);
    result = amiport_vasprintf(strp, fmt, ap);
    va_end(ap);
    return result;
}

/*
 * mkstemp — create a temporary file with unique name
 *
 * The template must end with "XXXXXX" which gets replaced with
 * a unique suffix. Returns an amiport fd opened for read/write,
 * or -1 on error.
 *
 * On AmigaOS we use T: assign for temporary files and generate
 * unique names using the task address and a counter.
 */
static int mkstemp_counter = 0;

int
amiport_mkstemp(char *tmpl)
{
    int len;
    int fd;
    LONG pid;
    char suffix[7];
    int i;

    if (!tmpl) {
        errno = EINVAL;
        return -1;
    }

    len = strlen(tmpl);
    if (len < 6) {
        errno = EINVAL;
        return -1;
    }

    /* Verify template ends with XXXXXX */
    if (strcmp(tmpl + len - 6, "XXXXXX") != 0) {
        errno = EINVAL;
        return -1;
    }

    pid = (LONG)FindTask(NULL);

    /* Generate a unique suffix from pid + counter */
    i = mkstemp_counter++;
    sprintf(suffix, "%02lx%04x",
            (unsigned long)(pid & 0xFF), (unsigned int)(i & 0xFFFF));

    /* Replace XXXXXX in template */
    memcpy(tmpl + len - 6, suffix, 6);

    /* Open for read/write, create new file */
    fd = amiport_open(tmpl, O_RDWR | O_CREAT | O_TRUNC);
    if (fd < 0) {
        /* Restore template on failure */
        memcpy(tmpl + len - 6, "XXXXXX", 6);
        return -1;
    }

    return fd;
}

/*
 * pread — read at offset without changing file position
 */
LONG
amiport_pread(int fd, void *buf, LONG count, LONG offset)
{
    LONG saved_pos;
    LONG result;

    /* Save current position */
    saved_pos = amiport_lseek(fd, 0, SEEK_CUR);
    if (saved_pos < 0)
        return -1;

    /* Seek to requested offset */
    if (amiport_lseek(fd, offset, SEEK_SET) < 0)
        return -1;

    /* Read data */
    result = amiport_read(fd, buf, count);

    /* Restore original position */
    amiport_lseek(fd, saved_pos, SEEK_SET);

    return result;
}

/*
 * pwrite — write at offset without changing file position
 */
LONG
amiport_pwrite(int fd, const void *buf, LONG count, LONG offset)
{
    LONG saved_pos;
    LONG result;

    saved_pos = amiport_lseek(fd, 0, SEEK_CUR);
    if (saved_pos < 0)
        return -1;

    if (amiport_lseek(fd, offset, SEEK_SET) < 0)
        return -1;

    result = amiport_write(fd, buf, count);

    amiport_lseek(fd, saved_pos, SEEK_SET);

    return result;
}
