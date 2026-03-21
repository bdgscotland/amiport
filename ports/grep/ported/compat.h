/*
 * compat.h — AmigaOS compatibility shims for OpenBSD grep
 *
 * amiport: Centralizes all POSIX/BSD/OpenBSD compatibility definitions
 * needed by the ported grep source files.
 */

#ifndef GREP_COMPAT_H
#define GREP_COMPAT_H

/* amiport: define NOZ to disable gzip/zlib support (no zlib on Amiga) */
#define NOZ

/* amiport: system headers that libnix provides */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <stddef.h>
#include <unistd.h>

/* amiport: POSIX shim headers */
#include <amiport/string.h>     /* strlcpy, strlcat, reallocarray */
#include <amiport/err.h>        /* err, errx, warn, warnx, warnc, strtonum */
#include <amiport/unistd.h>     /* ssize_t, off_t, u_char, STDIN_FILENO */
#include <amiport/getopt.h>     /* getopt, optarg, optind */
#include <amiport/dirent.h>     /* opendir, readdir, closedir */

/* amiport: Tier 2 regex emulation */
#include <amiport-emu/regex.h>

/* amiport: stub pledge (OpenBSD security, no-op on Amiga) */
#define pledge(p, e) (0)

/* amiport: __progname — OpenBSD provides this; we define it ourselves */
extern char *__progname;

/* amiport: PATH_MAX */
#ifndef PATH_MAX
#define PATH_MAX 256
#endif

/* amiport: regoff_t not defined in our regex emulation */
#ifndef _REGOFF_T_DECLARED
typedef int regoff_t;
#define _REGOFF_T_DECLARED
#endif

/* amiport: LLONG_MAX and ULLONG_MAX — C99, may not be in C89 limits.h */
#ifndef LLONG_MAX
#define LLONG_MAX  0x7FFFFFFFFFFFFFFFLL
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX 0xFFFFFFFFFFFFFFFFULL
#endif

/* amiport: SSIZE_MAX */
#ifndef SSIZE_MAX
#define SSIZE_MAX LONG_MAX
#endif

/* amiport: REG_STARTEND — non-standard regex extension.
 * Our Tier 2 regex doesn't support it; define as 0 so it's a no-op flag.
 * procline() in util.c handles the substring extraction manually. */
#ifndef REG_STARTEND
#define REG_STARTEND 0
#endif

/* amiport: REG_NOSPEC — treat pattern as literal (for -F mode).
 * fgrep uses fgrepcomp/fastcomp instead, so this is rarely hit.
 * Define as a unique flag value that our regex emu ignores. */
#ifndef REG_NOSPEC
#define REG_NOSPEC 0x1000
#endif

/* amiport: bool type (C99 stdbool.h) — GCC supports this even in C89 mode */
#ifndef __bool_true_false_are_defined
typedef int bool;
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#endif

/* amiport: S_ISDIR macro if not defined */
#ifndef S_ISDIR
#define S_ISDIR(m)  (((m) & 0170000) == 0040000)
#endif

/* amiport: Amiga exit codes */
#define AMIGA_RETURN_OK    0
#define AMIGA_RETURN_WARN  5
#define AMIGA_RETURN_ERROR 10
#define AMIGA_RETURN_FAIL  20

/* ====================================================================
 * sys/queue.h SLIST macros (from BSD)
 * amiport: BSD queue macros not available in libnix
 * ==================================================================== */

#define SLIST_HEAD(name, type) \
    struct name { struct type *slh_first; }

#define SLIST_ENTRY(type) \
    struct { struct type *sle_next; }

#define SLIST_INIT(head)        ((head)->slh_first = NULL)

#define SLIST_INSERT_HEAD(head, elm, field) do { \
    (elm)->field.sle_next = (head)->slh_first;   \
    (head)->slh_first = (elm);                    \
} while (0)

#define SLIST_FIRST(head)       ((head)->slh_first)
#define SLIST_NEXT(elm, field)  ((elm)->field.sle_next)

/* ====================================================================
 * getopt_long() — not in libnix, needed for grep's option parsing
 * amiport: Minimal implementation supporting required_argument,
 * optional_argument, and no_argument.
 * ==================================================================== */

#ifndef no_argument
#define no_argument        0
#define required_argument  1
#define optional_argument  2
#endif

struct option {
    const char *name;
    int         has_arg;
    int        *flag;
    int         val;
};

static int
amiport_getopt_long(int argc, char * const argv[], const char *optstring,
    const struct option *longopts, int *longindex)
{
    /* Handle -- long options */
    if (amiport_optind < argc &&
        argv[amiport_optind] != NULL &&
        argv[amiport_optind][0] == '-' &&
        argv[amiport_optind][1] == '-' &&
        argv[amiport_optind][2] != '\0') {

        const char *arg = argv[amiport_optind] + 2;
        const char *eq = strchr(arg, '=');
        size_t namelen = eq ? (size_t)(eq - arg) : strlen(arg);
        int i;

        for (i = 0; longopts[i].name != NULL; i++) {
            if (strncmp(longopts[i].name, arg, namelen) == 0 &&
                strlen(longopts[i].name) == namelen) {

                if (longindex)
                    *longindex = i;
                amiport_optind++;

                if (longopts[i].has_arg == required_argument) {
                    if (eq) {
                        amiport_optarg = (char *)(eq + 1);
                    } else if (amiport_optind < argc) {
                        amiport_optarg = argv[amiport_optind++];
                    } else {
                        return '?';
                    }
                } else if (longopts[i].has_arg == optional_argument) {
                    amiport_optarg = eq ? (char *)(eq + 1) : NULL;
                } else {
                    amiport_optarg = NULL;
                }

                if (longopts[i].flag != NULL) {
                    *longopts[i].flag = longopts[i].val;
                    return 0;
                }
                return longopts[i].val;
            }
        }

        /* Unknown long option */
        amiport_optind++;
        return '?';
    }

    /* Fall through to short option processing.
     * amiport: skip leading '+' in optstring (GNU extension for
     * POSIX argument ordering — our getopt does this by default) */
    if (optstring[0] == '+')
        optstring++;
    return amiport_getopt(argc, argv, optstring);
}

/* amiport: override getopt macros for getopt_long support */
#undef getopt
#define getopt      amiport_getopt
#define getopt_long amiport_getopt_long

/* amiport: getline() now provided by amiport/stdio_ext.h (promoted from inline) */
#include <amiport/stdio_ext.h>

/* ====================================================================
 * fts replacement — simple recursive directory walker using opendir/readdir
 * amiport: Provides the subset of fts(3) that grep uses for -R
 * ==================================================================== */

/* FTS flags (unused but defined for source compatibility) */
#define FTS_PHYSICAL  0x01
#define FTS_NOSTAT    0x02
#define FTS_NOCHDIR   0x04

/* FTSENT info values */
#define FTS_D       1   /* Directory (preorder) */
#define FTS_DP      2   /* Directory (postorder) */
#define FTS_F       3   /* Regular file */
#define FTS_DNR     4   /* Unreadable directory */
#define FTS_ERR     5   /* Error */
#define FTS_DEFAULT 6   /* Default — process as file */

typedef struct _ftsent {
    char     *fts_path;
    size_t    fts_pathlen;
    char     *fts_name;
    int       fts_info;
    int       fts_errno;
} FTSENT;

/* amiport: Simple FTS state — stack of directories to visit */
#define AMIPORT_FTS_MAX_DEPTH 32

typedef struct {
    char          **root_argv;
    int             root_idx;
    DIR            *dir_stack[AMIPORT_FTS_MAX_DEPTH];
    char            path_stack[AMIPORT_FTS_MAX_DEPTH][PATH_MAX];
    int             depth;
    FTSENT          entry;
    char            pathbuf[PATH_MAX];
    int             flags;
} FTS;

static FTS *
fts_open(char * const *argv, int options,
    int (*compar)(const FTSENT **, const FTSENT **))
{
    FTS *ftsp;
    (void)compar;

    ftsp = (FTS *)malloc(sizeof(FTS));
    if (ftsp == NULL)
        return NULL;
    memset(ftsp, 0, sizeof(FTS));
    ftsp->root_argv = (char **)argv;
    ftsp->root_idx = 0;
    ftsp->depth = -1;
    ftsp->flags = options;
    return ftsp;
}

static FTSENT *
fts_read(FTS *ftsp)
{
    struct dirent *dp;

    for (;;) {
        /* If we have an open directory, read from it */
        if (ftsp->depth >= 0) {
            DIR *d = ftsp->dir_stack[ftsp->depth];
            dp = readdir(d);
            if (dp != NULL) {
                size_t dirlen;

                /* Skip . and .. */
                if (dp->d_name[0] == '.' &&
                    (dp->d_name[1] == '\0' ||
                     (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
                    continue;

                /* amiport: build full path, handle Amiga volume: syntax */
                dirlen = strlen(ftsp->path_stack[ftsp->depth]);
                if (dirlen > 0 &&
                    ftsp->path_stack[ftsp->depth][dirlen - 1] == ':')
                    snprintf(ftsp->pathbuf, PATH_MAX, "%s%s",
                        ftsp->path_stack[ftsp->depth], dp->d_name);
                else
                    snprintf(ftsp->pathbuf, PATH_MAX, "%s/%s",
                        ftsp->path_stack[ftsp->depth], dp->d_name);

                ftsp->entry.fts_path = ftsp->pathbuf;
                ftsp->entry.fts_pathlen = strlen(ftsp->pathbuf);
                ftsp->entry.fts_name = dp->d_name;
                ftsp->entry.fts_errno = 0;

                if (dp->d_type == DT_DIR) {
                    /* Try to descend into directory */
                    if (ftsp->depth + 1 < AMIPORT_FTS_MAX_DEPTH) {
                        DIR *subdir = opendir(ftsp->pathbuf);
                        if (subdir != NULL) {
                            ftsp->depth++;
                            ftsp->dir_stack[ftsp->depth] = subdir;
                            snprintf(ftsp->path_stack[ftsp->depth],
                                PATH_MAX, "%s", ftsp->pathbuf);
                            ftsp->entry.fts_info = FTS_D;
                            return &ftsp->entry;
                        } else {
                            ftsp->entry.fts_info = FTS_DNR;
                            ftsp->entry.fts_errno = errno;
                            return &ftsp->entry;
                        }
                    }
                    /* Max depth reached — skip this directory */
                    ftsp->entry.fts_info = FTS_D;
                    return &ftsp->entry;
                } else {
                    ftsp->entry.fts_info = FTS_DEFAULT;
                    return &ftsp->entry;
                }
            }
            /* Directory exhausted, pop */
            closedir(ftsp->dir_stack[ftsp->depth]);
            ftsp->depth--;
            continue;
        }

        /* Move to next root argument */
        if (ftsp->root_argv[ftsp->root_idx] == NULL) {
            errno = 0;
            return NULL;
        }

        {
            char *root = ftsp->root_argv[ftsp->root_idx++];
            DIR *d;

            /* Check if it's a directory */
            d = opendir(root);
            if (d != NULL) {
                ftsp->depth = 0;
                ftsp->dir_stack[0] = d;
                snprintf(ftsp->path_stack[0], PATH_MAX, "%s", root);
                ftsp->entry.fts_path = root;
                ftsp->entry.fts_pathlen = strlen(root);
                ftsp->entry.fts_name = root;
                ftsp->entry.fts_info = FTS_D;
                ftsp->entry.fts_errno = 0;
                return &ftsp->entry;
            } else {
                /* Treat as a regular file */
                ftsp->entry.fts_path = root;
                ftsp->entry.fts_pathlen = strlen(root);
                ftsp->entry.fts_name = root;
                ftsp->entry.fts_info = FTS_DEFAULT;
                ftsp->entry.fts_errno = 0;
                return &ftsp->entry;
            }
        }
    }
}

static int
fts_close(FTS *ftsp)
{
    while (ftsp->depth >= 0) {
        closedir(ftsp->dir_stack[ftsp->depth]);
        ftsp->depth--;
    }
    free(ftsp);
    return 0;
}

/* ====================================================================
 * isatty — map to amiport shim
 * ==================================================================== */
/* amiport: replaced POSIX isatty() with amiport_isatty() */
#define isatty amiport_isatty

#endif /* GREP_COMPAT_H */
