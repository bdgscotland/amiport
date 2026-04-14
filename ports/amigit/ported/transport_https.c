/*
 * transport_https.c -- amigit custom smart-HTTP(S) transport backend
 *
 * PDR-012 Phase 2 (stub skeleton).
 *
 * What this file does today:
 *   - Defines a git_smart_subtransport whose action() returns
 *     GIT_ERROR_NOT_IMPLEMENTED (-1) with a clear error message for
 *     every git_smart_service_t verb.
 *   - Registers a git_smart_subtransport_definition under the
 *     "https://" URL prefix via git_transport_register(), so that
 *     libgit2's URL dispatch (transport_find_by_url) picks up our
 *     backend before falling back to the transport_stubs.c
 *     git_smart_subtransport_http stub.
 *
 * What this file does NOT do yet:
 *   - Open any socket. No bsdsocket.library usage.
 *   - Open AmiSSL. No TLS.
 *   - Parse any HTTP framing or pkt-line framing.
 *
 * That all lives in later phases (3 through 7). The point of the
 * Phase 2 skeleton is to prove that:
 *   1. An HTTPS URL routes through our registered backend, not the
 *      upstream stub or a crash in libgit2 path probing.
 *   2. Allocation/free bookkeeping matches libgit2's expected
 *      lifecycle (action() may be followed by close()/free()).
 *   3. The error message makes it back out to the caller so the CLI
 *      can surface it as a RETURN_ERROR (10) with a readable string.
 *
 * Lifecycle contract from git2/sys/transport.h:
 *
 *   A "definition" is long-lived (static in this TU). git_transport_smart
 *   uses it to build a fresh git_smart_subtransport per git_remote_connect.
 *
 *   Smart transport calls:
 *     1. definition.callback(&subt, owner, definition.param) -- create
 *     2. subt.action(&stream, subt, url, service) -- zero or more times
 *     3. subt.close(subt) -- when the smart transport is done with it
 *     4. subt.free(subt) -- final teardown
 *
 *   For a stateless (rpc=1) transport like HTTPS, action() may be
 *   called again after close() with a fresh stream -- the close()
 *   only ends the prior stream's network exchange, not the
 *   subtransport object itself.
 *
 *   At Phase 2 we never create a stream, so close() is a no-op.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "git2.h"
#include "git2/sys/transport.h"
#include "git2/sys/errors.h"

#include "transport_https.h"

/* ========================================================================
 * Subtransport object
 * ======================================================================== */

/*
 * The parent git_smart_subtransport struct must be the FIRST field --
 * libgit2 passes a git_smart_subtransport* back into our callbacks and
 * we cast it straight to this type. If you add state later (socket fd,
 * AmiSSL context, response buffer, ...), put it after `parent`.
 */
typedef struct {
    git_smart_subtransport parent;
    git_transport         *owner;  /* back-reference to the smart transport */
} amigit_https_subtransport;

/* ========================================================================
 * Action handler -- Phase 2 stub
 * ======================================================================== */

static int https_action(
    git_smart_subtransport_stream **out,
    git_smart_subtransport *subt,
    const char *url,
    git_smart_service_t action)
{
    const char *verb;
    char buf[256];

    (void)subt;

    if (out != NULL) {
        *out = NULL;
    }

    switch (action) {
    case GIT_SERVICE_UPLOADPACK_LS:  verb = "upload-pack-ls";  break;
    case GIT_SERVICE_UPLOADPACK:     verb = "upload-pack";     break;
    case GIT_SERVICE_RECEIVEPACK_LS: verb = "receive-pack-ls"; break;
    case GIT_SERVICE_RECEIVEPACK:    verb = "receive-pack";    break;
    default:                         verb = "unknown";         break;
    }

    /* Return a negative error code. libgit2's smart transport surfaces
     * this back through git_remote_connect() to our caller, which
     * turns it into a printable message via amigit_error_exit().
     *
     * git_error_set_str is a plain string setter (no printf-style
     * formatting), so build the message in a local buffer first. The
     * buffer is copied into libgit2's internal error storage by
     * git_error_set_str. */
    (void)snprintf(buf, sizeof(buf),
        "amigit: HTTPS transport not implemented yet "
        "(service=%s, url=%s, scheduled for amigit 0.2 per PDR-012)",
        verb, url ? url : "(null)");
    git_error_set_str(GIT_ERROR_NET, buf);
    return -1;
}

/* ========================================================================
 * close/free -- Phase 2 stub
 * ======================================================================== */

static int https_close(git_smart_subtransport *subt)
{
    /* No network state to tear down yet. Phase 3 will close the
     * socket fd here. Phase 7 will shut down the AmiSSL session. */
    (void)subt;
    return 0;
}

static void https_free(git_smart_subtransport *subt)
{
    /* Mirror of the malloc in https_subtransport_cb. */
    free(subt);
}

/* ========================================================================
 * Definition callback -- invoked by git_transport_smart per connect
 * ======================================================================== */

static int https_subtransport_cb(
    git_smart_subtransport **out,
    git_transport *owner,
    void *param)
{
    amigit_https_subtransport *t;

    (void)param;

    if (out == NULL) {
        git_error_set_str(GIT_ERROR_INVALID,
            "amigit: https_subtransport_cb called with NULL out");
        return -1;
    }

    t = (amigit_https_subtransport *)calloc(1, sizeof(*t));
    if (t == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: out of memory allocating https subtransport");
        return -1;
    }

    t->parent.action = https_action;
    t->parent.close  = https_close;
    t->parent.free   = https_free;
    t->owner         = owner;

    *out = &t->parent;
    return 0;
}

/* ========================================================================
 * Registration
 * ======================================================================== */

/*
 * The definition is stored at file scope because git_transport_register
 * keeps a pointer to it (it does not copy the struct). Giving it static
 * lifetime means the pointer stays valid for the lifetime of the
 * process, which matches libgit2's expectation.
 *
 * rpc = 1 (stateless) matches the HTTPS wire protocol: each
 * request/response is independent at the TCP level. libgit2's smart
 * transport uses this flag to decide whether to hold one physical
 * connection across action() calls.
 */
static git_smart_subtransport_definition amigit_https_def = {
    https_subtransport_cb,
    1,      /* rpc */
    NULL    /* param -- unused at Phase 2 */
};

static int amigit_https_registered = 0;

/*
 * Note: libgit2's public git2/sys/transport.h documents the `prefix`
 * argument as "The scheme (ending in "://") to match, i.e. git://".
 * The IMPLEMENTATION in lib/libgit2/src/libgit2/transport.c disagrees:
 * git_str_printf(&prefix, "%s://", scheme) -- it takes the bare scheme
 * and appends "://" itself. Passing "https://" results in an internal
 * prefix of "https:////" which never matches any URL. Code wins over
 * docs; pass the bare "https" here.
 *
 * The same discrepancy applies to git_transport_unregister.
 */
int amigit_transport_https_register(void)
{
    int rc;

    if (amigit_https_registered) {
        return 0;
    }

    rc = git_transport_register(
        "https",
        git_transport_smart,
        &amigit_https_def);

    if (rc == 0) {
        amigit_https_registered = 1;
    }

    return rc;
}

void amigit_transport_https_cleanup(void)
{
    if (!amigit_https_registered) {
        return;
    }

    (void)git_transport_unregister("https");
    amigit_https_registered = 0;
}
