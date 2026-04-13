/*
 * cmd_diff.c -- `amigit diff [--cached]`
 *
 * Default: unified diff of index against worktree (matches `git diff`).
 * --cached: unified diff of HEAD tree against index (matches
 *           `git diff --cached` / `--staged`).
 *
 * Output is plain unified format via git_diff_print + PATCH format, no
 * color, 3 lines of context (libgit2 default).
 *
 * Usage:
 *   amigit diff              -- index vs worktree
 *   amigit diff --cached     -- HEAD tree vs index
 *   amigit diff --staged     -- synonym for --cached
 *   amigit diff --help       -- usage + exit 0
 *
 * Exit:
 *   0  on success (whether or not there are deltas)
 *   10 on libgit2 failure or not a repository
 *
 * Reference: tests/libgit2/test_libgit2.c diff_numdeltas_after_add,
 * diff_empty_repo.
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

static int cmd_diff_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit diff [--cached|--staged]\n\n");
    fprintf(out, "Show changes between index and worktree, or\n");
    fprintf(out, "between HEAD and index with --cached/--staged.\n");
    return rc;
}

static int diff_line_cb(const git_diff_delta *delta,
                        const git_diff_hunk *hunk,
                        const git_diff_line *line,
                        void *payload)
{
    (void)delta;
    (void)hunk;
    (void)payload;

    if (line->origin == GIT_DIFF_LINE_CONTEXT ||
        line->origin == GIT_DIFF_LINE_ADDITION ||
        line->origin == GIT_DIFF_LINE_DELETION) {
        fputc(line->origin, stdout);
    }
    fwrite(line->content, 1, line->content_len, stdout);
    return 0;
}

/*
 * Build a cached diff: HEAD tree vs current index.
 *
 * If HEAD is unborn (new repo, no commits), the old_tree is NULL which
 * libgit2 treats as the empty tree -- the diff shows every staged file
 * as a new addition, matching upstream git.
 */
static int diff_cached(git_repository *repo, git_diff **out)
{
    git_reference *head_ref = NULL;
    git_object *head_obj = NULL;
    git_commit *head_commit = NULL;
    git_tree *head_tree = NULL;
    git_index *idx = NULL;
    int rc;

    rc = git_repository_index(&idx, repo);
    if (rc != 0) {
        return rc;
    }

    /* Best-effort HEAD lookup. If HEAD is unborn, fall through with
     * head_tree = NULL so the diff shows all staged files as added. */
    rc = git_repository_head(&head_ref, repo);
    if (rc == 0) {
        rc = git_reference_peel(&head_obj, head_ref, GIT_OBJECT_COMMIT);
        if (rc == 0) {
            head_commit = (git_commit *)head_obj;
            rc = git_commit_tree(&head_tree, head_commit);
            git_commit_free(head_commit);
            head_obj = NULL;
        }
        git_reference_free(head_ref);
    } else {
        /* Unborn HEAD isn't an error for diff --cached. */
        git_error_clear();
        rc = 0;
    }

    if (rc != 0) {
        if (head_obj != NULL) git_object_free(head_obj);
        git_index_free(idx);
        return rc;
    }

    rc = git_diff_tree_to_index(out, repo, head_tree, idx, NULL);

    if (head_tree != NULL) git_tree_free(head_tree);
    git_index_free(idx);
    return rc;
}

int amigit_cmd_diff(int argc, char **argv)
{
    git_repository *repo = NULL;
    git_diff *diff = NULL;
    int cached = 0;
    int rc;
    int i;

    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_diff_usage(RETURN_OK);
        }
        if (strcmp(argv[i], "--cached") == 0 ||
            strcmp(argv[i], "--staged") == 0) {
            cached = 1;
            continue;
        }
        fprintf(stderr, "amigit: diff: unknown option '%s'\n", argv[i]);
        return cmd_diff_usage(RETURN_ERROR);
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: diff: cannot resolve CWD\n");
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

    if (cached) {
        rc = diff_cached(repo, &diff);
    } else {
        rc = git_diff_index_to_workdir(&diff, repo, NULL, NULL);
    }
    if (rc != 0) {
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    rc = git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, diff_line_cb, NULL);

    git_diff_free(diff);
    git_repository_free(repo);

    if (rc != 0) {
        return amigit_error_exit(rc);
    }
    return RETURN_OK;
}
