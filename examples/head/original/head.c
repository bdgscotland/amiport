/*
 * head -- print first N lines of files
 *
 * A POSIX implementation using low-level file I/O (open/read/close/write).
 * Uses getopt() for option parsing.
 *
 * Usage: head [-n NUM] [file ...]
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>

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

            fd = open(argv[i], O_RDONLY);
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

            close(fd);
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
    ssize_t nread;
    long lines_remaining;
    ssize_t j;

    lines_remaining = nlines;

    while (lines_remaining > 0) {
        nread = read(fd, buf, BUFSIZE);
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
                    write(1, buf, (size_t)(j + 1));
                    return 0;
                }
            }
        }

        /* No newline boundary hit yet; write entire buffer */
        write(1, buf, (size_t)nread);
    }

    return 0;
}

/*
 * write_str -- write a NUL-terminated string to a file descriptor.
 */
static void write_str(int fd, const char *s)
{
    size_t len;

    len = strlen(s);
    write(fd, s, len);
}

/*
 * write_stderr -- convenience wrapper for writing to stderr (fd 2).
 */
static void write_stderr(const char *msg)
{
    write_str(2, msg);
}
