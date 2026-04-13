/*
 * cmd_init.c -- `amigit init [path]`
 *
 * Creates an empty git repository. If path is omitted, uses the
 * current directory. If the directory already contains a .git/,
 * libgit2 returns 0 and reinitializes -- matches upstream git.
 *
 * Usage:
 *   amigit init              -- initialize .git in CWD
 *   amigit init WORK:myrepo  -- initialize .git in WORK:myrepo (creates
 *                               the parent directory if missing)
 *   amigit init --bare path  -- create a bare repository at path
 *
 * Exit:
 *   0  on success (created or reinitialized)
 *   10 on libgit2 failure (path busy, permission, disk full, ...)
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

static int cmd_init_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit init [--bare] [path]\n\n");
    fprintf(out, "Create an empty git repository.\n\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  --bare   Create a bare repository (no working tree)\n");
    fprintf(out, "  path     Directory to initialize (default: current)\n");
    return rc;
}

int amigit_cmd_init(int argc, char **argv)
{
    git_repository *repo = NULL;
    const char *path = ".";
    unsigned is_bare = 0;
    int existed_before;
    int rc;
    int i;

    /* Simple flag parse -- no getopt for a 2-option command.
     * We accept --bare and --help anywhere before or after the path,
     * to match upstream git's tolerance. */
    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_init_usage(RETURN_OK);
        }
        if (strcmp(argv[i], "--bare") == 0) {
            is_bare = 1;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "amigit: init: unknown option '%s'\n",
                    argv[i]);
            return cmd_init_usage(RETURN_ERROR);
        }
        /* Positional -- the path. Only the first one is honored. */
        path = argv[i];
    }

    /* Detect "already a repo" before calling init, so we can print
     * the "Reinitialized" vs "Initialized" message correctly.
     * git_repository_open_ext with NO_SEARCH is the cheap check. */
    existed_before = 0;
    {
        git_repository *probe = NULL;
        int probe_rc = git_repository_open_ext(
            &probe, path, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);
        if (probe_rc == 0) {
            existed_before = 1;
            git_repository_free(probe);
        }
        /* Clear any libgit2 error from the probe -- a failed open is
         * expected when the directory is not yet a repo. */
        git_error_clear();
    }

    rc = git_repository_init(&repo, path, is_bare);
    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    if (existed_before) {
        printf("Reinitialized existing %sgit repository in %s\n",
               is_bare ? "bare " : "", path);
    } else {
        printf("Initialized empty %sgit repository in %s\n",
               is_bare ? "bare " : "", path);
    }

    git_repository_free(repo);
    return RETURN_OK;
}
