/*
 * http_client.h -- HTTP/1.1 client for amigit smart-HTTP transport.
 *
 * PDR-012 Phase 3: HTTPS-from-day-one. The TLS branch opens AmiSSL
 * via manual OpenLibrary (NOT libamisslauto.a -- that crashes at
 * process start if AmiSSL is missing, see known-pitfalls.md).
 *
 * The parser half (status line, header iteration, Content-Length
 * body reader) is decoupled from the transport via an io vtable
 * so vamos unit tests can drive it from string literals in memory
 * without touching bsdsocket.library or amisslmaster.library.
 *
 * Phase 3 owns: Content-Length body reader only.
 * Phase 4 extends: chunked transfer encoding (this file).
 * Phase 5 will consume: transport_https.c https_action() wires
 *                        http_client into the libgit2 dispatch.
 */
#ifndef AMIGIT_HTTP_CLIENT_H
#define AMIGIT_HTTP_CLIENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes -- negative means error, >=0 means bytes/ok. */
#define HTTP_OK                  0
#define HTTP_ERR_DNS            -1
#define HTTP_ERR_CONNECT        -2
#define HTTP_ERR_TLS_MISSING    -3   /* AmiSSL not installed */
#define HTTP_ERR_TLS_HANDSHAKE  -4
#define HTTP_ERR_SEND           -5
#define HTTP_ERR_RECV           -6
#define HTTP_ERR_PROTOCOL       -7   /* malformed status/header */
#define HTTP_ERR_NOMEM          -8
#define HTTP_ERR_TOO_LONG       -9   /* header/body exceeds buffer */
#define HTTP_ERR_EOF           -10   /* peer closed mid-response */
#define HTTP_ERR_INVAL         -11

/* Max lengths -- chosen to cover real-world git smart-HTTP traffic
 * without unbounded allocation. A typical GitHub header set is
 * ~2 KB; an initial ref advertisement can push the body to 100+ KB
 * but that goes through http_read_body() in chunks. */
#define HTTP_MAX_STATUS_LINE    512
#define HTTP_MAX_HEADER_LINE   2048

/* io vtable -- lets unit tests substitute memory-backed IO. */
typedef struct http_io http_io_t;

typedef int  (*http_io_read_fn)(http_io_t *io, void *buf, int len);
typedef int  (*http_io_write_fn)(http_io_t *io, const void *buf, int len);
typedef void (*http_io_close_fn)(http_io_t *io);

struct http_io {
    http_io_read_fn  read;
    http_io_write_fn write;
    http_io_close_fn close;
    void            *priv;       /* backend-specific context */
};

/* Connection handle. Contains the io vtable and parse state. */
typedef struct http_conn http_conn_t;

/* ============================================================
 * Connection lifecycle
 * ============================================================ */

/*
 * http_connect -- open a connection to host:port.
 *
 * If use_tls != 0, opens AmiSSL via manual OpenLibrary and performs
 * a TLS handshake over the socket. Returns HTTP_ERR_TLS_MISSING if
 * AmiSSL is not installed (caller should print a friendly message).
 *
 * On success, *out_conn is set to a newly allocated http_conn_t.
 * Caller must eventually call http_close(conn).
 */
int http_connect(http_conn_t **out_conn,
                 const char *host, int port, int use_tls);

/*
 * http_connect_io -- construct a connection from a caller-provided
 * io vtable. Used by unit tests to drive the parser over a memory
 * buffer. The connection takes ownership of io; http_close() will
 * call io->close and free the http_conn_t, but not the io struct
 * itself (caller allocates/frees that).
 */
int http_connect_io(http_conn_t **out_conn, http_io_t *io);

/*
 * http_close -- tear down a connection. Frees SSL handles, closes
 * the socket, CloseLibrary()s AmiSSL, frees internal buffers.
 * Handles partial-init cleanup gracefully -- safe to call on a
 * connection that failed mid-setup.
 */
void http_close(http_conn_t *conn);

/* ============================================================
 * Request / response
 * ============================================================ */

/*
 * http_send_request -- format and send the request line + headers
 * + optional body. headers is a NUL-terminated C string with one
 * "Name: value\r\n" per line (caller is responsible for correct
 * formatting -- this keeps the header writer simple and
 * allocation-free).
 *
 * If body_len > 0, body is sent after the headers. Caller must
 * have included Content-Length in the headers string.
 *
 * Returns HTTP_OK on success, HTTP_ERR_SEND on short write.
 */
int http_send_request(http_conn_t *conn,
                      const char *method, const char *path,
                      const char *headers,
                      const void *body, int body_len);

/*
 * http_read_response_status -- read and parse the status line.
 * Fills *out_status with the numeric status code.
 * Returns HTTP_OK or negative on error.
 */
int http_read_response_status(http_conn_t *conn, int *out_status);

/*
 * http_read_response_header -- read one response header line.
 * Returns:
 *   HTTP_OK with *out_name and *out_value populated (pointers
 *           into internal buffer, valid until the next call).
 *   1       if end-of-headers (empty CRLF line) reached.
 *   < 0     on error.
 */
int http_read_response_header(http_conn_t *conn,
                              const char **out_name,
                              const char **out_value);

/*
 * http_read_body -- read up to max bytes of response body into buf.
 * Respects Content-Length seen during header iteration, or chunked
 * transfer encoding if http_set_chunked() was called.
 * Returns the number of bytes read (0 at end-of-body), or negative
 * on error.
 */
int http_read_body(http_conn_t *conn, void *buf, int max);

/*
 * http_set_content_length -- set the expected body length. Called
 * by the caller after header iteration observes Content-Length.
 * Pass -1 for "body until EOF" (HTTP/1.0 fallback). Parser tests
 * use this directly; production code reads it from the headers.
 */
void http_set_content_length(http_conn_t *conn, long length);

/*
 * http_set_chunked -- put the connection into HTTP/1.1 chunked
 * transfer-encoding mode. Caller invokes this instead of
 * http_set_content_length() when header iteration observes a
 * "Transfer-Encoding: chunked" header. Subsequent http_read_body()
 * calls transparently strip chunk size prefixes, CRLF separators,
 * consume the terminating "0\r\n" chunk and any trailer headers.
 *
 * Chunked mode and Content-Length are mutually exclusive. Setting
 * one overrides the other.
 */
void http_set_chunked(http_conn_t *conn);

/* Low-level accessor for testing: expose the io vtable so tests
 * can verify the connection is plumbed correctly. */
http_io_t *http_conn_io(http_conn_t *conn);

/*
 * amissl_glue_free_cached -- atexit hook that releases the cached
 * AmiSSLMasterBase / AmiSSLBase library handles opened by
 * amissl_glue_open_io (called on the first TLS connect). Safe to
 * call even when no TLS connection was ever attempted. Defined in
 * amissl_glue.c.
 */
void amissl_glue_free_cached(void);

/*
 * amissl_glue_probe -- PDR-012 Phase 3 session 3 diagnostic. Runs
 * the same OpenLibrary sequence as ensure_amissl_open() but with
 * heavy per-call instrumentation (AvailMem before/after, IoErr on
 * failure, explicit path fallbacks). Prints results to stdout.
 * Returns 0 on any successful open, nonzero on all-fail. Used by
 * `amigit _https-probe --lib-only` to isolate the OpenLibrary
 * failure inside amigit's runtime environment. NO network calls.
 * Defined in amissl_glue.c.
 */
int amissl_glue_probe(void);

#ifdef __cplusplus
}
#endif

#endif /* AMIGIT_HTTP_CLIENT_H */
