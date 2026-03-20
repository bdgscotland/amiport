/*
 * mini-find — simplified find utility
 *
 * Recursively walks a directory tree and prints matching entries.
 * Supports -name PATTERN (simple glob) and -type f/d filtering.
 *
 * Usage: mini-find [-name PATTERN] [-type f|d] directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <getopt.h>

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
    DIR *dp;
    struct dirent *entry;
    struct stat st;
    char fullpath[MAX_PATH_LEN];
    int is_dir;
    int is_file;

    dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "mini-find: cannot open '%s': ", dir);
        perror("");
        return -1;
    }

    while ((entry = readdir(dp)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path */
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);

        /* Stat the entry to determine type */
        if (stat(fullpath, &st) != 0) {
            fprintf(stderr, "mini-find: cannot stat '%s': ", fullpath);
            perror("");
            continue;
        }

        is_dir = S_ISDIR(st.st_mode);
        is_file = S_ISREG(st.st_mode);

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

    closedir(dp);
    return 0;
}

static void usage(void)
{
    fprintf(stderr,
        "usage: mini-find [-name PATTERN] [-type f|d] directory\n");
}

int main(int argc, char *argv[])
{
    int opt;
    const char *start_dir;

    /* Parse options: -name and -type use Unix find-style syntax,
     * but we map them to getopt-friendly single chars:
     *   -n PATTERN  (alias for -name)
     *   -t TYPE     (alias for -type)
     */
    while ((opt = getopt(argc, argv, "n:t:")) != -1) {
        switch (opt) {
            case 'n':
                name_pattern = optarg;
                break;
            case 't':
                if (optarg[0] == 'f') {
                    type_filter = FILTER_FILE;
                } else if (optarg[0] == 'd') {
                    type_filter = FILTER_DIR;
                } else {
                    fprintf(stderr,
                        "mini-find: unknown type '%s' (use f or d)\n",
                        optarg);
                    return 1;
                }
                break;
            default:
                usage();
                return 1;
        }
    }

    /* Remaining argument is the directory */
    if (optind >= argc) {
        usage();
        return 1;
    }

    start_dir = argv[optind];

    return walk_dir(start_dir) == 0 ? 0 : 1;
}
