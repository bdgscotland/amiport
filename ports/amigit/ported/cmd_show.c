/*
 * cmd_show.c -- `amigit show <ref>`
 *
 * Resolves a ref to a commit and prints:
 *   commit <full-sha>
 *   Author: <name> <<email>>
 *   Date:   <unix seconds>
 *
 *   <commit message>
 *
 *   <unified diff against first parent, or empty tree for root commit>
 *
 * The date is emitted as a raw Unix seconds timestamp (no strftime) to
 * avoid locale/tzset dependencies that libnix does not fully support.
 * Users can format it themselves if needed.
 *
 * Usage:
 *   amigit show HEAD         -- show latest commit
 *   amigit show <hex>        -- show any commit by OID prefix
 *   amigit show --help       -- usage + exit 0
 *
 * Exit:
 *   0  on success
 *   10 on libgit2 failure, missing ref, or not a repository
 *
 * Reference: tests/libgit2/test_libgit2.c revparse_single_head,
 * diff_tree_to_tree_initial.
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

static int cmd_show_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit show <ref>\n\n");
    fprintf(out, "Show a commit with its diff against the first parent.\n");
    fprintf(out, "Use HEAD, a branch name, a tag, or an OID prefix.\n");
    return rc;
}

/*
 * Diff line printer: forwards every line from git_diff_print to stdout,
 * honoring the origin byte so that context lines don't get a '+' or '-'.
 * Content is not NUL-terminated per libgit2 docs, so we write with fwrite.
 */
static int show_diff_line_cb(const git_diff_delta *delta,
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

int amigit_cmd_show(int argc, char **argv)
{
    git_repository *repo = NULL;
    git_object *obj = NULL;
    git_commit *commit = NULL;
    git_commit *parent = NULL;
    git_tree *new_tree = NULL;
    git_tree *old_tree = NULL;
    git_diff *diff = NULL;
    const char *ref = NULL;
    const git_signature *author;
    const char *msg;
    char full_sha[GIT_OID_MAX_HEXSIZE + 1];
    int rc;
    int i;

    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_show_usage(RETURN_OK);
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "amigit: show: unknown option '%s'\n", argv[i]);
            return cmd_show_usage(RETURN_ERROR);
        }
        if (ref == NULL) {
            ref = argv[i];
        }
    }

    if (ref == NULL) {
        ref = "HEAD";
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: show: cannot resolve CWD\n");
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

    rc = git_revparse_single(&obj, repo, ref);
    if (rc != 0) {
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    if (git_object_type(obj) != GIT_OBJECT_COMMIT) {
        fprintf(stderr, "amigit: show: '%s' is not a commit\n", ref);
        git_object_free(obj);
        git_repository_free(repo);
        return RETURN_ERROR;
    }

    /* Steal the commit pointer from the object handle. Don't free both. */
    commit = (git_commit *)obj;
    obj = NULL;

    /* Header: commit <full hex>, author, date, blank line, message, blank */
    git_oid_tostr(full_sha, sizeof(full_sha), git_commit_id(commit));
    printf("commit %s\n", full_sha);

    author = git_commit_author(commit);
    if (author != NULL) {
        printf("Author: %s <%s>\n",
               author->name  != NULL ? author->name  : "",
               author->email != NULL ? author->email : "");
    }
    printf("Date:   %ld\n", (long)git_commit_time(commit));
    printf("\n");

    msg = git_commit_message(commit);
    if (msg != NULL) {
        printf("%s", msg);
        /* libgit2 guarantees message ends with \n, but defensively: */
        if (msg[0] == '\0' || msg[strlen(msg) - 1] != '\n') {
            printf("\n");
        }
    }
    printf("\n");

    /* Diff against first parent, or against empty tree for root commit. */
    rc = git_commit_tree(&new_tree, commit);
    if (rc != 0) {
        git_commit_free(commit);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    if (git_commit_parentcount(commit) > 0) {
        rc = git_commit_parent(&parent, commit, 0);
        if (rc != 0) {
            git_tree_free(new_tree);
            git_commit_free(commit);
            git_repository_free(repo);
            return amigit_error_exit(rc);
        }
        rc = git_commit_tree(&old_tree, parent);
        if (rc != 0) {
            git_commit_free(parent);
            git_tree_free(new_tree);
            git_commit_free(commit);
            git_repository_free(repo);
            return amigit_error_exit(rc);
        }
    }
    /* old_tree == NULL for root commit -- libgit2 treats NULL as empty. */

    rc = git_diff_tree_to_tree(&diff, repo, old_tree, new_tree, NULL);
    if (rc != 0) {
        if (old_tree  != NULL) git_tree_free(old_tree);
        if (parent    != NULL) git_commit_free(parent);
        git_tree_free(new_tree);
        git_commit_free(commit);
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    rc = git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, show_diff_line_cb, NULL);
    /* Non-zero rc from print_cb would have propagated, but our cb always
     * returns 0; this rc is the libgit2 internal status. */

    git_diff_free(diff);
    if (old_tree != NULL) git_tree_free(old_tree);
    if (parent   != NULL) git_commit_free(parent);
    git_tree_free(new_tree);
    git_commit_free(commit);
    git_repository_free(repo);

    if (rc != 0) {
        return amigit_error_exit(rc);
    }
    return RETURN_OK;
}
