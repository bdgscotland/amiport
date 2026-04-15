/*
 * transport_https.c -- amigit custom smart-HTTP(S) transport backend
 *
 * PDR-012 Phase 5: service discovery.
 *
 * What this file does:
 *   - Implements a git_smart_subtransport whose action() speaks real
 *     HTTP/1.1 over AmiSSL via the http_client layer.
 *   - Handles GIT_SERVICE_UPLOADPACK_LS (the initial discovery GET at
 *     /info/refs?service=git-upload-pack). libgit2 calls action() with
 *     this verb during git_remote_connect(FETCH) on an https:// URL.
 *   - Returns a git_smart_subtransport_stream whose read() streams the
 *     response body bytes back to libgit2 so it can parse the pkt-line
 *     framed ref advertisement. The stream wraps an http_conn_t and
 *     owns its lifecycle through free().
 *   - Returns GIT_ERROR_NET with a clear "not implemented yet" message
 *     for GIT_SERVICE_UPLOADPACK (Phase 6), GIT_SERVICE_RECEIVEPACK_LS,
 *     and GIT_SERVICE_RECEIVEPACK (Phase 11).
 *
 * Registration is unchanged from Phase 2: git_transport_register takes
 * the BARE scheme "https" (not "https://") because libgit2's
 * implementation appends the "://" itself -- a public-header/impl
 * discrepancy that is documented in known-pitfalls.md. This comment
 * is load-bearing: the pitfall re-bit a future session already.
 *
 * Lifecycle contract from git2/sys/transport.h:
 *
 *   A "definition" is long-lived (static in this TU). git_transport_smart
 *   uses it to build a fresh git_smart_subtransport per git_remote_connect.
 *
 *   Smart transport calls:
 *     1. definition.callback(&subt, owner, definition.param) -- create
 *     2. subt.action(&stream, subt, url, service) -- zero or more times
 *     3. subt.close(subt) -- before the next action() on a new URL
 *     4. subt.free(subt) -- final teardown
 *
 *   For RPC=1 (stateless) HTTP, smart.c resets the stream between
 *   actions: current_stream->free() is called BEFORE wrapped->close().
 *   So by the time our https_close() fires, the stream is already gone
 *   and the http_conn_t has already been torn down. close() is a no-op.
 *
 * Memory discipline: the stream owns the http_conn_t, the subtransport
 * owns a back-pointer to the current stream (so free paths can clear
 * it). All error paths inside https_action_uploadpack_ls() MUST close
 * the http_conn_t before returning -1 if a stream was not yet built.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "git2.h"
#include "git2/sys/transport.h"
#include "git2/sys/errors.h"

#include "amigit.h"            /* AMIGIT_VERSION */
#include "http_client.h"
#include "transport_https.h"

/* ========================================================================
 * Types
 * ======================================================================== */

typedef struct amigit_https_subtransport amigit_https_subtransport;
typedef struct amigit_https_stream       amigit_https_stream;

/*
 * The parent git_smart_subtransport struct must be the FIRST field --
 * libgit2 passes a git_smart_subtransport* back into our callbacks and
 * we cast it straight to this type.
 */
struct amigit_https_subtransport {
    git_smart_subtransport parent;
    git_transport         *owner;
    /* Back-reference to the currently-active stream so https_close /
     * https_free can assert no leaks (stream should already be freed
     * by smart.c before close() fires). */
    amigit_https_stream   *current_stream;
};

/*
 * Same pattern for the stream: parent FIRST so libgit2 can cast.
 * The stream owns an http_conn_t that was opened in action() and is
 * torn down in free().
 */
struct amigit_https_stream {
    git_smart_subtransport_stream parent;
    amigit_https_subtransport    *owner;
    http_conn_t                  *conn;
    int                           done;  /* 1 once body fully drained */
};

/* ========================================================================
 * Small local helpers -- no libnix string-case dependency
 *
 * We could use libnix's strcasecmp but keeping this allocation-free and
 * ASCII-only here makes the TU self-contained and avoids yet another
 * link-time dependency during the debug cycle.
 * ======================================================================== */

static char
to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }
    return c;
}

static int
ieq_ascii(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    while (*a != '\0' && *b != '\0') {
        if (to_lower_ascii(*a) != to_lower_ascii(*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

/* Return 1 if needle_lc (already lowercase) appears as a substring of
 * haystack under ASCII case-insensitive comparison. */
static int
contains_ci(const char *haystack, const char *needle_lc)
{
    size_t nlen;
    const char *p;

    if (haystack == NULL || needle_lc == NULL) {
        return 0;
    }
    nlen = strlen(needle_lc);
    if (nlen == 0) {
        return 1;
    }
    for (p = haystack; *p != '\0'; p++) {
        size_t i;
        for (i = 0; i < nlen; i++) {
            char hc = p[i];
            if (hc == '\0') {
                return 0;
            }
            if (to_lower_ascii(hc) != needle_lc[i]) {
                break;
            }
        }
        if (i == nlen) {
            return 1;
        }
    }
    return 0;
}

/* ========================================================================
 * URL parsing -- https://host[:port]/path
 *
 * Returns 0 on success, -1 on malformed input. path gets at least "/"
 * even if the caller omitted it. Parallels cmd_https_probe.c's
 * parse_https_url -- that file goes away once the _https-probe debug
 * command is removed post-Phase 6, so the small duplication is
 * deliberate rather than shared through a header.
 * ======================================================================== */

static int
parse_https_url(const char *url,
                char *host, size_t host_sz,
                int  *port_out,
                char *path, size_t path_sz)
{
    const char *p;
    const char *slash;
    const char *colon;
    int port;
    size_t hlen;

    if (url == NULL || host == NULL || path == NULL || port_out == NULL) {
        return -1;
    }
    if (strncmp(url, "https://", 8) != 0) {
        return -1;
    }
    p = url + 8;

    /* End of authority is first '/' or NUL. */
    slash = strchr(p, '/');
    if (slash == NULL) {
        hlen = strlen(p);
        if (path_sz < 2) {
            return -1;
        }
        path[0] = '/';
        path[1] = '\0';
    } else {
        size_t plen;
        hlen = (size_t)(slash - p);
        plen = strlen(slash);
        if (plen + 1 > path_sz) {
            return -1;
        }
        memcpy(path, slash, plen + 1);
    }

    /* Split optional :port from host. */
    port = 443;
    colon = memchr(p, ':', hlen);
    if (colon != NULL) {
        size_t host_only = (size_t)(colon - p);
        if (host_only == 0 || host_only + 1 >= host_sz) {
            return -1;
        }
        memcpy(host, p, host_only);
        host[host_only] = '\0';
        port = atoi(colon + 1);
        if (port <= 0 || port > 65535) {
            return -1;
        }
    } else {
        if (hlen == 0 || hlen + 1 >= host_sz) {
            return -1;
        }
        memcpy(host, p, hlen);
        host[hlen] = '\0';
    }

    *port_out = port;
    return 0;
}

/* ========================================================================
 * Stream callbacks
 * ======================================================================== */

static int
https_stream_read(git_smart_subtransport_stream *s,
                  char *buffer,
                  size_t buf_size,
                  size_t *bytes_read)
{
    amigit_https_stream *st = (amigit_https_stream *)s;
    int want;
    int n;

    if (bytes_read != NULL) {
        *bytes_read = 0;
    }
    if (st == NULL || st->conn == NULL || buffer == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: read on invalid HTTPS stream");
        return -1;
    }
    if (st->done || buf_size == 0) {
        return 0;
    }

    /* http_read_body takes int; clamp buf_size defensively. libgit2's
     * transport_smart buffer is 64 KB on the 68020 build so this clamp
     * never fires in practice -- belt-and-suspenders. */
    if (buf_size > 0x7FFFFFFFu) {
        want = 0x7FFFFFFF;
    } else {
        want = (int)buf_size;
    }

    n = http_read_body(st->conn, buffer, want);
    if (n < 0) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: error reading HTTPS response body");
        return -1;
    }
    if (n == 0) {
        st->done = 1;
    }
    if (bytes_read != NULL) {
        *bytes_read = (size_t)n;
    }
    return 0;
}

static int
https_stream_write(git_smart_subtransport_stream *s,
                   const char *buffer,
                   size_t len)
{
    (void)s;
    (void)buffer;
    (void)len;
    /* POST bodies (the upload-pack want/have negotiation) are Phase 6.
     * For Phase 5 we only need the initial GET, so write() is never
     * called on a GIT_SERVICE_UPLOADPACK_LS stream. Return a clear
     * error in case something goes wrong in the dispatcher. */
    git_error_set_str(GIT_ERROR_NET,
        "amigit: HTTPS POST (upload-pack body) not implemented yet "
        "-- scheduled for amigit 0.2 per PDR-012 Phase 6");
    return -1;
}

static void
https_stream_free(git_smart_subtransport_stream *s)
{
    amigit_https_stream *st = (amigit_https_stream *)s;

    if (st == NULL) {
        return;
    }
    if (st->owner != NULL && st->owner->current_stream == st) {
        st->owner->current_stream = NULL;
    }
    if (st->conn != NULL) {
        http_close(st->conn);
        st->conn = NULL;
    }
    free(st);
}

/* ========================================================================
 * Upload-pack LS action -- GET /info/refs?service=git-upload-pack
 * ======================================================================== */

static int
https_action_uploadpack_ls(amigit_https_subtransport *subt,
                           const char *url,
                           git_smart_subtransport_stream **out_stream)
{
    char host[256];
    char path[512];
    char req_path[1024];
    char host_hdr[320];
    char headers[1024];
    char errbuf[384];
    int  port = 443;
    int  rc;
    int  status = -1;
    http_conn_t *conn = NULL;
    amigit_https_stream *stream = NULL;
    int  chunked_seen = 0;
    long content_length = -1;
    const char *path_sep;
    size_t plen;

    if (parse_https_url(url, host, sizeof(host), &port,
                        path, sizeof(path)) != 0) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: invalid URL (expected https://host[:port]/path)");
        return -1;
    }

    /* Compose request path. libgit2 passes the user-supplied URL
     * verbatim, so it may or may not end in a slash. */
    plen = strlen(path);
    path_sep = (plen > 0 && path[plen - 1] == '/') ? "" : "/";

    rc = snprintf(req_path, sizeof(req_path),
                  "%s%sinfo/refs?service=git-upload-pack",
                  path, path_sep);
    if (rc < 0 || (size_t)rc >= sizeof(req_path)) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: HTTPS request path too long");
        return -1;
    }

    /* Host header: include :port only for non-default ports. This is
     * the standard convention; some origin servers reject "host:443"
     * in the Host header for default-port TLS. */
    if (port == 443) {
        (void)snprintf(host_hdr, sizeof(host_hdr), "%s", host);
    } else {
        (void)snprintf(host_hdr, sizeof(host_hdr), "%s:%d", host, port);
    }

    rc = snprintf(headers, sizeof(headers),
        "Host: %s\r\n"
        "User-Agent: git/amigit-%s\r\n"
        "Accept: application/x-git-upload-pack-advertisement\r\n"
        "Accept-Encoding: identity\r\n"
        "Pragma: no-cache\r\n"
        "Connection: close\r\n",
        host_hdr, AMIGIT_VERSION);
    if (rc < 0 || (size_t)rc >= sizeof(headers)) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: HTTPS request headers too long");
        return -1;
    }

    /* Open the connection. AmiSSL + bsdsocket plumbing lives behind
     * http_connect -- this fails gracefully with a typed error code
     * if AmiSSL is missing or DNS/connect/TLS handshake fails. */
    rc = http_connect(&conn, host, port, 1 /* use_tls */);
    if (rc != HTTP_OK) {
        const char *reason;
        switch (rc) {
        case HTTP_ERR_DNS:
            reason = "DNS lookup failed";
            break;
        case HTTP_ERR_CONNECT:
            reason = "TCP connect failed";
            break;
        case HTTP_ERR_TLS_MISSING:
            reason = "AmiSSL not installed -- run `amiport install amissl`";
            break;
        case HTTP_ERR_TLS_HANDSHAKE:
            reason = "TLS handshake failed";
            break;
        case HTTP_ERR_NOMEM:
            reason = "out of memory";
            break;
        default:
            reason = "http_client error";
            break;
        }
        (void)snprintf(errbuf, sizeof(errbuf),
            "amigit: HTTPS connect to %s:%d failed (%s)",
            host, port, reason);
        git_error_set_str(GIT_ERROR_NET, errbuf);
        return -1;
    }

    /* Send the request. */
    rc = http_send_request(conn, "GET", req_path, headers, NULL, 0);
    if (rc != HTTP_OK) {
        (void)snprintf(errbuf, sizeof(errbuf),
            "amigit: HTTPS send failed for %s (http_client rc=%d)",
            req_path, rc);
        git_error_set_str(GIT_ERROR_NET, errbuf);
        http_close(conn);
        return -1;
    }

    /* Read status line. */
    rc = http_read_response_status(conn, &status);
    if (rc != HTTP_OK) {
        (void)snprintf(errbuf, sizeof(errbuf),
            "amigit: HTTPS read status failed for %s (http_client rc=%d)",
            req_path, rc);
        git_error_set_str(GIT_ERROR_NET, errbuf);
        http_close(conn);
        return -1;
    }
    if (status != 200) {
        /* Non-200 is fatal at Phase 5 -- redirect support (301/302/
         * 307/308) is Phase 6 scope. Surface the status code so the
         * user understands it's not a transport bug. */
        (void)snprintf(errbuf, sizeof(errbuf),
            "amigit: HTTPS GET %s returned status %d (expected 200)",
            req_path, status);
        git_error_set_str(GIT_ERROR_NET, errbuf);
        http_close(conn);
        return -1;
    }

    /* Iterate headers. Pull out Transfer-Encoding and Content-Length.
     * Any other header is ignored -- we don't care about content type
     * because libgit2 parses the body pkt-line frames on its own. */
    for (;;) {
        const char *name = NULL;
        const char *value = NULL;
        int hr = http_read_response_header(conn, &name, &value);
        if (hr < 0) {
            (void)snprintf(errbuf, sizeof(errbuf),
                "amigit: HTTPS header parse failed (http_client rc=%d)",
                hr);
            git_error_set_str(GIT_ERROR_NET, errbuf);
            http_close(conn);
            return -1;
        }
        if (hr == 1 || name == NULL) {
            break;  /* end of headers */
        }
        if (ieq_ascii(name, "Transfer-Encoding")) {
            if (contains_ci(value, "chunked")) {
                chunked_seen = 1;
            }
        } else if (ieq_ascii(name, "Content-Length")) {
            content_length = atol(value);
            if (content_length < 0) {
                content_length = -1;
            }
        }
    }

    /* Wire the body-read mode. Chunked wins over Content-Length if
     * both are (incorrectly) present; this matches RFC 7230 Section
     * 3.3.3 bullet 3. If neither header was sent the server is
     * HTTP/1.0-style -- read to EOF. */
    if (chunked_seen) {
        http_set_chunked(conn);
    } else if (content_length >= 0) {
        http_set_content_length(conn, content_length);
    } else {
        http_set_content_length(conn, -1);  /* read to EOF */
    }

    /* All good -- wrap in a stream object. libgit2 now calls
     * stream->read() repeatedly, parses pkt-line frames out of our
     * body bytes, and eventually calls stream->free(). */
    stream = (amigit_https_stream *)calloc(1, sizeof(*stream));
    if (stream == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: out of memory allocating HTTPS stream");
        http_close(conn);
        return -1;
    }
    stream->parent.subtransport = &subt->parent;
    stream->parent.read  = https_stream_read;
    stream->parent.write = https_stream_write;
    stream->parent.free  = https_stream_free;
    stream->owner = subt;
    stream->conn  = conn;
    stream->done  = 0;

    subt->current_stream = stream;
    *out_stream = &stream->parent;
    return 0;
}

/* ========================================================================
 * Action dispatcher
 * ======================================================================== */

static int
https_action(git_smart_subtransport_stream **out,
             git_smart_subtransport *subt_p,
             const char *url,
             git_smart_service_t action)
{
    amigit_https_subtransport *subt = (amigit_https_subtransport *)subt_p;
    const char *verb;
    char buf[320];

    if (out != NULL) {
        *out = NULL;
    }

    switch (action) {
    case GIT_SERVICE_UPLOADPACK_LS:
        return https_action_uploadpack_ls(subt, url, out);

    case GIT_SERVICE_UPLOADPACK:
        verb = "upload-pack POST (want/have negotiation)";
        break;
    case GIT_SERVICE_RECEIVEPACK_LS:
        verb = "receive-pack-ls";
        break;
    case GIT_SERVICE_RECEIVEPACK:
        verb = "receive-pack POST";
        break;
    default:
        verb = "unknown";
        break;
    }

    (void)snprintf(buf, sizeof(buf),
        "amigit: HTTPS %s not implemented yet "
        "(url=%s, scheduled per PDR-012 Phase 6/11)",
        verb, url ? url : "(null)");
    git_error_set_str(GIT_ERROR_NET, buf);
    return -1;
}

/* ========================================================================
 * close/free
 * ======================================================================== */

static int
https_close(git_smart_subtransport *subt_p)
{
    amigit_https_subtransport *subt = (amigit_https_subtransport *)subt_p;

    /* smart.c frees the stream via stream->free() BEFORE calling us,
     * so current_stream should already be NULL. Defensive cleanup in
     * case the smart transport's ordering ever changes. */
    if (subt != NULL && subt->current_stream != NULL) {
        https_stream_free(&subt->current_stream->parent);
        subt->current_stream = NULL;
    }
    return 0;
}

static void
https_free(git_smart_subtransport *subt_p)
{
    amigit_https_subtransport *subt = (amigit_https_subtransport *)subt_p;

    if (subt != NULL && subt->current_stream != NULL) {
        https_stream_free(&subt->current_stream->parent);
        subt->current_stream = NULL;
    }
    free(subt);
}

/* ========================================================================
 * Definition callback
 * ======================================================================== */

static int
https_subtransport_cb(git_smart_subtransport **out,
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
    t->current_stream = NULL;

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
    NULL    /* param -- unused */
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
 *
 * See known-pitfalls.md "libgit2 git_transport_register Takes BARE
 * Scheme" for the incident history.
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
