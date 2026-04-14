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
#include <dos/dos.h>            /* RETURN_OK / RETURN_ERROR */

#include "amigit.h"

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
        if (amigit_is_help_flag(argv[i])) {
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

    /* Normalize the path via amigit_resolve_repo_path -- this resolves
     * "." to the absolute CWD path and rewrites "X:foo" to "X:/foo",
     * working around libgit2's AmigaOS path-handling quirks (see the
     * helper's documentation in amigit.h). */
    {
        char resolved[256];
        if (amigit_resolve_repo_path(path, resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr,
                    "amigit: init: cannot resolve path '%s'\n", path);
            return RETURN_ERROR;
        }

        /*
         * amiport: libgit2's git_fs_path_root() only recognizes
         * single-character ASCII drive prefixes ("X:"). For multi-
         * character AmigaOS volume names (e.g. "Ram Disk:foo",
         * "WORK:playground", "System 3.1:bin") git_fs_path_root
         * returns -1, which causes the mkdir walk in
         * git_futils_mkdir() to strip everything back to "./." and
         * fail with "failed to make directory './.': No such file or
         * directory". This ONLY affects init (mkdir walk); status,
         * log, add, commit, etc. use git_repository_open_ext() which
         * has a more permissive path handler and works fine with
         * multi-char volume names.
         *
         * Until libgit2 gets a proper AmigaOS path root recognizer,
         * detect this case up front and emit a helpful error. The
         * user can always run init with an explicit path.
         */
        if (path[0] == '.' && path[1] == '\0' &&
            !(resolved[0] != '\0' && resolved[1] == ':')) {
            fprintf(stderr,
                "amigit: init: cannot initialize from this working directory\n"
                "amigit: init: the AmigaOS volume name contains more than\n"
                "amigit: init: one character, which libgit2 does not recognize\n"
                "amigit: init: as a path root. Use an explicit path instead:\n"
                "amigit: init:     amigit init <path>\n"
                "amigit: init: e.g. amigit init %s\n",
                resolved);
            return RETURN_ERROR;
        }

        /* Probe for "already a repo" using the resolved path so the
         * "Reinitialized" message fires correctly on repeat calls. */
        existed_before = 0;
        {
            git_repository *probe = NULL;
            int probe_rc = git_repository_open_ext(
                &probe, resolved, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);
            if (probe_rc == 0) {
                existed_before = 1;
                git_repository_free(probe);
            }
            git_error_clear();
        }

        rc = git_repository_init(&repo, resolved, is_bare);
    }
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
