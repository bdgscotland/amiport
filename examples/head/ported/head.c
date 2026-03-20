/*
 * head -- print first N lines of files (AmigaOS port)
 *
 * Ported to AmigaOS 3.x by amiport.
 * Original: POSIX implementation using low-level file I/O.
 *
 * Usage: head [-n NUM] [file ...]
 */

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>                    /* amiport: added for LONG type */
#include <amiport/unistd.h>               /* amiport: replaced <unistd.h> */
#include <amiport/getopt.h>               /* amiport: replaced <getopt.h> */

/* amiport: Amiga version string */
static const char *verstag = "$VER: head 1.0 (19.03.2026)";

/* amiport: Stack cookie -- 32KB */
LONG __stack = 32768;

#define BUFSIZE 4096
#define DEFAULT_LINES 10

static int head_fd(int fd, long nlines);
static void write_str(int fd, const char *s);
static void write_stderr(const char *msg);

int main(int argc, char *argv[])
{
    long nlines = DEFAULT_LINES;
    int ch;
    int i;
    int nfiles;
    int fd;
    int ret = 0;

    /* amiport: using amiport_getopt (via macro) */
    while ((ch = getopt(argc, argv, "n:")) != -1) {
        switch (ch) {
            case 'n':
                nlines = atol(optarg);
                if (nlines <= 0) {
                    write_stderr("head: invalid line count\n");
                    return 1;
                }
                break;
            default:
                write_stderr("usage: head [-n NUM] [file ...]\n");
                return 1;
        }
    }
    argc -= optind;
    argv += optind;

    nfiles = argc;

    if (nfiles == 0) {
        /* Read from stdin (fd 0) */
        if (head_fd(0, nlines) != 0) {
            ret = 1;
        }
    } else {
        for (i = 0; i < nfiles; i++) {
            /* Print header when multiple files */
            if (nfiles > 1) {
                if (i > 0) {
                    write_str(1, "\n");
                }
                write_str(1, "==> ");
                write_str(1, argv[i]);
                write_str(1, " <==\n");
            }

            fd = amiport_open(argv[i], O_RDONLY);  /* amiport: replaced open() with amiport_open() */
            if (fd < 0) {
                write_stderr("head: ");
                write_stderr(argv[i]);
                write_stderr(": No such file or directory\n");
                ret = 1;
                continue;
            }

            if (head_fd(fd, nlines) != 0) {
                ret = 1;
            }

            amiport_close(fd);                      /* amiport: replaced close() with amiport_close() */
        }
    }

    return ret;
}

/*
 * head_fd -- read from fd and output first nlines lines to stdout.
 * Returns 0 on success, -1 on read error.
 */
static int head_fd(int fd, long nlines)
{
    char buf[BUFSIZE];
    LONG nread;                             /* amiport: replaced ssize_t with LONG */
    long lines_remaining;
    LONG j;                                 /* amiport: replaced ssize_t with LONG */

    lines_remaining = nlines;

    while (lines_remaining > 0) {
        nread = amiport_read(fd, buf, BUFSIZE);     /* amiport: replaced read() with amiport_read() */
        if (nread < 0) {
            write_stderr("head: read error\n");
            return -1;
        }
        if (nread == 0) {
            break;
        }

        /* Scan buffer for newlines */
        for (j = 0; j < nread; j++) {
            if (buf[j] == '\n') {
                lines_remaining--;
                if (lines_remaining == 0) {
                    /* Write up to and including this newline */
                    amiport_write(1, buf, j + 1);   /* amiport: replaced write() with amiport_write() */
                    return 0;
                }
            }
        }

        /* No newline boundary hit yet; write entire buffer */
        amiport_write(1, buf, nread);               /* amiport: replaced write() with amiport_write() */
    }

    return 0;
}

/*
 * write_str -- write a NUL-terminated string to a file descriptor.
 */
static void write_str(int fd, const char *s)
{
    LONG len;                               /* amiport: replaced size_t with LONG */

    len = (LONG)strlen(s);
    amiport_write(fd, s, len);              /* amiport: replaced write() with amiport_write() */
}

/*
 * write_stderr -- convenience wrapper for writing to stderr (fd 2).
 */
static void write_stderr(const char *msg)
{
    write_str(2, msg);
}
