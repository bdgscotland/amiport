/*
 * amigit.c -- Main dispatcher for the amigit CLI
 *
 * amigit is an amiport-native local-only git client built on libgit2.
 * It takes a verb as argv[1], looks up the matching command in
 * dispatch_table[], and delegates. No global state other than the
 * libgit2 init refcount.
 *
 * Lifecycle:
 *   1. Suppress AmigaDOS volume requesters (pr_WindowPtr = -1)
 *   2. git_libgit2_init()
 *   3. Dispatch to the command handler
 *   4. atexit(shutdown_libgit2) runs git_libgit2_shutdown()
 *   5. pr_WindowPtr restored at exit (informally -- process dies)
 *
 * See PDR-010a for the v1 scope and PDR-010 for the design rationale.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/dos.h>    /* RETURN_OK, RETURN_ERROR, RETURN_FAIL */

#include "amigit.h"

/* Real AmigaOS honors this; vamos uses -s from the command line.
 * 256 KB matches lib/libgit2 tree/pack walk worst case. */
long __stack = 262144;

/* $VER tag for `version` command. Date is the build day. */
static const char *verstag = "$VER: amigit 0.1 (13.04.2026)";

/* ========================================================================
 * Dispatch table -- single source of truth for v1 command set
 * ========================================================================
 * Phase 3a: version + init. Phase 3b will add status, log, show, diff.
 * Phase 3c will add add, commit, checkout, branch, tag. Each command
 * lives in its own ported/cmd_<name>.c.
 */

static const amigit_command dispatch_table[] = {
    { "version", amigit_cmd_version,
      "Print amigit and libgit2 version strings" },
    { "init",    amigit_cmd_init,
      "Create an empty git repository" },
    { NULL,      NULL,                NULL }
};

/* ========================================================================
 * amigit_usage -- central help output
 * ======================================================================== */

int amigit_usage(const char *cmd_name)
{
    const amigit_command *cmd;

    if (cmd_name == NULL) {
        fprintf(stderr, "usage: amigit <command> [args]\n\n");
        fprintf(stderr, "amiport-native git client built on libgit2.\n");
        fprintf(stderr, "Local repositories only; no network commands.\n\n");
        fprintf(stderr, "Commands:\n");
        for (cmd = dispatch_table; cmd->name != NULL; cmd++) {
            fprintf(stderr, "  %-10s  %s\n", cmd->name, cmd->summary);
        }
        fprintf(stderr, "\nSee: amigit <command> --help for per-command usage.\n");
        return RETURN_ERROR;
    }

    /* Per-command usage: each command handles its own --help/-h by
     * printing its usage and returning RETURN_OK. This branch is only
     * reached if the dispatcher itself wants to emit a usage hint for
     * a known but misused command (no current callers). */
    fprintf(stderr, "usage: amigit %s ...\n", cmd_name);
    fprintf(stderr, "Run 'amigit %s --help' for details.\n", cmd_name);
    return RETURN_ERROR;
}

/* ========================================================================
 * amigit_error_exit -- central libgit2 error mapper
 * ======================================================================== */

int amigit_error_exit(int libgit2_rc)
{
    const git_error *e;

    if (libgit2_rc == 0) {
        return RETURN_OK;
    }
    if (libgit2_rc > 0) {
        /* Defensive: libgit2 never returns positive error codes, but
         * treat as opaque success signal. */
        return RETURN_OK;
    }

    e = git_error_last();
    if (e != NULL && e->message != NULL) {
        fprintf(stderr, "amigit: %s\n", e->message);
    } else {
        fprintf(stderr, "amigit: libgit2 error %d (no message)\n",
                libgit2_rc);
    }
    return RETURN_ERROR;
}

/* ========================================================================
 * Libgit2 lifecycle
 * ======================================================================== */

static void shutdown_libgit2(void)
{
    /* Balanced with the git_libgit2_init() in main(). Per the
     * libgit2 init/shutdown refcount pitfall (known-pitfalls), extra
     * init calls must be matched; we init once here so one shutdown
     * is correct. */
    git_libgit2_shutdown();
}

/* ========================================================================
 * main()
 * ======================================================================== */

int main(int argc, char **argv)
{
    const amigit_command *cmd;
    struct Process *me;
    APTR saved_win;
    int rc;

    (void)verstag;

    /* Suppress AmigaDOS volume requesters -- libgit2's path
     * normalization probes bare names (e.g. T:, WORK:) which would
     * otherwise trigger "please insert volume" requesters. Matches
     * the pattern used by tests/libgit2/test_libgit2.c and the vim
     * port (see known-pitfalls "AmigaDOS Volume Requester"). */
    me = (struct Process *)FindTask(NULL);
    saved_win = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1L;

    if (argc < 2) {
        me->pr_WindowPtr = saved_win;
        return amigit_usage(NULL);
    }

    /* Top-level --help / -h */
    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        me->pr_WindowPtr = saved_win;
        /* usage returns RETURN_ERROR but for explicit --help we
         * want RETURN_OK so Amiga scripts treat it as success. */
        (void)amigit_usage(NULL);
        return RETURN_OK;
    }

    /* Initialize libgit2 before any git_* call. The test suite
     * proved this works in a user binary context (PDR-010 Phase 2
     * Stage 5), but we re-check the refcount defensively. */
    if (git_libgit2_init() < 1) {
        fprintf(stderr, "amigit: git_libgit2_init failed\n");
        me->pr_WindowPtr = saved_win;
        return RETURN_FAIL;
    }
    if (atexit(shutdown_libgit2) != 0) {
        /* atexit failure is rare but possible; shut down manually
         * to keep the refcount balanced. */
        git_libgit2_shutdown();
        fprintf(stderr, "amigit: atexit registration failed\n");
        me->pr_WindowPtr = saved_win;
        return RETURN_FAIL;
    }

    /* Dispatch by argv[1] */
    for (cmd = dispatch_table; cmd->name != NULL; cmd++) {
        if (strcmp(argv[1], cmd->name) == 0) {
            rc = cmd->fn(argc, argv);
            me->pr_WindowPtr = saved_win;
            return rc;
        }
    }

    /* Unknown command */
    fprintf(stderr, "amigit: unknown command '%s'\n", argv[1]);
    (void)amigit_usage(NULL);
    me->pr_WindowPtr = saved_win;
    return RETURN_ERROR;
}
