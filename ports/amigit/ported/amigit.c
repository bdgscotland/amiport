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
#include "transport_https.h"
#include "http_client.h"   /* amissl_glue_free_cached */

/* ========================================================================
 * amigit_resolve_repo_path -- AmigaDOS path -> libgit2-friendly form
 * ========================================================================
 *
 * Two transformations (see amigit.h for rationale):
 *   1. "." / NULL / empty  -> NameFromLock(pr_CurrentDir)
 *   2. "X:foo"             -> "X:/foo" (so libgit2 sees it as rooted)
 *
 * This is the single choke point for path normalization. Every
 * command that hands a user-supplied path to libgit2 open/init
 * should funnel it through here first.
 */
int amigit_resolve_repo_path(const char *in, char *out, size_t outsize)
{
    char tmp[256];
    const char *src = in;

    if (outsize < 4) {
        return RETURN_ERROR;
    }

    /* Step 1: resolve "." / NULL / "" to an absolute CWD path. */
    if (src == NULL || src[0] == '\0' ||
        (src[0] == '.' && src[1] == '\0')) {
        struct Process *me = (struct Process *)FindTask(NULL);
        BPTR cwd_lock = me->pr_CurrentDir;
        if (cwd_lock == 0 ||
            !NameFromLock(cwd_lock, (STRPTR)tmp, sizeof(tmp) - 1)) {
            return RETURN_ERROR;
        }
        src = tmp;
    }

    /* Step 2: rewrite "X:foo" -> "X:/foo" if needed. */
    if (src[0] != '\0' && src[1] == ':' && src[2] != '\0' &&
        src[2] != '/') {
        size_t slen = strlen(src);
        if (slen + 2 > outsize) {
            return RETURN_ERROR;
        }
        out[0] = src[0];
        out[1] = ':';
        out[2] = '/';
        /* Copy from src[2] through trailing NUL (slen - 2 + 1 bytes). */
        memcpy(&out[3], &src[2], slen - 1);
        return RETURN_OK;
    }

    /* No rewrite needed -- straight copy. */
    if (strlen(src) + 1 > outsize) {
        return RETURN_ERROR;
    }
    strcpy(out, src);
    return RETURN_OK;
}

/* Real AmigaOS honors this; vamos uses -s from the command line.
 * 256 KB matches lib/libgit2 tree/pack walk worst case. */
long __stack = 262144;

/* $VER tag for `version` command. Date is the build day. */
static const char *verstag = "$VER: amigit 0.2 (15.04.2026)";

/* ========================================================================
 * Dispatch table -- single source of truth for v1 command set
 * ========================================================================
 * Phase 3a: version + init. Phase 3b will add status, log, show, diff.
 * Phase 3c will add add, commit, checkout, branch, tag. Each command
 * lives in its own ported/cmd_<name>.c.
 */

static const amigit_command dispatch_table[] = {
    { "version",  amigit_cmd_version,
      "Print amigit and libgit2 version strings" },
    { "init",     amigit_cmd_init,
      "Create an empty git repository" },
    { "status",   amigit_cmd_status,
      "Show worktree and index status" },
    { "log",      amigit_cmd_log,
      "Walk commit history from HEAD" },
    { "show",     amigit_cmd_show,
      "Show a commit with its diff" },
    { "diff",     amigit_cmd_diff,
      "Show index vs worktree diff (or --cached for HEAD vs index)" },
    { "add",      amigit_cmd_add,
      "Stage files into the index" },
    { "commit",   amigit_cmd_commit,
      "Record a commit from the staged index" },
    { "checkout", amigit_cmd_checkout,
      "Switch HEAD to a branch, tag, or commit" },
    { "branch",   amigit_cmd_branch,
      "List, create, or delete local branches" },
    { "tag",      amigit_cmd_tag,
      "List or create lightweight tags" },
    { "ls-remote", amigit_cmd_ls_remote,
      "List references from a remote over HTTPS" },
    { "clone",    amigit_cmd_clone,
      "Clone a remote git repository over HTTPS" },
    { NULL,       NULL,              NULL }
};

/* ========================================================================
 * amigit_usage -- central help output
 * ======================================================================== */

int amigit_usage(const char *cmd_name, int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    const amigit_command *cmd;

    if (cmd_name == NULL) {
        fprintf(out, "usage: amigit <command> [args]\n\n");
        fprintf(out, "amiport-native git client built on libgit2.\n");
        fprintf(out, "Clone and manage repositories over HTTPS.\n\n");
        fprintf(out, "Commands:\n");
        for (cmd = dispatch_table; cmd->name != NULL; cmd++) {
            fprintf(out, "  %-10s  %s\n", cmd->name, cmd->summary);
        }
        fprintf(out, "\nSee: amigit <command> --help for per-command usage.\n");
        return rc;
    }

    /* Per-command usage: each command handles its own --help/-h by
     * printing its usage and returning RETURN_OK. This branch is only
     * reached if the dispatcher itself wants to emit a usage hint for
     * a known but misused command. */
    fprintf(out, "usage: amigit %s ...\n", cmd_name);
    fprintf(out, "Run 'amigit %s --help' for details.\n", cmd_name);
    return rc;
}

/* ========================================================================
 * amigit_is_help_flag -- shared "--help" / "-h" recognizer
 * ======================================================================== */

int amigit_is_help_flag(const char *s)
{
    return strcmp(s, "--help") == 0 || strcmp(s, "-h") == 0;
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
        return amigit_usage(NULL, RETURN_ERROR);
    }

    /* Top-level --help / -h: success context, print to stdout. */
    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        me->pr_WindowPtr = saved_win;
        return amigit_usage(NULL, RETURN_OK);
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

    /* Release cached AmiSSL library handles (AmiSSLMasterBase /
     * AmiSSLBase) on exit. No-op if amissl_glue_open_io was never
     * called (e.g. local-only commands). PDR-012 Phase 3. */
    (void)atexit(amissl_glue_free_cached);

    /* Register the amigit custom HTTPS subtransport. Must be called
     * AFTER git_libgit2_init() (which sets up the transport registry)
     * and BEFORE any git_remote_connect on an https:// URL. At Phase 2
     * this is a stub that returns "not implemented"; Phase 3+ fleshes
     * it out. See ports/amigit/ported/transport_https.c and
     * docs/pdr/012-amigit-https-networking.md. */
    if (amigit_transport_https_register() != 0) {
        fprintf(stderr,
            "amigit: failed to register HTTPS transport\n");
        /* Not fatal for local-only commands -- let the dispatch
         * continue. Commands that need HTTPS will fail when they
         * try to connect. */
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
    (void)amigit_usage(NULL, RETURN_ERROR);
    me->pr_WindowPtr = saved_win;
    return RETURN_ERROR;
}
