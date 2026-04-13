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

#include "git2.h"

/* amigit version string. Keep in sync with the $VER tag in amigit.c
 * and the VERSION variable in the Makefile. */
#define AMIGIT_VERSION "0.1"

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
 * amigit_usage -- print usage message to stderr for a command.
 *
 * If cmd_name is NULL, prints the top-level usage (list of commands).
 * Otherwise prints the usage for the named command.
 *
 * Always returns 10 (RETURN_ERROR), so callers can:
 *   return amigit_usage("init");
 */
int amigit_usage(const char *cmd_name);

/* Command implementations -- one per ported/cmd_<name>.c */
int amigit_cmd_version(int argc, char **argv);
int amigit_cmd_init(int argc, char **argv);

#endif /* AMIGIT_H */
