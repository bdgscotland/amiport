/*
 * cmd_status.c -- `amigit status [-s|--short]`
 *
 * Reports staged, unstaged, and untracked file status for the current
 * repository in git porcelain v1 format:
 *
 *   XY <path>
 *
 * Where X is the index status and Y is the worktree status:
 *   M = modified, A = added, D = deleted, ?? = untracked
 *
 * Clean files are omitted (default porcelain behavior).
 *
 * Usage:
 *   amigit status          -- list changes
 *   amigit status -s       -- short format (already porcelain, no-op)
 *   amigit status --short  -- same
 *   amigit status --help   -- usage + exit 0
 *
 * Exit:
 *   0  on success (even if there are changes to report)
 *   10 on libgit2 failure or not a repository
 *
 * Reference: tests/libgit2/test_libgit2.c status_new_file_untracked,
 * status_clean_after_commit.
 */

#include <stdio.h>
#include <string.h>
#include "git2.h"
#include "git2/sys/errors.h"   /* git_error_clear() */
#include <dos/dos.h>

#include "amigit.h"

static int is_help_flag(const char *s)
{
    return strcmp(s, "--help") == 0 || strcmp(s, "-h") == 0;
}

static int cmd_status_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit status [-s|--short]\n\n");
    fprintf(out, "Show worktree, index, and untracked file status.\n\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -s, --short  Short (porcelain v1) format (default)\n");
    return rc;
}

/*
 * Status callback: one invocation per changed file. Prints the two-
 * character index/worktree code + path in porcelain v1 form.
 *
 * Per libgit2 docs, flags may combine INDEX_* and WT_* bits when a file
 * has been both staged and re-modified. Render the INDEX bit as X and
 * the WT bit as Y.
 */
static int status_print_cb(const char *path,
                           unsigned int flags,
                           void *payload)
{
    char x = ' ';
    char y = ' ';

    (void)payload;

    if (flags & GIT_STATUS_INDEX_NEW)        x = 'A';
    else if (flags & GIT_STATUS_INDEX_MODIFIED)   x = 'M';
    else if (flags & GIT_STATUS_INDEX_DELETED)    x = 'D';
    else if (flags & GIT_STATUS_INDEX_RENAMED)    x = 'R';
    else if (flags & GIT_STATUS_INDEX_TYPECHANGE) x = 'T';

    if (flags & GIT_STATUS_WT_NEW)        y = '?';
    else if (flags & GIT_STATUS_WT_MODIFIED)   y = 'M';
    else if (flags & GIT_STATUS_WT_DELETED)    y = 'D';
    else if (flags & GIT_STATUS_WT_RENAMED)    y = 'R';
    else if (flags & GIT_STATUS_WT_TYPECHANGE) y = 'T';

    /* Untracked files render as "?? <path>" (both chars are '?'). */
    if ((flags & GIT_STATUS_WT_NEW) && x == ' ') {
        x = '?';
    }

    printf("%c%c %s\n", x, y, path);
    return 0;
}

int amigit_cmd_status(int argc, char **argv)
{
    git_repository *repo = NULL;
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    int rc;
    int i;

    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_status_usage(RETURN_OK);
        }
        if (strcmp(argv[i], "-s") == 0 ||
            strcmp(argv[i], "--short") == 0) {
            /* Already porcelain; accept flag for CLI compat. */
            continue;
        }
        fprintf(stderr, "amigit: status: unknown option '%s'\n", argv[i]);
        return cmd_status_usage(RETURN_ERROR);
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: status: cannot resolve CWD\n");
            return RETURN_ERROR;
        }
        rc = git_repository_open_ext(&repo, resolved,
                                     GIT_REPOSITORY_OPEN_NO_SEARCH,
                                     NULL);
    }
    if (rc != 0) {
        git_error_clear();
        fprintf(stderr, "fatal: not a git repository\n");
        return RETURN_ERROR;
    }

    opts.show  = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                 GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    rc = git_status_foreach_ext(repo, &opts, status_print_cb, NULL);
    if (rc != 0) {
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    git_repository_free(repo);
    return RETURN_OK;
}
