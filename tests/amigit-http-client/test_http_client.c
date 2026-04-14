/*
 * test_http_client.c -- vamos unit tests for amigit's http_client parser.
 *
 * PDR-012 Phase 3. Exercises status-line, header, and body parsing
 * over a memory-backed io vtable -- no bsdsocket, no AmiSSL. Lets
 * us catch parser regressions on vamos in seconds.
 *
 * The TLS branch of http_client.c is NOT exercised here (vamos has
 * no amisslmaster.library). Live TLS verification happens on
 * FS-UAE with AmiSSL installed and on real hardware.
 *
 * __stack is sized to match tests/zlib and tests/libgit2. The
 * parser itself uses ~4 KB of locals per http_conn_t (header
 * name/value scratch + line buffer).
 */

#include "test_framework.h"
#include "http_client.h"

#include <stdlib.h>
#include <string.h>

/* vamos ignores __stack at runtime, but real AmigaOS reads it.
 * Keep it consistent across test harnesses. */
long __stack = 262144;

/* ============================================================
 * memory_io -- an http_io_t backed by a fixed buffer
 * ============================================================
 *
 * read()  : drains the in-buffer; returns 0 at end (EOF).
 * write() : appends to the out-buffer (so send_request tests can
 *           verify what was sent). Bounded -- returns short write
 *           if the out-buffer fills.
 * close() : frees priv (but the buffers themselves are owned by
 *           the caller / test).
 */

typedef struct {
    const char *in;       /* response bytes */
    int         in_len;
    int         in_pos;

    char       *out;      /* request bytes captured */
    int         out_cap;
    int         out_len;
} memio_priv_t;

static int
memio_read(http_io_t *io, void *buf, int len)
{
    memio_priv_t *p = (memio_priv_t *)io->priv;
    int avail = p->in_len - p->in_pos;
    int n;
    if (avail <= 0) {
        return 0; /* EOF */
    }
    n = (len < avail) ? len : avail;
    memcpy(buf, p->in + p->in_pos, (size_t)n);
    p->in_pos += n;
    return n;
}

static int
memio_write(http_io_t *io, const void *buf, int len)
{
    memio_priv_t *p = (memio_priv_t *)io->priv;
    int room = p->out_cap - p->out_len;
    int n;
    if (room <= 0) {
        return -1;
    }
    n = (len < room) ? len : room;
    memcpy(p->out + p->out_len, buf, (size_t)n);
    p->out_len += n;
    return n;
}

static void
memio_close(http_io_t *io)
{
    (void)io;
    /* caller-owned buffers, nothing to free here */
}

static http_conn_t *
open_memio(const char *response, char *request_out, int request_cap,
           memio_priv_t *priv, http_io_t *io)
{
    http_conn_t *conn = NULL;

    priv->in = response;
    priv->in_len = (int)strlen(response);
    priv->in_pos = 0;
    priv->out = request_out;
    priv->out_cap = request_cap;
    priv->out_len = 0;

    io->read  = memio_read;
    io->write = memio_write;
    io->close = memio_close;
    io->priv  = priv;

    if (http_connect_io(&conn, io) != HTTP_OK) {
        return NULL;
    }
    return conn;
}

/* ============================================================
 * Tests: status line
 * ============================================================ */

TEST(status_200)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;

    c = open_memio("HTTP/1.1 200 OK\r\n\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);
    ASSERT_EQ(status, 200);
    http_close(c);
}

TEST(status_301_http10)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;

    c = open_memio("HTTP/1.0 301 Moved Permanently\r\n\r\n",
                   NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);
    ASSERT_EQ(status, 301);
    http_close(c);
}

TEST(status_404)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;

    c = open_memio("HTTP/1.1 404 Not Found\r\n\r\n",
                   NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);
    ASSERT_EQ(status, 404);
    http_close(c);
}

TEST(status_500)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;

    c = open_memio("HTTP/1.1 500 Internal Server Error\r\n\r\n",
                   NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);
    ASSERT_EQ(status, 500);
    http_close(c);
}

TEST(status_malformed_rejected)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;

    /* No HTTP/1.x prefix */
    c = open_memio("GARBAGE\r\n\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT(http_read_response_status(c, &status) < 0);
    http_close(c);
}

TEST(status_bad_version_rejected)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;

    /* HTTP/2.0 not accepted */
    c = open_memio("HTTP/2.0 200 OK\r\n\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT(http_read_response_status(c, &status) < 0);
    http_close(c);
}

/* ============================================================
 * Tests: header iteration
 * ============================================================ */

TEST(headers_single)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;
    const char *name, *value;

    c = open_memio("HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/plain\r\n"
                   "\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);
    ASSERT_EQ(status, 200);

    ASSERT_EQ(http_read_response_header(c, &name, &value), HTTP_OK);
    ASSERT_STR_EQ(name, "Content-Type");
    ASSERT_STR_EQ(value, "text/plain");

    /* End of headers */
    ASSERT_EQ(http_read_response_header(c, &name, &value), 1);
    http_close(c);
}

TEST(headers_multi)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;
    const char *name, *value;

    c = open_memio("HTTP/1.1 200 OK\r\n"
                   "Server: github.com\r\n"
                   "Content-Type: application/x-git-upload-pack-advertisement\r\n"
                   "Content-Length: 42\r\n"
                   "Cache-Control: no-cache\r\n"
                   "\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);

    ASSERT_EQ(http_read_response_header(c, &name, &value), HTTP_OK);
    ASSERT_STR_EQ(name, "Server");
    ASSERT_STR_EQ(value, "github.com");

    ASSERT_EQ(http_read_response_header(c, &name, &value), HTTP_OK);
    ASSERT_STR_EQ(name, "Content-Type");
    ASSERT_STR_EQ(value, "application/x-git-upload-pack-advertisement");

    ASSERT_EQ(http_read_response_header(c, &name, &value), HTTP_OK);
    ASSERT_STR_EQ(name, "Content-Length");
    ASSERT_STR_EQ(value, "42");

    ASSERT_EQ(http_read_response_header(c, &name, &value), HTTP_OK);
    ASSERT_STR_EQ(name, "Cache-Control");
    ASSERT_STR_EQ(value, "no-cache");

    ASSERT_EQ(http_read_response_header(c, &name, &value), 1);
    http_close(c);
}

TEST(headers_whitespace_tolerated)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;
    const char *name, *value;

    /* Value has leading tab + trailing spaces */
    c = open_memio("HTTP/1.1 200 OK\r\n"
                   "X-Token:\tsecret   \r\n"
                   "\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);

    ASSERT_EQ(http_read_response_header(c, &name, &value), HTTP_OK);
    ASSERT_STR_EQ(name, "X-Token");
    ASSERT_STR_EQ(value, "secret");
    http_close(c);
}

TEST(headers_malformed_rejected)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;
    const char *name, *value;

    /* Header with no colon */
    c = open_memio("HTTP/1.1 200 OK\r\n"
                   "InvalidHeaderNoColon\r\n"
                   "\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);
    ASSERT(http_read_response_header(c, &name, &value) < 0);
    http_close(c);
}

/* ============================================================
 * Tests: body (Content-Length)
 * ============================================================ */

TEST(body_content_length)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;
    const char *name, *value;
    char body[64];
    int n;

    c = open_memio("HTTP/1.1 200 OK\r\n"
                   "Content-Length: 11\r\n"
                   "\r\n"
                   "hello world", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);

    /* Drain headers */
    while (http_read_response_header(c, &name, &value) == HTTP_OK) {
        /* skip */
    }
    http_set_content_length(c, 11);

    n = http_read_body(c, body, (int)sizeof(body));
    ASSERT(n > 0);
    ASSERT(n <= 11);
    body[n] = '\0';

    /* May need multiple reads to drain */
    {
        int got = n;
        while (got < 11) {
            n = http_read_body(c, body + got, (int)sizeof(body) - got);
            if (n <= 0) break;
            got += n;
        }
        body[got] = '\0';
        ASSERT_EQ(got, 11);
        ASSERT_STR_EQ(body, "hello world");
    }

    /* Next read returns 0 (end of body) */
    ASSERT_EQ(http_read_body(c, body, (int)sizeof(body)), 0);
    http_close(c);
}

TEST(body_content_length_zero)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    int status = -1;
    const char *name, *value;
    char body[16];

    c = open_memio("HTTP/1.1 204 No Content\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n", NULL, 0, &priv, &io);
    ASSERT_NOT_NULL(c);
    ASSERT_EQ(http_read_response_status(c, &status), HTTP_OK);
    ASSERT_EQ(status, 204);
    while (http_read_response_header(c, &name, &value) == HTTP_OK) {
        /* skip */
    }
    http_set_content_length(c, 0);

    ASSERT_EQ(http_read_body(c, body, (int)sizeof(body)), 0);
    http_close(c);
}

/* ============================================================
 * Tests: request writer -- verifies what goes on the wire
 * ============================================================ */

TEST(send_request_get)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    char req[512];

    c = open_memio("", req, (int)sizeof(req), &priv, &io);
    ASSERT_NOT_NULL(c);

    ASSERT_EQ(http_send_request(c, "GET",
                "/info/refs?service=git-upload-pack",
                "Host: github.com\r\n"
                "User-Agent: amigit/0.2\r\n"
                "Accept: application/x-git-upload-pack-advertisement\r\n",
                NULL, 0), HTTP_OK);

    /* NUL-terminate captured output */
    priv.out[priv.out_len] = '\0';
    ASSERT(strstr(priv.out,
        "GET /info/refs?service=git-upload-pack HTTP/1.1\r\n") != NULL);
    ASSERT(strstr(priv.out, "Host: github.com\r\n") != NULL);
    ASSERT(strstr(priv.out, "User-Agent: amigit/0.2\r\n") != NULL);
    /* Ends with empty CRLF */
    ASSERT(priv.out_len >= 4);
    ASSERT(memcmp(priv.out + priv.out_len - 4, "\r\n\r\n", 4) == 0);
    http_close(c);
}

TEST(send_request_post_with_body)
{
    memio_priv_t priv;
    http_io_t io;
    http_conn_t *c;
    char req[512];
    const char body[] = "0032want 1111222233334444\n0000";
    int body_len = (int)sizeof(body) - 1;

    c = open_memio("", req, (int)sizeof(req), &priv, &io);
    ASSERT_NOT_NULL(c);

    ASSERT_EQ(http_send_request(c, "POST", "/git-upload-pack",
                "Host: github.com\r\n"
                "Content-Type: application/x-git-upload-pack-request\r\n"
                "Content-Length: 30\r\n",
                body, body_len), HTTP_OK);

    priv.out[priv.out_len] = '\0';
    ASSERT(strstr(priv.out, "POST /git-upload-pack HTTP/1.1\r\n") != NULL);
    ASSERT(strstr(priv.out, body) != NULL);
    http_close(c);
}

/* ============================================================
 * main
 * ============================================================ */

int main(void)
{
    RUN_TEST(status_200);
    RUN_TEST(status_301_http10);
    RUN_TEST(status_404);
    RUN_TEST(status_500);
    RUN_TEST(status_malformed_rejected);
    RUN_TEST(status_bad_version_rejected);

    RUN_TEST(headers_single);
    RUN_TEST(headers_multi);
    RUN_TEST(headers_whitespace_tolerated);
    RUN_TEST(headers_malformed_rejected);

    RUN_TEST(body_content_length);
    RUN_TEST(body_content_length_zero);

    RUN_TEST(send_request_get);
    RUN_TEST(send_request_post_with_body);

    return test_summary();
}
