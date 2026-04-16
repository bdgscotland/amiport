/*
 * transport_https.c -- amigit custom smart-HTTP(S) transport backend
 *
 * PDR-012 Phase 5: service discovery (UPLOADPACK_LS) via
 *                  GET /info/refs?service=git-upload-pack.
 * PDR-012 Phase 6: upload-pack POST body (UPLOADPACK) via
 *                  POST /git-upload-pack with a buffered want/have
 *                  negotiation, plus one-level HTTPS redirect
 *                  support shared by both the LS and POST paths.
 *
 * What this file does:
 *   - Implements a git_smart_subtransport whose action() speaks real
 *     HTTP/1.1 over AmiSSL via the http_client layer.
 *   - Handles GIT_SERVICE_UPLOADPACK_LS: opens the connection, issues
 *     the discovery GET, and returns a read-only stream whose read()
 *     feeds ref-advertisement bytes to libgit2's pkt-line parser.
 *   - Handles GIT_SERVICE_UPLOADPACK: returns a deferred stream whose
 *     write() appends libgit2-generated pkt-line frames (want/have/
 *     flush/done) to an internal heap buffer, and whose first read()
 *     flushes that buffer as a single HTTP POST, then streams the
 *     pack response body bytes back to libgit2's indexer.
 *   - Returns GIT_ERROR_NET with a clear "not implemented yet" message
 *     for GIT_SERVICE_RECEIVEPACK_LS and GIT_SERVICE_RECEIVEPACK, which
 *     are Phase 11 scope (git push).
 *   - Follows up to 3 HTTPS redirects (301/302/307/308) on either verb.
 *     Location headers must be absolute https:// URLs. GitHub's common
 *     "https://github.com/foo/bar -> https://github.com/foo/bar.git"
 *     redirect is the primary target.
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
 * Memory discipline: the stream owns the http_conn_t, and (for POST)
 * the accumulated request body buffer. The subtransport owns a back-
 * pointer to the current stream (so free paths can clear it). Every
 * error path inside the action handlers either (a) has not yet
 * allocated anything, (b) tears down the http_conn_t before returning
 * if a stream was not yet built, or (c) returns -1 leaving the stream
 * alive and expects libgit2 to call stream->free() which cleans up
 * both the body buffer and the connection.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "git2.h"
#include "git2/sys/transport.h"
#include "git2/sys/errors.h"

#include "amigit.h"            /* AMIGIT_VERSION */
#include "credential.h"        /* Phase 7: HTTP Basic auth sourcing */
#include "http_client.h"
#include "transport_https.h"

/* ========================================================================
 * Types
 * ======================================================================== */

#define AMIGIT_HTTPS_URL_MAX         1024
#define AMIGIT_HTTPS_MAX_REDIRECTS   3
#define AMIGIT_HTTPS_POST_INIT_CAP   8192
/* Upper bound on the POST body we'll buffer. libgit2's want/have lists
 * for a fresh clone are a handful of KB; a full history fetch with
 * thousands of haves is typically < 256 KB. 8 MB is a defensive cap. */
#define AMIGIT_HTTPS_POST_MAX_CAP    (8u * 1024u * 1024u)

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
 * The stream owns an http_conn_t that was opened in action() (for the
 * GET LS path) or is opened lazily on first read() (for the POST path).
 * For POST streams, the stream also owns body_buf during the accumulate
 * phase; body_buf is free()'d once the POST has been dispatched.
 */
struct amigit_https_stream {
    git_smart_subtransport_stream parent;
    amigit_https_subtransport    *owner;
    http_conn_t                  *conn;
    int                           done;         /* 1 once body fully drained */

    /* PDR-012 Phase 6: POST body accumulation for upload-pack.
     *
     * is_post == 0 (GET LS path):
     *   action() opens the connection, sends the GET, reads the status
     *   line + headers, sets the body mode, and stores the live conn
     *   on the stream. post_url / body_* fields are unused.
     *
     * is_post == 1 (POST path):
     *   action() only stashes the URL in post_url and returns the stream
     *   with conn == NULL. libgit2 then calls stream->write one or more
     *   times to supply the pkt-line framed body, which accumulates
     *   into body_buf (heap, grown via realloc). The first stream->read
     *   triggers the actual http_connect + http_send_request, then
     *   reads the response status + headers and flips conn to live.
     *   body_buf is free()'d at that point since the bytes are out.
     */
    int    is_post;
    char   post_url[AMIGIT_HTTPS_URL_MAX];
    char  *body_buf;
    size_t body_len;
    size_t body_cap;
    int    post_sent;
};

/*
 * http_request_result_t -- output of open_request_with_redirects().
 * The live http_conn_t has its body mode already set and is ready to
 * drive through http_read_body(). On failure the caller receives an
 * error message in errbuf and does NOT need to touch conn.
 */
typedef struct {
    http_conn_t *conn;
    int          status;
    int          chunked;
    long         content_length;  /* -1 if unset or read-to-EOF */
} http_request_result_t;

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
 * command is removed post-Phase 7, so the small duplication is
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
 * Shared request driver: connect, send, read status + headers, follow
 * up to 3 HTTPS redirects. Leaves conn->body reader armed with
 * chunked or Content-Length mode. PDR-012 Phase 6.
 *
 * initial_url   full https://host[:port]/basepath URL, no service suffix
 * method        "GET" or "POST"
 * path_suffix   e.g. "info/refs?service=git-upload-pack" for LS,
 *               "git-upload-pack" for POST. No leading slash -- the
 *               helper inserts one.
 * accept_hdr    the application/x-git-*-advertisement or -result value
 *               to send in the Accept: header
 * content_type  the application/x-git-*-request value for Content-Type,
 *               or NULL to skip (GET path)
 * body          POST body bytes (may be NULL for GET)
 * body_len      POST body byte count (0 for GET)
 * out           filled with the live conn + status on success
 * errbuf        filled with a human-readable error on failure
 *
 * Returns 0 on success, -1 on any failure (including 3xx without
 * Location, too many redirects, non-200 final status, transport errors).
 * ======================================================================== */

static int
open_request_with_redirects(
    const char *initial_url,
    const char *method,
    const char *path_suffix,
    const char *accept_hdr,
    const char *content_type_hdr,
    const void *body,
    int         body_len,
    http_request_result_t *out,
    char       *errbuf,
    size_t      errbuf_sz)
{
    char current_url[AMIGIT_HTTPS_URL_MAX];
    int  redirects = 0;

    /* Phase 7: 401 retry state. auth_header is either empty (no
     * auth attempted yet) or holds the full "Authorization: Basic
     * <base64>\r\n" line to append to the headers string on the
     * next iteration. auth_attempted caps the retry at 1 so we do
     * not loop forever on a server that persistently returns 401
     * (e.g. wrong PAT). */
    char auth_header[512];
    int  auth_attempted = 0;
    auth_header[0] = '\0';

    if (out != NULL) {
        memset(out, 0, sizeof(*out));
        out->content_length = -1;
    }
    if (initial_url == NULL || method == NULL || path_suffix == NULL ||
        accept_hdr == NULL || errbuf == NULL || errbuf_sz == 0) {
        if (errbuf != NULL && errbuf_sz > 0) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: internal: bad args to open_request_with_redirects");
        }
        return -1;
    }

    if (strlen(initial_url) + 1 > sizeof(current_url)) {
        (void)snprintf(errbuf, errbuf_sz,
            "amigit: URL too long (max %d bytes)",
            AMIGIT_HTTPS_URL_MAX - 1);
        return -1;
    }
    (void)strcpy(current_url, initial_url);

    for (;;) {
        char host[256];
        char path[512];
        char req_path[1024];
        char host_hdr[320];
        char headers[1536];
        char redirect_target[AMIGIT_HTTPS_URL_MAX];
        int  port = 443;
        int  rc;
        int  status = -1;
        http_conn_t *conn = NULL;
        int  chunked_seen = 0;
        long content_length = -1;
        int  is_redirect = 0;
        int  hdrs_len;
        size_t plen;
        const char *path_sep;
        const char *suffix_no_slash;

        redirect_target[0] = '\0';

        if (parse_https_url(current_url, host, sizeof(host), &port,
                            path, sizeof(path)) != 0) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: invalid URL %s "
                "(expected https://host[:port]/path)",
                current_url);
            return -1;
        }

        /* Compose the request path: path + optional slash + suffix. */
        suffix_no_slash = path_suffix;
        if (suffix_no_slash[0] == '/') {
            suffix_no_slash++;
        }
        plen = strlen(path);
        path_sep = (plen > 0 && path[plen - 1] == '/') ? "" : "/";
        rc = snprintf(req_path, sizeof(req_path), "%s%s%s",
                      path, path_sep, suffix_no_slash);
        if (rc < 0 || (size_t)rc >= sizeof(req_path)) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: HTTPS request path too long");
            return -1;
        }

        /* Host header: include :port only for non-default ports. Some
         * origin servers reject "host:443" in the Host header for
         * default-port TLS. */
        if (port == 443) {
            /* amiport: strcpy saves snprintf printf-machinery (hundreds of
             * cycles) -- size already validated by parse_https_url above. */
            (void)strcpy(host_hdr, host);
        } else {
            (void)snprintf(host_hdr, sizeof(host_hdr), "%s:%d", host, port);
        }

        /* Header block. We always send Host / User-Agent / Accept /
         * Accept-Encoding / Pragma / Connection. For POST (when
         * content_type_hdr is non-NULL), we also send Content-Type and
         * Content-Length. http_send_request takes the body bytes
         * directly but expects Content-Length to already be in the
         * headers string -- see http_client.h. */
        hdrs_len = snprintf(headers, sizeof(headers),
            "Host: %s\r\n"
            "User-Agent: git/amigit-%s\r\n"
            "Accept: %s\r\n"
            "Accept-Encoding: identity\r\n"
            "Pragma: no-cache\r\n"
            "Connection: close\r\n",
            host_hdr, AMIGIT_VERSION, accept_hdr);
        if (hdrs_len < 0 || (size_t)hdrs_len >= sizeof(headers)) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: HTTPS request headers too long");
            return -1;
        }
        /* Phase 7: append the Authorization header if a prior 401
         * retry populated it. The full "Authorization: Basic <b64>
         * \r\n" line is already assembled in auth_header -- we just
         * concatenate. Update hdrs_len so the POST Content-Type
         * block below appends at the new position. */
        if (auth_header[0] != '\0') {
            int ah_extra = snprintf(headers + hdrs_len,
                sizeof(headers) - (size_t)hdrs_len,
                "%s", auth_header);
            if (ah_extra < 0 ||
                (size_t)hdrs_len + (size_t)ah_extra >= sizeof(headers)) {
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: HTTPS request headers too long "
                    "after appending Authorization");
                return -1;
            }
            hdrs_len += ah_extra;
        }
        if (content_type_hdr != NULL) {
            int extra = snprintf(headers + hdrs_len,
                sizeof(headers) - (size_t)hdrs_len,
                "Content-Type: %s\r\n"
                "Content-Length: %d\r\n",
                content_type_hdr, body_len);
            if (extra < 0 ||
                (size_t)hdrs_len + (size_t)extra >= sizeof(headers)) {
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: HTTPS POST headers too long");
                return -1;
            }
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
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: HTTPS connect to %s:%d failed (%s)",
                host, port, reason);
            return -1;
        }

        /* Send the request (headers + optional body). */
        rc = http_send_request(conn, method, req_path, headers,
                               body, body_len);
        if (rc != HTTP_OK) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: HTTPS %s %s send failed (http_client rc=%d)",
                method, req_path, rc);
            http_close(conn);
            return -1;
        }

        /* Read status line. */
        rc = http_read_response_status(conn, &status);
        if (rc != HTTP_OK) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: HTTPS %s %s read status failed "
                "(http_client rc=%d)",
                method, req_path, rc);
            http_close(conn);
            return -1;
        }

        is_redirect = (status == 301 || status == 302 ||
                       status == 307 || status == 308);

        /* Iterate headers. Collect Transfer-Encoding, Content-Length,
         * and (if this is a redirect status) Location. */
        for (;;) {
            const char *name = NULL;
            const char *value = NULL;
            int hr = http_read_response_header(conn, &name, &value);
            if (hr < 0) {
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: HTTPS header parse failed "
                    "(http_client rc=%d)", hr);
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
            } else if (is_redirect && ieq_ascii(name, "Location") &&
                       redirect_target[0] == '\0') {
                /* Copy immediately -- name/value point into
                 * http_client's internal header line buffer and are
                 * only valid until the next http_read_response_header
                 * call. */
                size_t vlen = strlen(value);
                if (vlen + 1 > sizeof(redirect_target)) {
                    (void)snprintf(errbuf, errbuf_sz,
                        "amigit: HTTPS redirect Location too long "
                        "(%lu bytes)", (unsigned long)vlen);
                    http_close(conn);
                    return -1;
                }
                memcpy(redirect_target, value, vlen + 1);
            }
        }

        if (is_redirect) {
            size_t rt_len;
            size_t tail_len;
            char tail[540];

            if (redirect_target[0] == '\0') {
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: HTTPS %s %s returned status %d "
                    "but no Location header",
                    method, req_path, status);
                http_close(conn);
                return -1;
            }
            if (strncmp(redirect_target, "https://", 8) != 0) {
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: HTTPS redirect to non-https URL "
                    "not supported: %s",
                    redirect_target);
                http_close(conn);
                return -1;
            }
            if (++redirects > AMIGIT_HTTPS_MAX_REDIRECTS) {
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: too many HTTPS redirects (>%d)",
                    AMIGIT_HTTPS_MAX_REDIRECTS);
                http_close(conn);
                return -1;
            }

            /* If the server's Location includes our service-specific
             * suffix (as GitHub does -- it returns the full resolved
             * URL including /info/refs?service=... or /git-upload-pack),
             * strip it so the next iteration can re-append the suffix
             * cleanly. Servers that redirect to the repo root without
             * the suffix are handled transparently by leaving
             * redirect_target as-is. */
            rt_len = strlen(redirect_target);
            (void)snprintf(tail, sizeof(tail), "/%s", suffix_no_slash);
            tail_len = strlen(tail);
            if (rt_len >= tail_len &&
                strcmp(redirect_target + rt_len - tail_len, tail) == 0) {
                redirect_target[rt_len - tail_len] = '\0';
            }

            /* Swap current_url for the redirect target and loop. */
            if (strlen(redirect_target) + 1 > sizeof(current_url)) {
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: HTTPS redirect target too long");
                http_close(conn);
                return -1;
            }
            (void)strcpy(current_url, redirect_target);
            http_close(conn);
            conn = NULL;
            continue;
        }

        /* Phase 7: 401 Unauthorized -> fetch credentials and retry.
         *
         * We cap the retry at 1: if the server still returns 401
         * after we sent Authorization: Basic, fall through to the
         * `status != 200` branch below with a clear error. This
         * avoids an infinite auth loop when a stored PAT is wrong
         * or expired.
         *
         * We deliberately do NOT drain the response body. The
         * server was told Connection: close, so it will not reuse
         * the connection; closing without draining is the same
         * thing every redirect-retry already does (see the
         * is_redirect branch above). TCP layer handles the cleanup.
         *
         * Security: all credential buffers are stack-local and are
         * scrubbed via amigit_credential_zero before this block
         * returns -- including on every error-return path. The
         * only thing that survives into the retry iteration is
         * auth_header, which holds the already-base64-encoded
         * "Authorization: Basic <b64>\r\n" line. The plaintext
         * credential never leaves this block.
         */
        if (status == 401 && !auth_attempted) {
            char cred_user[128];
            char cred_token[512];
            char cred_pair[640];
            char cred_b64[1024];
            char cred_errbuf[256];
            int  cred_pair_len;
            int  cred_b64_len;
            int  ah_built_len;

            http_close(conn);
            conn = NULL;

            if (amigit_credential_get(cred_user, sizeof(cred_user),
                                      cred_token, sizeof(cred_token),
                                      cred_errbuf,
                                      sizeof(cred_errbuf)) != 0) {
                amigit_credential_zero(cred_user,  sizeof(cred_user));
                amigit_credential_zero(cred_token, sizeof(cred_token));
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: HTTPS %s %s returned 401 and %s",
                    method, req_path, cred_errbuf);
                amigit_credential_zero(cred_errbuf, sizeof(cred_errbuf));
                return -1;
            }

            cred_pair_len = snprintf(cred_pair, sizeof(cred_pair),
                                     "%s:%s", cred_user, cred_token);
            amigit_credential_zero(cred_user,  sizeof(cred_user));
            amigit_credential_zero(cred_token, sizeof(cred_token));

            if (cred_pair_len < 0 ||
                (size_t)cred_pair_len >= sizeof(cred_pair)) {
                amigit_credential_zero(cred_pair, sizeof(cred_pair));
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: credential pair exceeds buffer "
                    "(username + token too long)");
                return -1;
            }

            cred_b64_len = amigit_base64_encode(cred_pair,
                                                (size_t)cred_pair_len,
                                                cred_b64,
                                                sizeof(cred_b64));
            amigit_credential_zero(cred_pair, sizeof(cred_pair));

            if (cred_b64_len < 0) {
                amigit_credential_zero(cred_b64, sizeof(cred_b64));
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: base64 encoder overflow "
                    "(credential pair too long)");
                return -1;
            }

            ah_built_len = snprintf(auth_header, sizeof(auth_header),
                "Authorization: Basic %s\r\n", cred_b64);
            amigit_credential_zero(cred_b64, sizeof(cred_b64));

            if (ah_built_len < 0 ||
                (size_t)ah_built_len >= sizeof(auth_header)) {
                amigit_credential_zero(auth_header, sizeof(auth_header));
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: Authorization header exceeds buffer");
                return -1;
            }

            auth_attempted = 1;
            continue;  /* resend with the Authorization header set */
        }

        /* Non-redirect path. Only 200 is acceptable as a final status
         * for smart-HTTP -- the client does not follow 3xx to non-2xx
         * or interpret 4xx/5xx as anything but failure. */
        if (status != 200) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: HTTPS %s %s returned status %d (expected 200)",
                method, req_path, status);
            http_close(conn);
            /* Phase 7: scrub auth_header on this error path -- after
             * a failed auth retry this holds a base64-encoded
             * credential that could otherwise linger on the stack. */
            amigit_credential_zero(auth_header, sizeof(auth_header));
            return -1;
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

        if (out != NULL) {
            out->conn = conn;
            out->status = status;
            out->chunked = chunked_seen;
            out->content_length = content_length;
        }
        /* Phase 7: scrub auth_header on the success path too. It
         * held a base64-encoded credential; even though the stack
         * frame will be reused, the volatile wipe is discipline. */
        amigit_credential_zero(auth_header, sizeof(auth_header));
        return 0;
    }
}

/* ========================================================================
 * POST dispatch (Phase 6)
 *
 * Called from https_stream_read on the first read after action() built
 * a deferred POST stream. Opens the connection, sends the buffered
 * body, reads the status + headers, wires conn on the stream, and
 * frees body_buf. After this runs, subsequent reads drain the body
 * via the same path the GET LS stream uses.
 * ======================================================================== */

static int
dispatch_post_if_needed(amigit_https_stream *st)
{
    http_request_result_t res;
    char errbuf[384];
    int rc;

    if (st == NULL || !st->is_post || st->post_sent) {
        return 0;
    }

    /* body_len is size_t -> int: clamp defensively. Our heap cap of
     * 8 MB is well under INT_MAX on every 68k target so this clamp is
     * belt-and-suspenders. */
    if (st->body_len > 0x7FFFFFFFu) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: POST body exceeds 2 GB (unreachable)");
        return -1;
    }

    rc = open_request_with_redirects(
        st->post_url,
        "POST",
        "git-upload-pack",
        "application/x-git-upload-pack-result",
        "application/x-git-upload-pack-request",
        st->body_buf, (int)st->body_len,
        &res, errbuf, sizeof(errbuf));
    if (rc != 0) {
        git_error_set_str(GIT_ERROR_NET, errbuf);
        return -1;
    }

    /* Connection is live and body mode is set. Hand it to the stream
     * and release the accumulated body buffer -- bytes are out. */
    st->conn = res.conn;
    if (st->body_buf != NULL) {
        free(st->body_buf);
        st->body_buf = NULL;
        st->body_len = 0;
        st->body_cap = 0;
    }
    st->post_sent = 1;
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
    if (st == NULL || buffer == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: read on invalid HTTPS stream");
        return -1;
    }

    /* POST streams defer the actual request until the first read --
     * libgit2 must have finished supplying the body via stream->write
     * before it starts reading. */
    if (st->is_post && !st->post_sent) {
        if (dispatch_post_if_needed(st) != 0) {
            return -1;
        }
    }

    if (st->conn == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: read on HTTPS stream with no connection");
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
    amigit_https_stream *st = (amigit_https_stream *)s;
    size_t need;
    size_t new_cap;
    char *new_buf;

    if (st == NULL || buffer == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: write on invalid HTTPS stream");
        return -1;
    }
    if (!st->is_post) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: write on non-POST HTTPS stream "
            "(libgit2 should never call this on a LS stream)");
        return -1;
    }
    if (st->post_sent) {
        /* RPC=1 stateless transports never re-enter write after read --
         * if this fires it means libgit2's smart transport changed
         * contract or our dispatcher routed the wrong stream. */
        git_error_set_str(GIT_ERROR_NET,
            "amigit: write after POST already dispatched "
            "(libgit2 stream re-entry detected)");
        return -1;
    }
    if (len == 0) {
        return 0;
    }

    /* Grow body_buf geometrically. Start at 8 KB, double until we fit,
     * cap at AMIGIT_HTTPS_POST_MAX_CAP (8 MB). */
    need = st->body_len + len;
    if (need < st->body_len) {
        /* size_t wrap -- impossible with our 8 MB cap but guard anyway */
        git_error_set_str(GIT_ERROR_NET,
            "amigit: POST body size overflow");
        return -1;
    }
    if (need > AMIGIT_HTTPS_POST_MAX_CAP) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: POST body exceeds 8 MB cap "
            "(upload-pack request too large)");
        return -1;
    }

    if (need > st->body_cap) {
        new_cap = st->body_cap == 0 ? AMIGIT_HTTPS_POST_INIT_CAP
                                    : st->body_cap;
        while (new_cap < need) {
            if (new_cap > AMIGIT_HTTPS_POST_MAX_CAP / 2) {
                new_cap = AMIGIT_HTTPS_POST_MAX_CAP;
                break;
            }
            new_cap *= 2;
        }
        if (new_cap < need) {
            git_error_set_str(GIT_ERROR_NET,
                "amigit: POST body cap reached before buffer fit");
            return -1;
        }
        new_buf = (char *)realloc(st->body_buf, new_cap);
        if (new_buf == NULL) {
            /* realloc failure: original body_buf is still valid and
             * still owned by the stream; free happens in stream_free. */
            git_error_set_str(GIT_ERROR_NET,
                "amigit: out of memory growing POST body buffer");
            return -1;
        }
        st->body_buf = new_buf;
        st->body_cap = new_cap;
    }

    memcpy(st->body_buf + st->body_len, buffer, len);
    st->body_len += len;
    return 0;
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
    if (st->body_buf != NULL) {
        free(st->body_buf);
        st->body_buf = NULL;
        st->body_len = 0;
        st->body_cap = 0;
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
    http_request_result_t res;
    char errbuf[384];
    amigit_https_stream *stream = NULL;
    int rc;

    rc = open_request_with_redirects(
        url,
        "GET",
        "info/refs?service=git-upload-pack",
        "application/x-git-upload-pack-advertisement",
        NULL /* content_type */,
        NULL, 0 /* body */,
        &res, errbuf, sizeof(errbuf));
    if (rc != 0) {
        git_error_set_str(GIT_ERROR_NET, errbuf);
        return -1;
    }

    /* All good -- wrap the live conn in a stream object. libgit2 now
     * calls stream->read() repeatedly, parses pkt-line frames out of
     * our body bytes, and eventually calls stream->free(). */
    stream = (amigit_https_stream *)calloc(1, sizeof(*stream));
    if (stream == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: out of memory allocating HTTPS stream");
        http_close(res.conn);
        return -1;
    }
    stream->parent.subtransport = &subt->parent;
    stream->parent.read  = https_stream_read;
    stream->parent.write = https_stream_write;
    stream->parent.free  = https_stream_free;
    stream->owner = subt;
    stream->conn  = res.conn;
    stream->done  = 0;
    stream->is_post = 0;
    stream->post_url[0] = '\0';
    stream->body_buf = NULL;
    stream->body_len = 0;
    stream->body_cap = 0;
    stream->post_sent = 0;

    subt->current_stream = stream;
    *out_stream = &stream->parent;
    return 0;
}

/* ========================================================================
 * Upload-pack POST action -- POST /git-upload-pack (Phase 6)
 *
 * Returns a DEFERRED stream: we do NOT touch the network here. libgit2
 * calls stream->write() one or more times to supply the pkt-line
 * framed want/have/flush/done body, then calls stream->read() to
 * consume the response. The first read() is where we actually issue
 * the HTTP POST (see dispatch_post_if_needed above).
 *
 * Deferring the POST lets us send a single HTTP request with a known
 * Content-Length header, which matches upstream libgit2's http.c
 * behavior and is simpler than streaming Transfer-Encoding: chunked
 * requests. libgit2 upload-pack POST bodies for a typical clone are
 * a handful of KB; a full history fetch is typically under 256 KB;
 * the 8 MB hard cap in stream_write is more than enough.
 * ======================================================================== */

static int
https_action_uploadpack_post(amigit_https_subtransport *subt,
                             const char *url,
                             git_smart_subtransport_stream **out_stream)
{
    amigit_https_stream *stream = NULL;
    size_t ulen;

    if (url == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: HTTPS POST dispatch given NULL URL");
        return -1;
    }
    ulen = strlen(url);
    if (ulen + 1 > AMIGIT_HTTPS_URL_MAX) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: HTTPS POST URL too long");
        return -1;
    }

    stream = (amigit_https_stream *)calloc(1, sizeof(*stream));
    if (stream == NULL) {
        git_error_set_str(GIT_ERROR_NET,
            "amigit: out of memory allocating HTTPS POST stream");
        return -1;
    }

    stream->parent.subtransport = &subt->parent;
    stream->parent.read  = https_stream_read;
    stream->parent.write = https_stream_write;
    stream->parent.free  = https_stream_free;
    stream->owner = subt;
    stream->conn  = NULL;    /* deferred until first read */
    stream->done  = 0;
    stream->is_post   = 1;
    stream->post_sent = 0;
    stream->body_buf  = NULL;
    stream->body_len  = 0;
    stream->body_cap  = 0;
    memcpy(stream->post_url, url, ulen + 1);

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
        return https_action_uploadpack_post(subt, url, out);

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
        "(url=%s, scheduled per PDR-012 Phase 11)",
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

