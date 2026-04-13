/*
 * cmd_branch.c -- `amigit branch [-l|-d] [name]`
 *
 * Modes:
 *   amigit branch             -- list local branches (one per line)
 *   amigit branch -l          -- same as no args
 *   amigit branch <name>      -- create a new branch at HEAD
 *   amigit branch -d <name>   -- delete a branch (refuses current HEAD)
 *
 * Listing marks the current branch with a leading '* ' per the upstream
 * porcelain convention.
 *
 * Usage:
 *   amigit branch               -- list
 *   amigit branch -l            -- list
 *   amigit branch foo           -- create foo at HEAD
 *   amigit branch -d foo        -- delete foo
 *   amigit branch --help        -- usage + exit 0
 *
 * Exit:
 *   0  on success
 *   10 on missing ref, libgit2 error, not a repository, or delete-HEAD
 *
 * Reference: tests/libgit2/test_libgit2.c Section 10 (branches),
 * branch_create_and_lookup, branch_iterator_local.
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

static int cmd_branch_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit branch [-l] [-d <name>] [<name>]\n\n");
    fprintf(out, "List, create, or delete local branches.\n\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -l           List local branches (default)\n");
    fprintf(out, "  -d <name>    Delete the named branch\n");
    fprintf(out, "  <name>       Create a new branch at HEAD\n");
    return rc;
}

/*
 * List all local branches, prefixing the current branch with '* '.
 */
static int branch_list(git_repository *repo)
{
    git_branch_iterator *iter = NULL;
    git_reference *ref = NULL;
    git_branch_t branch_type;
    int rc;

    rc = git_branch_iterator_new(&iter, repo, GIT_BRANCH_LOCAL);
    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    while ((rc = git_branch_next(&ref, &branch_type, iter))
               != GIT_ITEROVER) {
        const char *name = NULL;
        int is_head;

        if (rc != 0) {
            git_branch_iterator_free(iter);
            return amigit_error_exit(rc);
        }

        if (git_branch_name(&name, ref) != 0 || name == NULL) {
            git_reference_free(ref);
            continue;
        }

        is_head = git_branch_is_head(ref);
        printf("%s%s\n", (is_head == 1) ? "* " : "  ", name);
        git_reference_free(ref);
        ref = NULL;
    }

    git_branch_iterator_free(iter);
    return RETURN_OK;
}

/*
 * Create a new branch named <name> at HEAD's commit.
 */
static int branch_create(git_repository *repo, const char *name)
{
    git_reference *head_ref = NULL;
    git_reference *resolved_ref = NULL;
    git_commit *head_commit = NULL;
    git_reference *new_ref = NULL;
    const git_oid *oid;
    int rc;

    rc = git_reference_lookup(&head_ref, repo, "HEAD");
    if (rc != 0) {
        return amigit_error_exit(rc);
    }
    rc = git_reference_resolve(&resolved_ref, head_ref);
    if (rc != 0) {
        git_reference_free(head_ref);
        return amigit_error_exit(rc);
    }
    oid = git_reference_target(resolved_ref);
    if (oid == NULL) {
        git_reference_free(resolved_ref);
        git_reference_free(head_ref);
        fprintf(stderr,
                "amigit: branch: HEAD has no commit to branch from\n");
        return RETURN_ERROR;
    }
    rc = git_commit_lookup(&head_commit, repo, oid);
    if (rc != 0) {
        git_reference_free(resolved_ref);
        git_reference_free(head_ref);
        return amigit_error_exit(rc);
    }

    rc = git_branch_create(&new_ref, repo, name, head_commit, 0);

    git_commit_free(head_commit);
    git_reference_free(resolved_ref);
    git_reference_free(head_ref);

    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    git_reference_free(new_ref);
    return RETURN_OK;
}

/*
 * Delete the branch <name>. Refuses to delete the branch currently
 * pointed to by HEAD.
 */
static int branch_delete_named(git_repository *repo, const char *name)
{
    git_reference *ref = NULL;
    int rc;

    rc = git_branch_lookup(&ref, repo, name, GIT_BRANCH_LOCAL);
    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    if (git_branch_is_head(ref) == 1) {
        git_reference_free(ref);
        fprintf(stderr,
                "amigit: branch: cannot delete branch '%s' "
                "checked out at HEAD\n", name);
        return RETURN_ERROR;
    }

    rc = git_branch_delete(ref);
    git_reference_free(ref);

    if (rc != 0) {
        return amigit_error_exit(rc);
    }
    return RETURN_OK;
}

int amigit_cmd_branch(int argc, char **argv)
{
    git_repository *repo = NULL;
    const char *delete_name = NULL;
    const char *create_name = NULL;
    int list_mode = 0;
    int rc;
    int i;

    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_branch_usage(RETURN_OK);
        }
        if (strcmp(argv[i], "-l") == 0 ||
            strcmp(argv[i], "--list") == 0) {
            list_mode = 1;
            continue;
        }
        if (strcmp(argv[i], "-d") == 0 ||
            strcmp(argv[i], "--delete") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr,
                        "amigit: branch: -d requires a branch name\n");
                return cmd_branch_usage(RETURN_ERROR);
            }
            delete_name = argv[++i];
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "amigit: branch: unknown option '%s'\n",
                    argv[i]);
            return cmd_branch_usage(RETURN_ERROR);
        }
        if (create_name == NULL) {
            create_name = argv[i];
            continue;
        }
        fprintf(stderr,
                "amigit: branch: unexpected argument '%s'\n", argv[i]);
        return cmd_branch_usage(RETURN_ERROR);
    }

    /* Mutually exclusive: -d + create, or create + list. */
    if (delete_name != NULL && create_name != NULL) {
        fprintf(stderr,
                "amigit: branch: -d and create are mutually exclusive\n");
        return cmd_branch_usage(RETURN_ERROR);
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: branch: cannot resolve CWD\n");
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

    if (delete_name != NULL) {
        rc = branch_delete_named(repo, delete_name);
    } else if (create_name != NULL) {
        rc = branch_create(repo, create_name);
    } else {
        (void)list_mode;        /* default is list */
        rc = branch_list(repo);
    }

    git_repository_free(repo);
    return rc;
}
