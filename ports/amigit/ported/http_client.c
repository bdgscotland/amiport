/*
 * http_client.c -- HTTP/1.1 client for amigit smart-HTTP transport.
 *
 * PDR-012 Phase 3. See http_client.h for the public API.
 *
 * Architecture: the client is split into three pieces so the
 * parser half can be tested on vamos without bsdsocket or AmiSSL.
 *
 *   1. Parser (this file) -- pure C. Reads from an abstract
 *      http_io_t vtable (read/write/close function pointers).
 *      No knowledge of sockets or SSL.
 *
 *   2. Socket backend (this file, __AMIGA__ only) -- implements
 *      http_io_t over bsdsocket-shim. Plain TCP; no TLS.
 *
 *   3. AmiSSL backend (amissl_glue.c, __AMIGA__ only) -- implements
 *      http_io_t over SSL_read/SSL_write with manual OpenLibrary
 *      of amisslmaster.library. Isolated so the parser TU doesn't
 *      need the AmiSSL headers.
 *
 * Memory discipline: every malloc is tracked on the http_conn_t
 * and freed by http_close() including partial-init failure paths.
 * See memory-checker audit notes in the PDR-012 session checkpoint.
 */

#include "http_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __AMIGA__
/* bsdsocket-shim: DNS + socket + connect + send/recv. */
#define AMIPORT_NET_MACROS
#include <amiport-net/socket.h>
#include <amiport-net/netdb.h>
#include <amiport-net/arpa/inet.h>
#include <errno.h>
#endif /* __AMIGA__ */

/* AmiSSL glue lives in amissl_glue.c. These symbols are resolved
 * at link time on Amiga builds. On the host/vamos build where we
 * don't link AmiSSL, the TLS path is simply not reachable -- unit
 * tests pass use_tls=0 and never call amissl_glue_open_io(). */
int  amissl_glue_open_io(http_io_t *io, int sockfd, const char *host);
void amissl_glue_free_cached(void);

/* ============================================================
 * http_conn_t -- parse state + owned io
 * ============================================================ */

struct http_conn {
    http_io_t *io;
    int        owns_io;       /* 1 if http_close should free io */

    /* Read-ahead buffer for line-oriented parsing. We read whatever
     * the io returns into rbuf, then hand lines out of it. */
    char   rbuf[HTTP_MAX_HEADER_LINE + 64];
    int    rlen;              /* bytes currently in rbuf */
    int    rpos;              /* next byte to consume */

    /* Scratch buffers for header name/value pointers returned to
     * the caller. Valid until the next http_read_response_header
     * call. Stored on the conn so the caller doesn't have to
     * allocate. */
    char   hname[HTTP_MAX_HEADER_LINE];
    char   hvalue[HTTP_MAX_HEADER_LINE];

    long   content_length;    /* -1 = unknown (read to EOF) */
    long   body_consumed;     /* bytes of body already returned */

    /* Chunked transfer encoding state. Enabled by http_set_chunked.
     * Mutually exclusive with content_length > 0. */
    int    chunked;                 /* 1 = chunked decode active */
    int    chunked_done;            /* 1 = terminator + trailers consumed */
    int    chunk_needs_trailing_crlf; /* 1 = drain "\r\n" after chunk body */
    long   chunk_remaining;         /* bytes left in current chunk */
};

http_io_t *
http_conn_io(http_conn_t *conn)
{
    return conn ? conn->io : NULL;
}

void
http_set_content_length(http_conn_t *conn, long length)
{
    if (conn != NULL) {
        conn->content_length = length;
        conn->body_consumed = 0;
        conn->chunked = 0;
        conn->chunked_done = 0;
        conn->chunk_needs_trailing_crlf = 0;
        conn->chunk_remaining = 0;
    }
}

void
http_set_chunked(http_conn_t *conn)
{
    if (conn != NULL) {
        conn->chunked = 1;
        conn->chunked_done = 0;
        conn->chunk_needs_trailing_crlf = 0;
        conn->chunk_remaining = 0;
        conn->content_length = -1;
        conn->body_consumed = 0;
    }
}

/* ============================================================
 * Low-level read helpers -- work over the io vtable
 * ============================================================ */

/* Refill rbuf from the io if there's nothing left to consume.
 * Returns >0 on success, 0 on EOF, <0 on error. */
static int
conn_fill(http_conn_t *conn)
{
    int n;

    if (conn->rpos < conn->rlen) {
        return 1;
    }
    conn->rpos = 0;
    conn->rlen = 0;
    n = conn->io->read(conn->io, conn->rbuf, (int)sizeof(conn->rbuf));
    if (n < 0) {
        return HTTP_ERR_RECV;
    }
    if (n == 0) {
        return 0; /* EOF */
    }
    conn->rlen = n;
    return 1;
}

/* Read one line terminated by CRLF (or LF) into out[0..out_sz-1].
 * Strips the trailing CRLF. Returns line length (>=0) or negative
 * on error (HTTP_ERR_TOO_LONG, HTTP_ERR_EOF, HTTP_ERR_RECV). */
static int
conn_read_line(http_conn_t *conn, char *out, int out_sz)
{
    int pos = 0;
    int got_cr = 0;
    int rc;

    for (;;) {
        rc = conn_fill(conn);
        if (rc < 0) {
            return rc;
        }
        if (rc == 0) {
            /* EOF mid-line is a protocol error for header parsing. */
            return (pos == 0) ? HTTP_ERR_EOF : HTTP_ERR_PROTOCOL;
        }
        while (conn->rpos < conn->rlen) {
            char c = conn->rbuf[conn->rpos++];
            if (c == '\r') {
                got_cr = 1;
                continue;
            }
            if (c == '\n') {
                if (pos >= out_sz) {
                    return HTTP_ERR_TOO_LONG;
                }
                out[pos] = '\0';
                return pos;
            }
            if (got_cr) {
                /* CR not followed by LF -- tolerate as LF for
                 * HTTP/1.0 compatibility, then emit current line. */
                conn->rpos--; /* push back the non-LF char */
                got_cr = 0;
                if (pos >= out_sz) {
                    return HTTP_ERR_TOO_LONG;
                }
                out[pos] = '\0';
                return pos;
            }
            if (pos + 1 >= out_sz) {
                return HTTP_ERR_TOO_LONG;
            }
            out[pos++] = c;
        }
    }
}

/* ============================================================
 * Parser -- status line, headers, body
 * ============================================================ */

int
http_read_response_status(http_conn_t *conn, int *out_status)
{
    char line[HTTP_MAX_STATUS_LINE];
    int len;
    int status;
    char *sp;

    if (conn == NULL || out_status == NULL) {
        return HTTP_ERR_INVAL;
    }

    len = conn_read_line(conn, line, (int)sizeof(line));
    if (len < 0) {
        return len;
    }

    /* Expect "HTTP/1.x NNN Reason". Tolerate "HTTP/1.0" or "HTTP/1.1". */
    if (len < 12 || memcmp(line, "HTTP/1.", 7) != 0) {
        return HTTP_ERR_PROTOCOL;
    }
    if (line[7] != '0' && line[7] != '1') {
        return HTTP_ERR_PROTOCOL;
    }
    if (line[8] != ' ') {
        return HTTP_ERR_PROTOCOL;
    }

    sp = line + 9;
    if (sp[0] < '0' || sp[0] > '9') {
        return HTTP_ERR_PROTOCOL;
    }
    status = 0;
    while (*sp >= '0' && *sp <= '9') {
        status = status * 10 + (*sp - '0');
        sp++;
    }
    if (status < 100 || status > 599) {
        return HTTP_ERR_PROTOCOL;
    }

    *out_status = status;
    return HTTP_OK;
}

int
http_read_response_header(http_conn_t *conn,
                          const char **out_name,
                          const char **out_value)
{
    char line[HTTP_MAX_HEADER_LINE];
    int len;
    char *colon;
    char *vp;
    int nlen;
    int vlen;

    if (conn == NULL || out_name == NULL || out_value == NULL) {
        return HTTP_ERR_INVAL;
    }

    len = conn_read_line(conn, line, (int)sizeof(line));
    if (len < 0) {
        return len;
    }
    if (len == 0) {
        /* End of headers. */
        *out_name = NULL;
        *out_value = NULL;
        return 1;
    }

    colon = strchr(line, ':');
    if (colon == NULL) {
        return HTTP_ERR_PROTOCOL;
    }

    nlen = (int)(colon - line);
    if (nlen <= 0 || nlen >= (int)sizeof(conn->hname)) {
        return HTTP_ERR_PROTOCOL;
    }
    memcpy(conn->hname, line, (size_t)nlen);
    conn->hname[nlen] = '\0';

    vp = colon + 1;
    while (*vp == ' ' || *vp == '\t') {
        vp++;
    }
    vlen = (int)strlen(vp);
    /* Strip trailing whitespace. */
    while (vlen > 0 && (vp[vlen - 1] == ' ' || vp[vlen - 1] == '\t')) {
        vlen--;
    }
    if (vlen >= (int)sizeof(conn->hvalue)) {
        return HTTP_ERR_PROTOCOL;
    }
    memcpy(conn->hvalue, vp, (size_t)vlen);
    conn->hvalue[vlen] = '\0';

    *out_name = conn->hname;
    *out_value = conn->hvalue;
    return HTTP_OK;
}

/* Return up to max bytes, draining rbuf residue first or pulling
 * directly from the io. 0 == EOF, negative == error. */
static int
drain_or_read(http_conn_t *conn, void *buf, int max)
{
    int available;
    int want;
    int rc;

    available = conn->rlen - conn->rpos;
    if (available > 0) {
        want = (available < max) ? available : max;
        memcpy(buf, conn->rbuf + conn->rpos, (size_t)want);
        conn->rpos += want;
        return want;
    }
    rc = conn->io->read(conn->io, buf, max);
    if (rc < 0) {
        return HTTP_ERR_RECV;
    }
    return rc;
}

/* Read exactly n bytes. EOF before n is HTTP_ERR_EOF. */
static int
read_exact(http_conn_t *conn, void *buf, int n)
{
    char *out = (char *)buf;
    int got = 0;
    while (got < n) {
        int r = drain_or_read(conn, out + got, n - got);
        if (r < 0) {
            return r;
        }
        if (r == 0) {
            return HTTP_ERR_EOF;
        }
        got += r;
    }
    return n;
}

/* Parse a hex-size chunk header line (with optional ;extension).
 * Returns the decoded size >= 0 on success, HTTP_ERR_PROTOCOL on
 * malformed input or integer overflow. The size is clamped to the
 * signed 32-bit range so long-chunk state can track it without
 * unsigned/signed conversion hazards. */
static long
parse_chunk_size(const char *line, int len)
{
    unsigned long size = 0;
    int i;
    int digits = 0;

    for (i = 0; i < len; i++) {
        int c = (unsigned char)line[i];
        int d;

        if (c == ';' || c == ' ' || c == '\t') {
            /* End of size field -- remainder is chunk-ext or
             * lax server trailing whitespace, ignore. */
            break;
        }
        if (c >= '0' && c <= '9') {
            d = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            d = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            d = 10 + (c - 'A');
        } else {
            return HTTP_ERR_PROTOCOL;
        }
        /* Cap at 0x7FFFFFFF (INT32_MAX). Reject anything larger
         * so downstream buffers can use signed 32-bit counters. */
        if (size > 0x07FFFFFFUL) {
            return HTTP_ERR_PROTOCOL;
        }
        size = size * 16UL + (unsigned long)d;
        digits++;
    }
    if (digits == 0) {
        return HTTP_ERR_PROTOCOL;
    }
    if (size > 0x7FFFFFFFUL) {
        return HTTP_ERR_PROTOCOL;
    }
    return (long)size;
}

/* Chunked transfer decoder. Lazily advances the chunk state
 * machine and returns up to max bytes of decoded body content.
 * Returns 0 at end-of-body, negative on error. */
static int
read_body_chunked(http_conn_t *conn, void *buf, int max)
{
    char line[HTTP_MAX_HEADER_LINE];
    char crlf[2];
    long size;
    int rc;
    int want;

    if (conn->chunked_done) {
        return 0;
    }

    for (;;) {
        /* Serve data from the current chunk if we have any. */
        if (conn->chunk_remaining > 0) {
            want = ((long)max < conn->chunk_remaining)
                   ? max : (int)conn->chunk_remaining;
            rc = drain_or_read(conn, buf, want);
            if (rc < 0) {
                return rc;
            }
            if (rc == 0) {
                /* Peer closed mid-chunk. */
                return HTTP_ERR_EOF;
            }
            conn->chunk_remaining -= rc;
            conn->body_consumed += rc;
            return rc;
        }

        /* Consume the mandatory CRLF that follows each chunk body. */
        if (conn->chunk_needs_trailing_crlf) {
            rc = read_exact(conn, crlf, 2);
            if (rc < 0) {
                return rc;
            }
            if (crlf[0] != '\r' || crlf[1] != '\n') {
                return HTTP_ERR_PROTOCOL;
            }
            conn->chunk_needs_trailing_crlf = 0;
        }

        /* Read the next chunk-size line. */
        rc = conn_read_line(conn, line, (int)sizeof(line));
        if (rc < 0) {
            return rc;
        }

        size = parse_chunk_size(line, rc);
        if (size < 0) {
            return (int)size;
        }

        if (size == 0) {
            /* Terminator. Consume any trailer headers followed by
             * the final empty CRLF line. */
            for (;;) {
                rc = conn_read_line(conn, line, (int)sizeof(line));
                if (rc < 0) {
                    return rc;
                }
                if (rc == 0) {
                    break;
                }
            }
            conn->chunked_done = 1;
            return 0;
        }

        conn->chunk_remaining = size;
        conn->chunk_needs_trailing_crlf = 1;
        /* loop back to serve data */
    }
}

int
http_read_body(http_conn_t *conn, void *buf, int max)
{
    int rc;

    if (conn == NULL || buf == NULL || max <= 0) {
        return HTTP_ERR_INVAL;
    }

    if (conn->chunked) {
        return read_body_chunked(conn, buf, max);
    }

    /* Content-Length / read-to-EOF path. */
    if (conn->content_length >= 0) {
        long remaining = conn->content_length - conn->body_consumed;
        if (remaining <= 0) {
            return 0;
        }
        if ((long)max > remaining) {
            max = (int)remaining;
        }
    }

    rc = drain_or_read(conn, buf, max);
    if (rc < 0) {
        return rc;
    }
    if (rc == 0 && conn->content_length > 0) {
        /* Unexpected EOF before Content-Length reached. */
        return HTTP_ERR_EOF;
    }
    conn->body_consumed += rc;
    return rc;
}

/* ============================================================
 * Request writer
 * ============================================================ */

static int
io_write_all(http_io_t *io, const void *buf, int len)
{
    const char *p = (const char *)buf;
    int remaining = len;
    while (remaining > 0) {
        int n = io->write(io, p, remaining);
        if (n <= 0) {
            return HTTP_ERR_SEND;
        }
        p += n;
        remaining -= n;
    }
    return HTTP_OK;
}

int
http_send_request(http_conn_t *conn,
                  const char *method, const char *path,
                  const char *headers,
                  const void *body, int body_len)
{
    char line[HTTP_MAX_STATUS_LINE];
    int rc;
    int n;

    if (conn == NULL || method == NULL || path == NULL) {
        return HTTP_ERR_INVAL;
    }
    if (headers == NULL) {
        headers = "";
    }

    n = snprintf(line, sizeof(line), "%s %s HTTP/1.1\r\n", method, path);
    if (n < 0 || (size_t)n >= sizeof(line)) {
        return HTTP_ERR_TOO_LONG;
    }

    rc = io_write_all(conn->io, line, n);
    if (rc < 0) {
        return rc;
    }
    rc = io_write_all(conn->io, headers, (int)strlen(headers));
    if (rc < 0) {
        return rc;
    }
    rc = io_write_all(conn->io, "\r\n", 2);
    if (rc < 0) {
        return rc;
    }
    if (body != NULL && body_len > 0) {
        rc = io_write_all(conn->io, body, body_len);
        if (rc < 0) {
            return rc;
        }
    }
    return HTTP_OK;
}

/* ============================================================
 * http_connect_io -- test helper / internal constructor
 * ============================================================ */

int
http_connect_io(http_conn_t **out_conn, http_io_t *io)
{
    http_conn_t *conn;

    if (out_conn == NULL || io == NULL || io->read == NULL ||
        io->write == NULL) {
        return HTTP_ERR_INVAL;
    }

    conn = (http_conn_t *)calloc(1, sizeof(*conn));
    if (conn == NULL) {
        return HTTP_ERR_NOMEM;
    }
    conn->io = io;
    conn->owns_io = 0;            /* caller owns the io struct */
    conn->content_length = -1;    /* unknown until headers parsed */
    *out_conn = conn;
    return HTTP_OK;
}

/* ============================================================
 * Socket backend (Amiga only)
 * ============================================================ */

#ifdef __AMIGA__

typedef struct {
    int sockfd;
} socket_priv_t;

static int
socket_read(http_io_t *io, void *buf, int len)
{
    socket_priv_t *p = (socket_priv_t *)io->priv;
    int n = amiport_recv(p->sockfd, buf, len, 0);
    if (n < 0) {
        return HTTP_ERR_RECV;
    }
    return n;
}

static int
socket_write(http_io_t *io, const void *buf, int len)
{
    socket_priv_t *p = (socket_priv_t *)io->priv;
    int n = amiport_send(p->sockfd, buf, len, 0);
    if (n < 0) {
        return HTTP_ERR_SEND;
    }
    return n;
}

static void
socket_close(http_io_t *io)
{
    socket_priv_t *p = (socket_priv_t *)io->priv;
    if (p != NULL) {
        if (p->sockfd >= 0) {
            amiport_closesocket(p->sockfd);
            p->sockfd = -1;
        }
        free(p);
        io->priv = NULL;
    }
}

/*
 * tcp_connect -- DNS resolve + socket + connect. Returns fd >=0
 * on success or HTTP_ERR_DNS / HTTP_ERR_CONNECT on failure.
 */
static int
tcp_connect(const char *host, int port)
{
    struct amiport_addrinfo hints;
    struct amiport_addrinfo *res = NULL;
    struct amiport_addrinfo *ai;
    char portbuf[16];
    int fd = -1;
    int rc;

    snprintf(portbuf, sizeof(portbuf), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    rc = amiport_getaddrinfo(host, portbuf, &hints, &res);
    if (rc != 0 || res == NULL) {
        return HTTP_ERR_DNS;
    }

    for (ai = res; ai != NULL; ai = ai->ai_next) {
        fd = amiport_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (amiport_connect(fd, ai->ai_addr, (socklen_t)ai->ai_addrlen) == 0) {
            break;
        }
        amiport_closesocket(fd);
        fd = -1;
    }

    amiport_freeaddrinfo(res);

    if (fd < 0) {
        return HTTP_ERR_CONNECT;
    }
    return fd;
}

int
http_connect(http_conn_t **out_conn,
             const char *host, int port, int use_tls)
{
    http_conn_t *conn = NULL;
    http_io_t *io = NULL;
    socket_priv_t *priv = NULL;
    int sockfd = -1;
    int rc;

    if (out_conn == NULL || host == NULL || port <= 0) {
        return HTTP_ERR_INVAL;
    }

    sockfd = tcp_connect(host, port);
    if (sockfd < 0) {
        return sockfd;  /* already an HTTP_ERR_* code */
    }

    io = (http_io_t *)calloc(1, sizeof(*io));
    if (io == NULL) {
        amiport_closesocket(sockfd);
        return HTTP_ERR_NOMEM;
    }

    if (use_tls) {
        /* amissl_glue_open_io wires SSL_read/SSL_write into io,
         * assumes ownership of sockfd (will close via SSL_shutdown
         * + closesocket), and allocates its own priv. On failure
         * it MUST NOT leak sockfd or io. */
        rc = amissl_glue_open_io(io, sockfd, host);
        if (rc < 0) {
            /* amissl_glue_open_io is responsible for closing sockfd
             * on its failure paths; no extra close here. */
            free(io);
            return rc;
        }
        /* io->read / io->write / io->close / io->priv now set. */
    } else {
        priv = (socket_priv_t *)calloc(1, sizeof(*priv));
        if (priv == NULL) {
            amiport_closesocket(sockfd);
            free(io);
            return HTTP_ERR_NOMEM;
        }
        priv->sockfd = sockfd;
        io->read  = socket_read;
        io->write = socket_write;
        io->close = socket_close;
        io->priv  = priv;
    }

    conn = (http_conn_t *)calloc(1, sizeof(*conn));
    if (conn == NULL) {
        if (io->close != NULL) {
            io->close(io);
        }
        free(io);
        return HTTP_ERR_NOMEM;
    }
    conn->io = io;
    conn->owns_io = 1;
    conn->content_length = -1;

    *out_conn = conn;
    return HTTP_OK;
}

#else /* !__AMIGA__ -- host/vamos parser test build */

int
http_connect(http_conn_t **out_conn,
             const char *host, int port, int use_tls)
{
    (void)out_conn;
    (void)host;
    (void)port;
    (void)use_tls;
    /* Parser unit tests use http_connect_io() directly. */
    return HTTP_ERR_CONNECT;
}

#endif /* __AMIGA__ */

/* ============================================================
 * http_close
 * ============================================================ */

void
http_close(http_conn_t *conn)
{
    if (conn == NULL) {
        return;
    }
    if (conn->io != NULL) {
        if (conn->io->close != NULL) {
            conn->io->close(conn->io);
        }
        if (conn->owns_io) {
            free(conn->io);
        }
        conn->io = NULL;
    }
    free(conn);
}
