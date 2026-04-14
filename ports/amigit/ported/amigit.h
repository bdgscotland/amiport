/*
 * amigit.h -- Shared declarations for the amigit CLI
 *
 * Each subcommand lives in its own ported/cmd_<name>.c and exports one
 * function matching the amigit_cmd_fn signature below. The dispatcher
 * in amigit.c walks a table of these and calls the matching one based
 * on argv[1].
 */

#ifndef AMIGIT_H
#define AMIGIT_H

#include <stddef.h>    /* size_t for amigit_resolve_repo_path */
#include "git2.h"

/* amigit version string. Keep in sync with the $VER tag in amigit.c
 * and the VERSION variable in the Makefile. */
#define AMIGIT_VERSION "0.1-5"

/*
 * Command function signature. argc/argv are the full program argv
 * (argv[0] = "amigit", argv[1] = verb, argv[2]... = subcommand args).
 * Each command is responsible for parsing its own options via
 * amiport_getopt_long or similar.
 *
 * Returns a POSIX-style return code (0 on success, 10 on error).
 * Commands MUST NOT call exit() directly -- the dispatcher handles
 * cleanup via atexit and the return value propagates to main().
 */
typedef int (*amigit_cmd_fn)(int argc, char **argv);

/*
 * Dispatch table entry. Populated by amigit.c's dispatch_table[].
 */
typedef struct amigit_command {
    const char  *name;        /* verb to match argv[1] against */
    amigit_cmd_fn fn;          /* command implementation */
    const char  *summary;     /* one-line help string */
} amigit_command;

/*
 * amigit_error_exit -- central error handler.
 *
 * Reads git_error_last(), prints "amigit: <message>" to stderr, and
 * returns RETURN_ERROR (10). Never calls exit() directly; caller is
 * expected to propagate the return code up to main().
 *
 * If libgit2_rc is 0, returns 0 without printing.
 * If libgit2_rc is negative, prints the libgit2 error message.
 * If libgit2_rc is positive, returns it verbatim (shouldn't happen
 * for libgit2 APIs that use negative error codes, but defensively
 * handled).
 */
int amigit_error_exit(int libgit2_rc);

/*
 * amigit_resolve_repo_path -- turn a caller-supplied repo path into
 * an absolute form libgit2 can open or initialize.
 *
 * Two transformations are applied:
 *
 *   1. Input "." (or NULL) is replaced with the current directory's
 *      absolute path via NameFromLock(pr_CurrentDir). libgit2's
 *      p_realpath on AmigaOS cannot handle "." -- Lock(".") fails.
 *
 *   2. AmigaDOS volume-rooted paths of the form "X:foo" (missing the
 *      slash after the colon) are rewritten to "X:/foo" so that
 *      libgit2's git_fs_path_root() recognizes them as rooted.
 *      Without this, libgit2 falls into a relative-path code path
 *      that produces broken mkdir targets like "./.". AmigaDOS
 *      accepts "X:/foo" as a synonym for "X:foo".
 *
 * out      output buffer (must be at least 256 bytes for safety).
 * outsize  size of out in bytes.
 *
 * Returns 0 on success, RETURN_ERROR on failure (out buffer too
 * small or CWD resolution failed). Prints no error message -- the
 * caller decides whether to log and exit.
 */
int amigit_resolve_repo_path(const char *in, char *out, size_t outsize);

/*
 * amigit_usage -- print the top-level usage message.
 *
 * If cmd_name is NULL, prints the top-level usage (list of commands).
 * Otherwise prints a one-line per-command usage hint.
 *
 * Output stream depends on rc: on RETURN_OK (explicit --help), prints
 * to stdout; otherwise prints to stderr (error path). Returns rc
 * unchanged for easy chaining:
 *
 *   return amigit_usage(NULL, RETURN_OK);      // stdout, exit 0
 *   return amigit_usage(NULL, RETURN_ERROR);   // stderr, exit 10
 */
int amigit_usage(const char *cmd_name, int rc);

/*
 * amigit_is_help_flag -- return non-zero if `s` is "--help" or "-h".
 *
 * Shared helper used by every cmd_*.c flag-parse loop. Previously
 * each command file defined its own static copy; this consolidation
 * removes ~10 duplicates and ensures one source of truth for what
 * counts as a help flag.
 */
int amigit_is_help_flag(const char *s);

/* Command implementations -- one per ported/cmd_<name>.c */
int amigit_cmd_version(int argc, char **argv);
int amigit_cmd_init(int argc, char **argv);
int amigit_cmd_status(int argc, char **argv);
int amigit_cmd_log(int argc, char **argv);
int amigit_cmd_show(int argc, char **argv);
int amigit_cmd_diff(int argc, char **argv);
int amigit_cmd_add(int argc, char **argv);
int amigit_cmd_commit(int argc, char **argv);
int amigit_cmd_checkout(int argc, char **argv);
int amigit_cmd_branch(int argc, char **argv);
int amigit_cmd_tag(int argc, char **argv);

#endif /* AMIGIT_H */
