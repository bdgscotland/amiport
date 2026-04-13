/*
 * cmd_checkout.c -- `amigit checkout <ref>`
 *
 * Moves HEAD to the given ref and updates the working tree to match.
 *
 * Behavior:
 *   - If <ref> names a local branch, HEAD is set symbolically to
 *     refs/heads/<ref> (attached).
 *   - If <ref> resolves to a commit OID directly (e.g. a raw SHA or a
 *     tag), HEAD is detached and points to the commit.
 *
 * Uses git_checkout_tree with GIT_CHECKOUT_SAFE -- refuses to overwrite
 * modified working tree files. Users with local changes get an error.
 *
 * Usage:
 *   amigit checkout main         -- switch to the main branch
 *   amigit checkout v0.1         -- detach HEAD at tag v0.1
 *   amigit checkout --help       -- usage + exit 0
 *
 * Exit:
 *   0  on success
 *   10 on missing ref, libgit2 error, or not a repository
 *
 * Reference: tests/libgit2/test_libgit2.c branch/reference tests.
 */

#include <stdio.h>
#include <string.h>
#include "git2.h"
#include "git2/sys/errors.h"    /* git_error_clear() */
#include <dos/dos.h>

#include "amigit.h"

static int is_help_flag(const char *s)
{
    return strcmp(s, "--help") == 0 || strcmp(s, "-h") == 0;
}

static int cmd_checkout_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit checkout <ref>\n\n");
    fprintf(out, "Switch HEAD to a branch, tag, or commit.\n\n");
    fprintf(out, "The working tree is updated to match. Local\n");
    fprintf(out, "modifications block the switch (SAFE mode).\n");
    return rc;
}

int amigit_cmd_checkout(int argc, char **argv)
{
    git_repository *repo = NULL;
    git_object *target = NULL;
    git_reference *branch_ref = NULL;
    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    const char *ref_name = NULL;
    char refspec[256];
    int rc;
    int i;

    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_checkout_usage(RETURN_OK);
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "amigit: checkout: unknown option '%s'\n",
                    argv[i]);
            return cmd_checkout_usage(RETURN_ERROR);
        }
        if (ref_name == NULL) {
            ref_name = argv[i];
            continue;
        }
        fprintf(stderr,
                "amigit: checkout: unexpected argument '%s'\n", argv[i]);
        return cmd_checkout_usage(RETURN_ERROR);
    }

    if (ref_name == NULL) {
        fprintf(stderr, "amigit: checkout: missing ref argument\n");
        return cmd_checkout_usage(RETURN_ERROR);
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: checkout: cannot resolve CWD\n");
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

    /* Resolve the ref to a commit-ish object. git_revparse_single
     * handles branch names, tags, SHA prefixes, and HEAD. */
    rc = git_revparse_single(&target, repo, ref_name);
    if (rc != 0) {
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    /* Update the working tree to the target's tree. */
    opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    rc = git_checkout_tree(repo, target, &opts);
    if (rc != 0) {
        git_object_free(target);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    /* Decide: attached (branch) or detached (commit/tag).
     * First try a local branch lookup by name. */
    rc = git_branch_lookup(&branch_ref, repo, ref_name, GIT_BRANCH_LOCAL);
    if (rc == 0) {
        /* Attached branch: set HEAD to refs/heads/<name>. */
        int n = snprintf(refspec, sizeof(refspec),
                         "refs/heads/%s", ref_name);
        if (n < 0 || (size_t)n >= sizeof(refspec)) {
            git_reference_free(branch_ref);
            git_object_free(target);
            git_repository_free(repo);
            fprintf(stderr,
                    "amigit: checkout: branch name too long\n");
            return RETURN_ERROR;
        }
        rc = git_repository_set_head(repo, refspec);
        git_reference_free(branch_ref);
    } else {
        /* Detached HEAD at the resolved commit OID. */
        git_error_clear();
        rc = git_repository_set_head_detached(repo,
                                              git_object_id(target));
    }

    git_object_free(target);
    git_repository_free(repo);

    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    printf("Switched to '%s'\n", ref_name);
    return RETURN_OK;
}
