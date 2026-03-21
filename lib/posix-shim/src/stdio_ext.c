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
     * libnix vsnprintf does NOT support NULL destination — passing NULL
     * causes an invalid memory access crash on 68000. Use a probe buffer
     * instead. 1024 bytes covers the vast majority of format strings;
     * if the result is truncated we reformat into a correctly-sized buffer. */
    {
        char probe[1024];
        va_copy(ap_copy, ap);
        len = vsnprintf(probe, sizeof(probe), fmt, ap_copy);
        va_end(ap_copy);
    }

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
 * getdelim — read a delimited line from stream
 *
 * For newline delimiter (the common case), uses fgets() which does
 * buffered block reads internally — much faster than per-character
 * fgetc() on a 7 MHz 68000. For other delimiters, falls back to
 * fgetc() since fgets() only stops at newlines.
 */
long
amiport_getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
    size_t len;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Ensure we have at least an initial buffer */
    if (*lineptr == NULL || *n == 0) {
        *n = 512;  /* one AmigaDOS disk block */
        *lineptr = (char *)malloc(*n);
        if (*lineptr == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    if (delim == '\n') {
        /* Fast path: use fgets for newline-delimited reads */
        if (fgets(*lineptr, (int)*n, stream) == NULL)
            return -1;

        len = strlen(*lineptr);

        /* Handle lines longer than buffer: grow and continue */
        while (len > 0 && (*lineptr)[len - 1] != '\n') {
            char *newbuf;
            size_t newsize = *n * 2;

            if (feof(stream))
                break;

            newbuf = (char *)realloc(*lineptr, newsize);
            if (newbuf == NULL) {
                errno = ENOMEM;
                return -1;
            }
            *lineptr = newbuf;
            *n = newsize;

            if (fgets(*lineptr + len, (int)(*n - len), stream) == NULL)
                break;
            len += strlen(*lineptr + len);
        }
    } else {
        /* Slow path: per-character for non-newline delimiters */
        int c;
        len = 0;

        while ((c = fgetc(stream)) != EOF) {
            if (len + 2 > *n) {
                char *newbuf;
                size_t newsize = *n * 2;
                newbuf = (char *)realloc(*lineptr, newsize);
                if (newbuf == NULL) {
                    errno = ENOMEM;
                    return -1;
                }
                *lineptr = newbuf;
                *n = newsize;
            }
            (*lineptr)[len++] = (char)c;
            if (c == delim)
                break;
        }

        if (len == 0)
            return -1;

        (*lineptr)[len] = '\0';
    }

    return (long)len;
}

/*
 * getline — read a newline-delimited line from stream
 *
 * Convenience wrapper around getdelim with delimiter '\n'.
 */
long
amiport_getline(char **lineptr, size_t *n, FILE *stream)
{
    return amiport_getdelim(lineptr, n, '\n', stream);
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
