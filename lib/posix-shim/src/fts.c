/*
 * fts.c -- File tree traversal for AmigaOS
 *
 * Recursive directory walk using amiport_opendir/readdir/closedir
 * and amiport_stat. Based on the proven grep 1.68 port FTS
 * replacement, extended with full FTSENT fields.
 *
 * amiport: Uses opendir() probe for directory detection since
 * AmigaOS d_type is always DT_UNKNOWN (known-pitfalls).
 * Handles Amiga volume: path syntax (VOLUME:path vs path/file).
 */

#include <amiport/fts.h>
#include <amiport/dirent.h>
#include <amiport/signal.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * build_path -- construct full path handling Amiga volume: syntax
 *
 * AmigaDOS paths use ':' after volume names and '/' as separator.
 * "SYS:C" not "SYS:/C". "Work:Projects/foo" not "Work:Projects//foo".
 */
static void
build_path(char *dst, size_t dstsize, const char *dir, const char *name)
{
    size_t dirlen;

    dirlen = strlen(dir);
    if (dirlen > 0 && dir[dirlen - 1] == ':') {
        /* Volume root: SYS: + name -> SYS:name */
        snprintf(dst, dstsize, "%s%s", dir, name);
    } else if (dirlen > 0 && dir[dirlen - 1] == '/') {
        /* Already has separator */
        snprintf(dst, dstsize, "%s%s", dir, name);
    } else {
        snprintf(dst, dstsize, "%s/%s", dir, name);
    }
}

/*
 * fill_entry -- populate FTSENT fields from path and stat
 */
static void
fill_entry(AMIPORT_FTSENT *e, char *path, char *name,
           struct amiport_stat *sb, int info, int level)
{
    e->fts_path = path;
    e->fts_pathlen = strlen(path);
    e->fts_name = name;
    e->fts_namelen = (short)strlen(name);
    e->fts_info = (short)info;
    e->fts_errno = 0;
    e->fts_level = (short)level;
    e->fts_accpath = path;
    e->fts_statp = sb;
    e->fts_link = NULL;
}

/* --- Public API --- */

AMIPORT_FTS *
amiport_fts_open(char * const *argv, int options,
    int (*compar)(const AMIPORT_FTSENT **, const AMIPORT_FTSENT **))
{
    AMIPORT_FTS *ftsp;

    if (argv == NULL || argv[0] == NULL) {
        errno = EINVAL;
        return NULL;
    }

    ftsp = (AMIPORT_FTS *)malloc(sizeof(AMIPORT_FTS));
    if (ftsp == NULL)
        return NULL;
    memset(ftsp, 0, sizeof(AMIPORT_FTS));

    ftsp->root_argv = (char **)argv;
    ftsp->root_idx = 0;
    ftsp->depth = -1;
    ftsp->flags = options;
    ftsp->compar = compar;
    ftsp->skip = 0;
    ftsp->children = NULL;
    ftsp->children_valid = 0;

    return ftsp;
}

AMIPORT_FTSENT *
amiport_fts_read(AMIPORT_FTS *ftsp)
{
    struct amiport_dirent *dp;
    DIR *d;
    DIR *probe;
    int info;

    if (ftsp == NULL) {
        errno = EINVAL;
        return NULL;
    }

    /* Invalidate children list on each read */
    ftsp->children_valid = 0;

    for (;;) {
        /* amiport: Ctrl-C check -- AmigaOS has no SIGINT delivery */
        if (amiport_check_break()) {
            errno = 0;
            return NULL;
        }

        /* If skip was set via fts_set, pop current directory */
        if (ftsp->skip && ftsp->depth >= 0) {
            closedir((DIR *)ftsp->dir_stack[ftsp->depth]);
            ftsp->dir_stack[ftsp->depth] = NULL;
            ftsp->depth--;
            ftsp->skip = 0;

            /* Return FTS_DP for the skipped directory's parent level */
            if (ftsp->depth >= 0) {
                fill_entry(&ftsp->entry,
                    ftsp->path_stack[ftsp->depth + 1],
                    ftsp->path_stack[ftsp->depth + 1],
                    &ftsp->statbuf, AMIPORT_FTS_DP,
                    ftsp->depth + 1);
                ftsp->entry.fts_parent = (ftsp->depth >= 0)
                    ? &ftsp->parent_entries[ftsp->depth] : NULL;
                return &ftsp->entry;
            }
            continue;
        }

        /* Read from current open directory */
        if (ftsp->depth >= 0) {
            d = (DIR *)ftsp->dir_stack[ftsp->depth];
            dp = readdir(d);

            if (dp != NULL) {
                /* Skip . and .. unless FTS_SEEDOT */
                if (!(ftsp->flags & AMIPORT_FTS_SEEDOT)) {
                    if (dp->d_name[0] == '.' &&
                        (dp->d_name[1] == '\0' ||
                         (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
                        continue;
                }

                /* Build full path */
                build_path(ftsp->pathbuf, sizeof(ftsp->pathbuf),
                    ftsp->path_stack[ftsp->depth], dp->d_name);

                /* Stat the entry */
                if (amiport_stat(ftsp->pathbuf, &ftsp->statbuf) == 0) {
                    if (AMIPORT_S_ISDIR(ftsp->statbuf.st_mode)) {
                        /* Try to open as directory */
                        probe = opendir(ftsp->pathbuf);
                        if (probe != NULL) {
                            if (ftsp->depth + 1 < AMIPORT_FTS_MAX_DEPTH) {
                                ftsp->depth++;
                                ftsp->dir_stack[ftsp->depth] = probe;
                                snprintf(ftsp->path_stack[ftsp->depth],
                                    PATH_MAX, "%s", ftsp->pathbuf);

                                info = AMIPORT_FTS_D;
                                fill_entry(&ftsp->entry, ftsp->pathbuf,
                                    dp->d_name, &ftsp->statbuf, info,
                                    ftsp->depth);

                                /* Save parent entry for this level */
                                ftsp->parent_entries[ftsp->depth] = ftsp->entry;
                                ftsp->entry.fts_parent = (ftsp->depth > 0)
                                    ? &ftsp->parent_entries[ftsp->depth - 1]
                                    : NULL;
                                return &ftsp->entry;
                            }
                            closedir(probe);
                            /* Max depth -- treat as unreadable dir */
                            info = AMIPORT_FTS_DNR;
                        } else {
                            info = AMIPORT_FTS_DNR;
                            ftsp->entry.fts_errno = errno;
                        }
                    } else {
                        info = AMIPORT_FTS_F;
                    }
                } else {
                    /* Stat failed */
                    info = AMIPORT_FTS_NS;
                    ftsp->entry.fts_errno = errno;
                }

                fill_entry(&ftsp->entry, ftsp->pathbuf,
                    dp->d_name, &ftsp->statbuf, info, ftsp->depth + 1);
                ftsp->entry.fts_parent = (ftsp->depth >= 0)
                    ? &ftsp->parent_entries[ftsp->depth] : NULL;
                return &ftsp->entry;
            }

            /* Directory exhausted -- return FTS_DP (postorder) and pop */
            fill_entry(&ftsp->entry, ftsp->path_stack[ftsp->depth],
                ftsp->path_stack[ftsp->depth], &ftsp->statbuf,
                AMIPORT_FTS_DP, ftsp->depth);
            ftsp->entry.fts_parent = (ftsp->depth > 0)
                ? &ftsp->parent_entries[ftsp->depth - 1] : NULL;

            closedir((DIR *)ftsp->dir_stack[ftsp->depth]);
            ftsp->dir_stack[ftsp->depth] = NULL;
            ftsp->depth--;

            return &ftsp->entry;
        }

        /* Move to next root argument */
        if (ftsp->root_argv[ftsp->root_idx] == NULL) {
            errno = 0;
            return NULL;
        }

        {
            char *root = ftsp->root_argv[ftsp->root_idx++];

            /* Stat the root entry */
            if (amiport_stat(root, &ftsp->statbuf) != 0) {
                fill_entry(&ftsp->entry, root, root, &ftsp->statbuf,
                    AMIPORT_FTS_NS, 0);
                ftsp->entry.fts_errno = errno;
                ftsp->entry.fts_parent = NULL;
                return &ftsp->entry;
            }

            if (AMIPORT_S_ISDIR(ftsp->statbuf.st_mode)) {
                d = opendir(root);
                if (d != NULL) {
                    ftsp->depth = 0;
                    ftsp->dir_stack[0] = d;
                    snprintf(ftsp->path_stack[0], PATH_MAX, "%s", root);

                    fill_entry(&ftsp->entry, root, root, &ftsp->statbuf,
                        AMIPORT_FTS_D, 0);
                    ftsp->parent_entries[0] = ftsp->entry;
                    ftsp->entry.fts_parent = NULL;
                    return &ftsp->entry;
                } else {
                    fill_entry(&ftsp->entry, root, root, &ftsp->statbuf,
                        AMIPORT_FTS_DNR, 0);
                    ftsp->entry.fts_errno = errno;
                    ftsp->entry.fts_parent = NULL;
                    return &ftsp->entry;
                }
            } else {
                fill_entry(&ftsp->entry, root, root, &ftsp->statbuf,
                    AMIPORT_FTS_F, 0);
                ftsp->entry.fts_parent = NULL;
                return &ftsp->entry;
            }
        }
    }
}

int
amiport_fts_close(AMIPORT_FTS *ftsp)
{
    AMIPORT_FTSENT *child;
    AMIPORT_FTSENT *next;

    if (ftsp == NULL)
        return -1;

    /* Close all open directory handles */
    while (ftsp->depth >= 0) {
        if (ftsp->dir_stack[ftsp->depth] != NULL)
            closedir((DIR *)ftsp->dir_stack[ftsp->depth]);
        ftsp->depth--;
    }

    /* Free children list if any */
    child = ftsp->children;
    while (child != NULL) {
        next = child->fts_link;
        free(child->fts_path);
        free(child);
        child = next;
    }

    free(ftsp);
    return 0;
}

int
amiport_fts_set(AMIPORT_FTS *ftsp, AMIPORT_FTSENT *f, int instr)
{
    (void)f;

    if (ftsp == NULL)
        return -1;

    switch (instr) {
    case AMIPORT_FTS_SKIP:
        ftsp->skip = 1;
        return 0;
    case AMIPORT_FTS_FOLLOW:
        /* amiport: No symlinks on classic AmigaOS -- no-op */
        return 0;
    case AMIPORT_FTS_AGAIN:
        /* amiport: Not implemented -- would need to rewind directory */
        return 0;
    default:
        errno = EINVAL;
        return -1;
    }
}

/*
 * fts_children -- return linked list of children of current directory
 *
 * amiport: Reads all entries in the current directory and returns
 * them as a linked list via fts_link. The list is invalidated on
 * the next fts_read() call.
 */
AMIPORT_FTSENT *
amiport_fts_children(AMIPORT_FTS *ftsp, int options)
{
    DIR *d;
    struct amiport_dirent *dp;
    AMIPORT_FTSENT *head;
    AMIPORT_FTSENT *tail;
    AMIPORT_FTSENT *node;
    struct amiport_stat sb;
    char pathbuf[PATH_MAX * 2];
    int info;

    (void)options;

    if (ftsp == NULL || ftsp->depth < 0) {
        return NULL;
    }

    /* Free previous children list */
    {
        AMIPORT_FTSENT *child = ftsp->children;
        AMIPORT_FTSENT *next;
        while (child != NULL) {
            next = child->fts_link;
            free(child->fts_path);
            free(child);
            child = next;
        }
        ftsp->children = NULL;
    }

    /* Re-open the current directory to read children
     * (the existing handle is being consumed by fts_read) */
    d = opendir(ftsp->path_stack[ftsp->depth]);
    if (d == NULL)
        return NULL;

    head = NULL;
    tail = NULL;

    while ((dp = readdir(d)) != NULL) {
        /* Skip . and .. */
        if (dp->d_name[0] == '.' &&
            (dp->d_name[1] == '\0' ||
             (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
            continue;

        build_path(pathbuf, sizeof(pathbuf),
            ftsp->path_stack[ftsp->depth], dp->d_name);

        node = (AMIPORT_FTSENT *)malloc(sizeof(AMIPORT_FTSENT));
        if (node == NULL)
            break;
        memset(node, 0, sizeof(AMIPORT_FTSENT));

        node->fts_path = (char *)malloc(strlen(pathbuf) + 1);
        if (node->fts_path == NULL) {
            free(node);
            break;
        }
        strcpy(node->fts_path, pathbuf);

        if (amiport_stat(pathbuf, &sb) == 0) {
            info = AMIPORT_S_ISDIR(sb.st_mode)
                ? AMIPORT_FTS_D : AMIPORT_FTS_F;
        } else {
            info = AMIPORT_FTS_NS;
        }

        node->fts_pathlen = strlen(pathbuf);
        node->fts_name = node->fts_path + (strlen(pathbuf) - strlen(dp->d_name));
        node->fts_namelen = (short)strlen(dp->d_name);
        node->fts_info = (short)info;
        node->fts_level = (short)(ftsp->depth + 1);
        node->fts_accpath = node->fts_path;
        node->fts_link = NULL;

        if (tail != NULL) {
            tail->fts_link = node;
        } else {
            head = node;
        }
        tail = node;
    }

    closedir(d);
    ftsp->children = head;
    ftsp->children_valid = 1;
    return head;
}
