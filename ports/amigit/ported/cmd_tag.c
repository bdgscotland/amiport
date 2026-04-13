/*
 * cmd_tag.c -- `amigit tag [-l] [name]`
 *
 * Modes:
 *   amigit tag               -- list existing tags
 *   amigit tag -l            -- same as no args
 *   amigit tag <name>        -- create a lightweight tag at HEAD
 *
 * Only lightweight tags are supported in v1. Annotated tags (with
 * tagger + message) are deferred to v1.1 since they have the same
 * editor/author-identity surface as commit.
 *
 * Usage:
 *   amigit tag               -- list
 *   amigit tag -l            -- list
 *   amigit tag v0.1          -- create lightweight tag v0.1 at HEAD
 *   amigit tag --help        -- usage + exit 0
 *
 * Exit:
 *   0  on success
 *   10 on libgit2 error, not a repository, or tag name collision
 *
 * Reference: tests/libgit2/test_libgit2.c Section 10 (tags),
 * tag_create_lightweight, tag_list_names.
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

static int cmd_tag_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit tag [-l] [<name>]\n\n");
    fprintf(out, "List or create lightweight tags.\n\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -l         List tag names (default)\n");
    fprintf(out, "  <name>     Create lightweight tag <name> at HEAD\n");
    return rc;
}

static int tag_list(git_repository *repo)
{
    git_strarray names;
    size_t i;
    int rc;

    names.strings = NULL;
    names.count = 0;

    rc = git_tag_list(&names, repo);
    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    for (i = 0; i < names.count; i++) {
        if (names.strings[i] != NULL) {
            printf("%s\n", names.strings[i]);
        }
    }

    git_strarray_dispose(&names);
    return RETURN_OK;
}

static int tag_create_at_head(git_repository *repo, const char *name)
{
    git_object *target = NULL;
    git_oid tag_oid;
    int rc;

    rc = git_revparse_single(&target, repo, "HEAD");
    if (rc != 0) {
        return amigit_error_exit(rc);
    }

    rc = git_tag_create_lightweight(&tag_oid, repo, name, target, 0);
    git_object_free(target);

    if (rc != 0) {
        return amigit_error_exit(rc);
    }
    return RETURN_OK;
}

int amigit_cmd_tag(int argc, char **argv)
{
    git_repository *repo = NULL;
    const char *create_name = NULL;
    int list_mode = 0;
    int rc;
    int i;

    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_tag_usage(RETURN_OK);
        }
        if (strcmp(argv[i], "-l") == 0 ||
            strcmp(argv[i], "--list") == 0) {
            list_mode = 1;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "amigit: tag: unknown option '%s'\n",
                    argv[i]);
            return cmd_tag_usage(RETURN_ERROR);
        }
        if (create_name == NULL) {
            create_name = argv[i];
            continue;
        }
        fprintf(stderr, "amigit: tag: unexpected argument '%s'\n",
                argv[i]);
        return cmd_tag_usage(RETURN_ERROR);
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: tag: cannot resolve CWD\n");
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

    if (create_name != NULL) {
        rc = tag_create_at_head(repo, create_name);
    } else {
        (void)list_mode;        /* default is list */
        rc = tag_list(repo);
    }

    git_repository_free(repo);
    return rc;
}
