/*
 * cmd_ls_remote.c -- `amigit ls-remote <url>`
 *
 * PDR-012 Phase 5: real service discovery.
 *
 * Walks the refs advertised by a remote git repository. Under the hood
 * this drives:
 *   1. git_remote_create_detached(&remote, url)
 *   2. git_remote_connect(remote, FETCH, ...) -- dispatches through
 *      our custom HTTPS subtransport (transport_https.c), which sends
 *      GET /info/refs?service=git-upload-pack and feeds the response
 *      body bytes back into libgit2's pkt-line parser.
 *   3. git_remote_ls(&refs, &n, remote) -- returns the parsed ref list.
 *   4. Print each ref as "<sha1-hex>\t<refname>" (git ls-remote format).
 *   5. git_remote_disconnect + git_remote_free.
 *
 * Output format matches `git ls-remote` exactly so downstream scripts
 * (grep, awk, test harness diff) work with either implementation.
 *
 * Error paths are surfaced via ls_remote_fail(): libgit2's last error
 * message is copied to stdout (so the FS-UAE test harness can capture
 * it) and then stderr via amigit_error_exit(). The transport_https
 * layer attaches detailed diagnostics (HTTP status, DNS error,
 * certificate failure) to git_error_set_str so the user sees a
 * readable message, not just "transport error".
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
    fprintf(out, "\n");
    fprintf(out, "Supported URL scheme: https://host[:port]/path\n");
    fprintf(out, "Requires AmiSSL -- run `amiport install amissl`.\n");
    return rc;
}

/*
 * Print the current git_error_last() message to stdout as well as
 * returning an RC=10 via amigit_error_exit().
 *
 * Rationale: the FS-UAE test harness only captures stdout, so an
 * EXPECT_CONTAINS assertion on the error message must see it on
 * stdout. amigit_error_exit() prints to stderr (correct for a
 * non-test user), so we echo the message to stdout first.
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
    const git_remote_head **refs = NULL;
    size_t refs_len = 0;
    size_t i;
    const char *url;
    int rc;

    if (argc < 3) {
        /* Mirror the error to stdout -- see ls_remote_fail for the
         * stream-capture rationale. */
        printf("ls-remote: missing url argument\n");
        return ls_remote_usage(RETURN_ERROR);
    }
    if (amigit_is_help_flag(argv[2])) {
        return ls_remote_usage(RETURN_OK);
    }

    url = argv[2];

    /* Detached remote: no repository needed, just a URL. The URL
     * dispatcher picks a transport based on the scheme prefix -- our
     * registered HTTPS backend handles https://. */
    rc = git_remote_create_detached(&remote, url);
    if (rc < 0) {
        return ls_remote_fail(rc);
    }

    /* Routes through transport_https.c:https_action() for https://
     * URLs. Phase 5 implements GIT_SERVICE_UPLOADPACK_LS; other
     * services still return "not implemented". */
    rc = git_remote_connect(remote,
                            GIT_DIRECTION_FETCH,
                            NULL,   /* callbacks */
                            NULL,   /* proxy_opts */
                            NULL);  /* custom_headers */
    if (rc < 0) {
        git_remote_free(remote);
        return ls_remote_fail(rc);
    }

    rc = git_remote_ls(&refs, &refs_len, remote);
    if (rc < 0) {
        git_remote_disconnect(remote);
        git_remote_free(remote);
        return ls_remote_fail(rc);
    }

    /* Format matches native `git ls-remote`: "<40-char-sha>\t<refname>".
     * When a ref has a symref target (e.g. HEAD -> refs/heads/main),
     * we still print the sha for the target since libgit2's
     * git_remote_head->oid already resolves the symref. */
    for (i = 0; i < refs_len; i++) {
        char sha[GIT_OID_SHA1_HEXSIZE + 1];
        git_oid_tostr(sha, sizeof(sha), &refs[i]->oid);
        printf("%s\t%s\n", sha, refs[i]->name);
    }

    if (refs_len == 0) {
        printf("ls-remote: server advertised no refs\n");
    }

    git_remote_disconnect(remote);
    git_remote_free(remote);
    return RETURN_OK;
}
