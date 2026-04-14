/*
 * cmd_add.c -- `amigit add <path>...`
 *
 * Stages one or more files into the index. Each path must be relative
 * to the repository root; absolute paths and paths outside the working
 * tree are rejected by libgit2 with GIT_ENOTFOUND.
 *
 * Usage:
 *   amigit add hello.txt           -- stage one file
 *   amigit add a.c b.c c.c         -- stage multiple files
 *   amigit add --help              -- usage + exit 0
 *
 * Exit:
 *   0  on success (all paths staged, index written)
 *   10 on missing path, libgit2 failure, or not a repository
 *
 * Reference: tests/libgit2/test_libgit2.c index_add_and_write_tree,
 * Section 7 (tree/index).
 */

#include <stdio.h>
#include <string.h>
#include "git2.h"
#include "git2/sys/errors.h"    /* git_error_clear() */
#include <dos/dos.h>

#include "amigit.h"

static int cmd_add_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit add <path>...\n\n");
    fprintf(out, "Stage one or more files into the index.\n\n");
    fprintf(out, "Paths are relative to the repository root.\n");
    return rc;
}

int amigit_cmd_add(int argc, char **argv)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    int first_path = -1;
    int rc;
    int i;

    /* First pass: scan for --help and find the first positional. */
    for (i = 2; i < argc; i++) {
        if (amigit_is_help_flag(argv[i])) {
            return cmd_add_usage(RETURN_OK);
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "amigit: add: unknown option '%s'\n", argv[i]);
            return cmd_add_usage(RETURN_ERROR);
        }
        if (first_path < 0) {
            first_path = i;
        }
    }

    if (first_path < 0) {
        fprintf(stderr, "amigit: add: missing path argument\n");
        return cmd_add_usage(RETURN_ERROR);
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: add: cannot resolve CWD\n");
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

    rc = git_repository_index(&idx, repo);
    if (rc != 0) {
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    /* Stage each path. Abort on the first failure so the index state
     * matches what the user sees in the error message. */
    for (i = first_path; i < argc; i++) {
        rc = git_index_add_bypath(idx, argv[i]);
        if (rc != 0) {
            git_index_free(idx);
            git_repository_free(repo);
            return amigit_error_exit(rc);
        }
    }

    rc = git_index_write(idx);
    if (rc != 0) {
        git_index_free(idx);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    git_index_free(idx);
    git_repository_free(repo);
    return RETURN_OK;
}
