/*
 * cmd_ls_remote.c -- `amigit ls-remote <url>`
 *
 * PDR-012 Phase 2 skeleton.
 *
 * At Phase 2 this command does the absolute minimum to exercise the
 * HTTPS subtransport registration:
 *   1. git_remote_create_detached(&remote, url)
 *   2. git_remote_connect(remote, GIT_DIRECTION_FETCH, ...)
 *   3. Report the error from our transport_https stub
 *   4. git_remote_free(remote)
 *
 * The connect call routes through libgit2's URL dispatcher, which
 * finds our "https://" registration (via transport_https.c) and hands
 * off to https_action(), which returns -1 with a GIT_ERROR_NET
 * "not implemented" message. The CLI surfaces that via
 * amigit_error_exit(), returning RETURN_ERROR (10).
 *
 * Phase 5 will replace this body with real ref listing once the
 * upload-pack-ls service discovery path is wired through phases
 * 3 (HTTP client), 4 (pkt-line), and 7 (AmiSSL).
 *
 * Phase 2 success criterion: exit 10 with a human-readable error
 * message on the HTTPS URL path, no crash, no memory corruption.
 */

#include <stdio.h>
#include <string.h>

#include <dos/dos.h>    /* RETURN_OK, RETURN_ERROR */

#include "git2.h"

#include "amigit.h"

static int ls_remote_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit ls-remote <url>\n");
    fprintf(out, "\n");
    fprintf(out, "List references from a remote git repository.\n");
    fprintf(out, "Phase 2 stub: HTTPS URLs return \"not implemented\"\n");
    fprintf(out, "from the subtransport action handler.\n");
    return rc;
}

/*
 * Print the current git_error_last() message to stdout as well as
 * returning an RC=10 via amigit_error_exit().
 *
 * Rationale: the FS-UAE test harness only captures stdout, so an
 * EXPECT_CONTAINS assertion on the error message must see it on
 * stdout. amigit_error_exit() prints to stderr (correct for a
 * non-test user), so we echo the message to stdout first. This
 * single-purpose helper is local to cmd_ls_remote because Phase 5
 * will replace most of this file with real ref iteration, at which
 * point the dual-stream pattern becomes irrelevant.
 */
static int ls_remote_fail(int libgit2_rc)
{
    const git_error *e = git_error_last();
    if (e != NULL && e->message != NULL) {
        printf("ls-remote: %s\n", e->message);
    } else {
        printf("ls-remote: libgit2 error %d (no message)\n", libgit2_rc);
    }
    return amigit_error_exit(libgit2_rc);
}

int amigit_cmd_ls_remote(int argc, char **argv)
{
    git_remote *remote = NULL;
    const char *url;
    int rc;

    if (argc < 3) {
        /* Mirror the usage to stdout too -- see ls_remote_fail for
         * the stream-capture rationale. Phase 5 will replace this. */
        printf("ls-remote: missing url argument\n");
        return ls_remote_usage(RETURN_ERROR);
    }
    if (amigit_is_help_flag(argv[2])) {
        return ls_remote_usage(RETURN_OK);
    }

    url = argv[2];

    /* Detached remote: no repository needed, just a URL. The URL
     * dispatcher picks a transport based on the scheme prefix. */
    rc = git_remote_create_detached(&remote, url);
    if (rc < 0) {
        return ls_remote_fail(rc);
    }

    /* This is the call that routes through the HTTPS subtransport
     * registration. At Phase 2 it returns the "not implemented"
     * error from https_action(). */
    rc = git_remote_connect(remote,
                            GIT_DIRECTION_FETCH,
                            NULL,   /* callbacks */
                            NULL,   /* proxy_opts */
                            NULL);  /* custom_headers */

    if (rc < 0) {
        git_remote_free(remote);
        return ls_remote_fail(rc);
    }

    /* Phase 2 never reaches here; the stub always fails. Phase 5
     * will replace this path with real ref iteration. */
    git_remote_free(remote);
    printf("amigit: ls-remote connect succeeded (unexpected in Phase 2)\n");
    return RETURN_OK;
}
