/*
 * cmd_log.c -- `amigit log [-n N] [--oneline]`
 *
 * Walks commit history from HEAD and prints one line per commit:
 *
 *   <7-char SHA> <commit summary>
 *
 * Default walk order is topological, matching `git log` v1 style.
 *
 * Usage:
 *   amigit log              -- walk HEAD, one line per commit
 *   amigit log -n 5         -- stop after 5 commits
 *   amigit log --oneline    -- synonym for default (already oneline)
 *   amigit log --help       -- usage + exit 0
 *
 * Exit:
 *   0  on success
 *   10 on libgit2 failure, not a repository, or bad argument
 *
 * Reference: tests/libgit2/test_libgit2.c revwalk_push_head_and_walk,
 * stress_10_commits_revwalk.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "git2.h"
#include "git2/sys/errors.h"   /* git_error_clear() */
#include <dos/dos.h>

#include "amigit.h"

static int is_help_flag(const char *s)
{
    return strcmp(s, "--help") == 0 || strcmp(s, "-h") == 0;
}

static int cmd_log_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit log [-n N] [--oneline]\n\n");
    fprintf(out, "Show commit history from HEAD.\n\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -n N         Limit output to N commits\n");
    fprintf(out, "  --oneline    One-line-per-commit format (default)\n");
    return rc;
}

int amigit_cmd_log(int argc, char **argv)
{
    git_repository *repo = NULL;
    git_revwalk *walk = NULL;
    git_commit *commit = NULL;
    git_oid oid;
    long max_count = -1;        /* -1 = no limit */
    long count = 0;
    char short_sha[8];
    int rc;
    int i;

    for (i = 2; i < argc; i++) {
        if (is_help_flag(argv[i])) {
            return cmd_log_usage(RETURN_OK);
        }
        if (strcmp(argv[i], "--oneline") == 0) {
            continue;   /* default format */
        }
        if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "amigit: log: -n requires an argument\n");
                return cmd_log_usage(RETURN_ERROR);
            }
            max_count = strtol(argv[++i], NULL, 10);
            if (max_count < 0) {
                fprintf(stderr, "amigit: log: bad -n value '%s'\n",
                        argv[i]);
                return RETURN_ERROR;
            }
            continue;
        }
        fprintf(stderr, "amigit: log: unknown option '%s'\n", argv[i]);
        return cmd_log_usage(RETURN_ERROR);
    }

    {
        char resolved[256];
        if (amigit_resolve_repo_path(".", resolved, sizeof(resolved))
                != RETURN_OK) {
            fprintf(stderr, "amigit: log: cannot resolve CWD\n");
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

    rc = git_revwalk_new(&walk, repo);
    if (rc != 0) {
        git_repository_free(repo);
        return amigit_error_exit(rc);
    }

    /* Topological order matches `git log`'s default. */
    git_revwalk_sorting(walk, GIT_SORT_TOPOLOGICAL);

    rc = git_revwalk_push_head(walk);
    if (rc != 0) {
        /* Unborn HEAD (fresh repo, no commits) is GIT_EUNBORNBRANCH.
         * Not an error for log -- just no output. */
        git_error_clear();
        git_revwalk_free(walk);
        git_repository_free(repo);
        return RETURN_OK;
    }

    while (git_revwalk_next(&oid, walk) != GIT_ITEROVER) {
        const char *summary;

        if (max_count >= 0 && count >= max_count) {
            break;
        }

        rc = git_commit_lookup(&commit, repo, &oid);
        if (rc != 0) {
            git_revwalk_free(walk);
            git_repository_free(repo);
            return amigit_error_exit(rc);
        }

        /* Render a 7-char abbreviated SHA -- git's default. */
        git_oid_tostr(short_sha, sizeof(short_sha), &oid);

        summary = git_commit_summary(commit);
        printf("%s %s\n", short_sha, summary != NULL ? summary : "");

        git_commit_free(commit);
        commit = NULL;
        count++;
    }

    git_revwalk_free(walk);
    git_repository_free(repo);
    return RETURN_OK;
}
