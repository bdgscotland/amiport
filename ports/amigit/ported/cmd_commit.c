/*
 * cmd_commit.c -- `amigit commit -m <msg>`
 *
 * Records a commit from the current index. If the index has no staged
 * changes (matches HEAD's tree), exits 10 with "nothing to commit".
 * If HEAD is unborn (fresh repo), creates the initial commit with no
 * parent; otherwise creates a single-parent commit from HEAD.
 *
 * Author and committer default to "amigit user <amigit@localhost>" and
 * can be overridden via the standard git environment variables:
 *   GIT_AUTHOR_NAME, GIT_AUTHOR_EMAIL
 *   GIT_COMMITTER_NAME, GIT_COMMITTER_EMAIL
 *
 * Timestamp is obtained via git_signature_now() (Unix time from
 * libnix time(NULL)).
 *
 * Usage:
 *   amigit commit -m "message"   -- record commit with inline message
 *   amigit commit --help         -- usage + exit 0
 *
 * Exit:
 *   0  on success
 *   10 on missing -m, empty message, nothing to commit, or libgit2 error
 *
 * Reference: tests/libgit2/test_libgit2.c commit_create_initial,
 * commit_create_with_parent.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>         /* libnix getenv -- returns static pointer */
#include "git2.h"
#include "git2/sys/errors.h"   /* git_error_clear() */
#include <dos/dos.h>

#include "amigit.h"

static int cmd_commit_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit commit -m <message>\n\n");
    fprintf(out, "Record a commit from the current index.\n\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -m <msg>   Commit message (required)\n\n");
    fprintf(out, "Environment:\n");
    fprintf(out, "  GIT_AUTHOR_NAME, GIT_AUTHOR_EMAIL       -- author identity\n");
    fprintf(out, "  GIT_COMMITTER_NAME, GIT_COMMITTER_EMAIL -- committer identity\n");
    return rc;
}

/*
 * Resolve an identity pair (name + email) from environment variables
 * with sensible defaults. libnix getenv() returns a pointer to static
 * internal storage -- DO NOT free. If the variable is unset or empty,
 * fall back to the default.
 */
static void resolve_identity(const char *name_var,
                             const char *email_var,
                             const char **out_name,
                             const char **out_email)
{
    const char *n = getenv(name_var);
    const char *e = getenv(email_var);
    *out_name  = (n != NULL && n[0] != '\0') ? n : "amigit user";
    *out_email = (e != NULL && e[0] != '\0') ? e : "amigit@localhost";
}

int amigit_cmd_commit(int argc, char **argv)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_oid tree_oid;
    git_oid commit_oid;
    git_signature *author = NULL;
    git_signature *committer = NULL;
    git_reference *head_ref = NULL;
    git_commit *parent = NULL;
    const git_commit *parents[1];
    const char *message = NULL;
    const char *author_name;
    const char *author_email;
    const char *committer_name;
    const char *committer_email;
    int rc;
    int i;
    int unborn_head = 0;

    for (i = 2; i < argc; i++) {
        if (amigit_is_help_flag(argv[i])) {
            return cmd_commit_usage(RETURN_OK);
        }
        if (strcmp(argv[i], "-m") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr,
                        "amigit: commit: -m requires an argument\n");
                return cmd_commit_usage(RETURN_ERROR);
            }
            message = argv[++i];
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "amigit: commit: unknown option '%s'\n",
                    argv[i]);
            return cmd_commit_usage(RETURN_ERROR);
        }
        fprintf(stderr, "amigit: commit: unexpected argument '%s'\n",
                argv[i]);
        return cmd_commit_usage(RETURN_ERROR);
    }

    if (message == NULL) {
        fprintf(stderr, "amigit: commit: -m <message> is required\n");
        return cmd_commit_usage(RETURN_ERROR);
    }
    if (message[0] == '\0') {
        fprintf(stderr, "amigit: commit: empty message\n");
        return RETURN_ERROR;
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: commit: cannot resolve CWD\n");
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

    /* Refuse to create an empty initial commit. libgit2 will happily
     * write an empty tree and let us commit it, which matches
     * `git commit --allow-empty` semantics, but plain `git commit`
     * rejects an empty index. Match the stricter behavior. */
    if (git_index_entrycount(idx) == 0) {
        git_index_free(idx);
        git_repository_free(repo);
        fprintf(stderr, "amigit: commit: nothing to commit\n");
        return RETURN_ERROR;
    }

    /* Write the index to a tree so we can compare against HEAD's tree
     * below to detect "nothing to commit" for non-initial commits. */
    rc = git_index_write_tree(&tree_oid, idx);
    if (rc != 0) {
        git_index_free(idx);
        git_repository_free(repo);
        git_error_clear();
        fprintf(stderr, "amigit: commit: nothing to commit\n");
        return RETURN_ERROR;
    }

    /* Lookup HEAD to decide whether this is the initial commit or a
     * subsequent one. GIT_EUNBORNBRANCH means no commits yet. */
    rc = git_reference_lookup(&head_ref, repo, "HEAD");
    if (rc == 0) {
        /* HEAD is a symbolic ref; resolve to find the target. If the
         * target does not exist yet (unborn branch), treat as initial. */
        git_reference *resolved_ref = NULL;
        int rres = git_reference_resolve(&resolved_ref, head_ref);
        if (rres == GIT_ENOTFOUND) {
            unborn_head = 1;
            git_error_clear();
        } else if (rres == 0) {
            const git_oid *parent_oid = git_reference_target(resolved_ref);
            if (parent_oid != NULL) {
                rc = git_commit_lookup(&parent, repo, parent_oid);
                if (rc != 0) {
                    git_reference_free(resolved_ref);
                    git_reference_free(head_ref);
                    git_index_free(idx);
                    git_repository_free(repo);
                    return amigit_error_exit(rc);
                }
            } else {
                unborn_head = 1;
            }
            git_reference_free(resolved_ref);
        } else {
            git_reference_free(head_ref);
            git_index_free(idx);
            git_repository_free(repo);
            return amigit_error_exit(rres);
        }
    } else if (rc == GIT_ENOTFOUND) {
        /* HEAD ref file missing -- brand new repo. */
        unborn_head = 1;
        git_error_clear();
    } else {
        git_index_free(idx);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    /* If this is not the initial commit, refuse to commit an empty
     * delta against HEAD -- matches upstream git's "nothing to commit". */
    if (!unborn_head && parent != NULL) {
        const git_oid *parent_tree_oid;
        git_tree *parent_tree = NULL;
        int same_tree;

        rc = git_commit_tree(&parent_tree, parent);
        if (rc != 0) {
            git_commit_free(parent);
            git_reference_free(head_ref);
            git_index_free(idx);
            git_repository_free(repo);
            return amigit_error_exit(rc);
        }
        parent_tree_oid = git_tree_id(parent_tree);
        same_tree = (git_oid_cmp(&tree_oid, parent_tree_oid) == 0);
        git_tree_free(parent_tree);

        if (same_tree) {
            git_commit_free(parent);
            git_reference_free(head_ref);
            git_index_free(idx);
            git_repository_free(repo);
            fprintf(stderr, "amigit: commit: nothing to commit\n");
            return RETURN_ERROR;
        }
    }

    rc = git_tree_lookup(&tree, repo, &tree_oid);
    if (rc != 0) {
        if (parent != NULL) git_commit_free(parent);
        if (head_ref != NULL) git_reference_free(head_ref);
        git_index_free(idx);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    resolve_identity("GIT_AUTHOR_NAME", "GIT_AUTHOR_EMAIL",
                     &author_name, &author_email);
    resolve_identity("GIT_COMMITTER_NAME", "GIT_COMMITTER_EMAIL",
                     &committer_name, &committer_email);

    rc = git_signature_now(&author, author_name, author_email);
    if (rc != 0) {
        git_tree_free(tree);
        if (parent != NULL) git_commit_free(parent);
        if (head_ref != NULL) git_reference_free(head_ref);
        git_index_free(idx);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }
    rc = git_signature_now(&committer, committer_name, committer_email);
    if (rc != 0) {
        git_signature_free(author);
        git_tree_free(tree);
        if (parent != NULL) git_commit_free(parent);
        if (head_ref != NULL) git_reference_free(head_ref);
        git_index_free(idx);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    /* Create the commit. Initial commit has zero parents; subsequent
     * commits have HEAD as the sole parent. */
    if (unborn_head || parent == NULL) {
        rc = git_commit_create_v(&commit_oid, repo, "HEAD",
                                 author, committer, NULL,
                                 message, tree, 0);
    } else {
        parents[0] = parent;
        rc = git_commit_create(&commit_oid, repo, "HEAD",
                               author, committer, NULL,
                               message, tree, 1, parents);
    }

    git_signature_free(committer);
    git_signature_free(author);
    git_tree_free(tree);
    if (parent != NULL) git_commit_free(parent);
    if (head_ref != NULL) git_reference_free(head_ref);
    git_index_free(idx);
    git_repository_free(repo);

    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    {
        char short_sha[8];
        git_oid_tostr(short_sha, sizeof(short_sha), &commit_oid);
        printf("[%s] %s\n", short_sha, message);
    }

    return RETURN_OK;
}
