/*
 * mini-find — simplified find utility (AmigaOS port)
 *
 * Ported to AmigaOS 3.x by amiport.
 * Original: POSIX find-like directory walker.
 *
 * Recursively walks a directory tree and prints matching entries.
 * Supports -name PATTERN (simple glob) and -type f/d filtering.
 *
 * Usage: mini-find [-n PATTERN] [-t f|d] directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>                         /* amiport: Amiga types for LONG */

/* amiport: replaced <dirent.h> with amiport shim */
#include <amiport/dirent.h>
/* amiport: replaced <sys/stat.h> with amiport shim */
#include <amiport/sys/stat.h>
/* amiport: replaced <getopt.h> with amiport shim */
#include <amiport/getopt.h>

/* amiport: Amiga version string */
static const char *verstag = "$VER: mini-find 1.0 (19.03.2026)";

/* amiport: extra stack for recursive directory traversal */
LONG __stack = 65536;

/* Filter modes */
#define FILTER_ALL   0
#define FILTER_FILE  1
#define FILTER_DIR   2

/* Maximum path length */
#define MAX_PATH_LEN 1024

static char *name_pattern = NULL;
static int type_filter = FILTER_ALL;

/*
 * match_pattern — simple glob matching
 *
 * Handles:
 *   *.ext   — suffix match
 *   prefix* — prefix match
 *   *       — match everything
 *   exact   — exact match (no wildcards)
 */
static int match_pattern(const char *pattern, const char *name)
{
    size_t plen, nlen;
    const char *star;

    if (pattern == NULL) {
        return 1;
    }

    plen = strlen(pattern);
    nlen = strlen(name);

    /* Single wildcard matches everything */
    if (plen == 1 && pattern[0] == '*') {
        return 1;
    }

    /* Find the wildcard position */
    star = strchr(pattern, '*');

    if (star == NULL) {
        /* No wildcard: exact match */
        return strcmp(pattern, name) == 0;
    }

    if (star == pattern) {
        /* Leading wildcard: *.ext — suffix match */
        const char *suffix = pattern + 1;
        size_t slen = strlen(suffix);
        if (nlen < slen) {
            return 0;
        }
        return strcmp(name + nlen - slen, suffix) == 0;
    }

    if (star == pattern + plen - 1) {
        /* Trailing wildcard: prefix* — prefix match */
        size_t prefix_len = plen - 1;
        if (nlen < prefix_len) {
            return 0;
        }
        return strncmp(name, pattern, prefix_len) == 0;
    }

    /* Wildcard in the middle is not supported; treat as no match */
    return 0;
}

/*
 * walk_dir — recursively traverse a directory tree
 *
 * Prints entries matching the current filter settings.
 * Returns 0 on success, -1 on error.
 */
static int walk_dir(const char *dir)
{
    AMIPORT_DIR *dp;                            /* amiport: replaced DIR with AMIPORT_DIR */
    struct amiport_dirent *entry;               /* amiport: replaced struct dirent with struct amiport_dirent */
    struct amiport_stat st;                     /* amiport: replaced struct stat with struct amiport_stat */
    char fullpath[MAX_PATH_LEN];
    int is_dir;
    int is_file;

    dp = amiport_opendir(dir);                  /* amiport: replaced opendir() with amiport_opendir() */
    if (dp == NULL) {
        fprintf(stderr, "mini-find: cannot open '%s'\n", dir);
        return -1;
    }

    while ((entry = amiport_readdir(dp)) != NULL) {  /* amiport: replaced readdir() with amiport_readdir() */
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path */
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);

        /* Stat the entry to determine type */
        if (amiport_stat(fullpath, &st) != 0) { /* amiport: replaced stat() with amiport_stat() */
            fprintf(stderr, "mini-find: cannot stat '%s'\n", fullpath);
            continue;
        }

        is_dir = AMIPORT_S_ISDIR(st.st_mode);  /* amiport: replaced S_ISDIR() with AMIPORT_S_ISDIR() */
        is_file = AMIPORT_S_ISREG(st.st_mode);  /* amiport: replaced S_ISREG() with AMIPORT_S_ISREG() */

        /* Check filters and print if matching */
        if (type_filter == FILTER_ALL ||
            (type_filter == FILTER_FILE && is_file) ||
            (type_filter == FILTER_DIR && is_dir)) {
            if (match_pattern(name_pattern, entry->d_name)) {
                printf("%s\n", fullpath);
            }
        }

        /* Recurse into subdirectories */
        if (is_dir) {
            walk_dir(fullpath);
        }
    }

    amiport_closedir(dp);                       /* amiport: replaced closedir() with amiport_closedir() */
    return 0;
}

static void usage(void)
{
    fprintf(stderr,
        "usage: mini-find [-n PATTERN] [-t f|d] directory\n");
}

int main(int argc, char *argv[])
{
    int opt;
    const char *start_dir;

    /* Parse options: -n PATTERN, -t TYPE */
    while ((opt = amiport_getopt(argc, argv, "n:t:")) != -1) {  /* amiport: replaced getopt() with amiport_getopt() */
        switch (opt) {
            case 'n':
                name_pattern = amiport_optarg;  /* amiport: replaced optarg with amiport_optarg */
                break;
            case 't':
                if (amiport_optarg[0] == 'f') { /* amiport: replaced optarg with amiport_optarg */
                    type_filter = FILTER_FILE;
                } else if (amiport_optarg[0] == 'd') { /* amiport: replaced optarg with amiport_optarg */
                    type_filter = FILTER_DIR;
                } else {
                    fprintf(stderr,
                        "mini-find: unknown type '%s' (use f or d)\n",
                        amiport_optarg);         /* amiport: replaced optarg with amiport_optarg */
                    return 1;
                }
                break;
            default:
                usage();
                return 1;
        }
    }

    /* Remaining argument is the directory */
    if (amiport_optind >= argc) {                /* amiport: replaced optind with amiport_optind */
        usage();
        return 1;
    }

    start_dir = argv[amiport_optind];            /* amiport: replaced optind with amiport_optind */

    return walk_dir(start_dir) == 0 ? 0 : 1;
}
