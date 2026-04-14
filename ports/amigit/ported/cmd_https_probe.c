/*
 * cmd_https_probe.c -- debug subcommand for PDR-012 Phase 3
 *
 * Usage: amigit _https-probe <url>
 *
 * Exercises http_client directly (NOT via libgit2 / the smart-HTTP
 * transport) so we can verify the AmiSSL + bsdsocket stack works
 * end-to-end from FS-UAE and real Amiga hardware before Phase 5
 * wires http_client into transport_https.c's https_action().
 *
 * The subcommand name starts with an underscore to mark it as
 * internal-only -- it is NOT advertised in `amigit --help` and
 * is not part of the 1.0 public surface. It will be removed once
 * Phase 5 lands the real HTTPS transport.
 *
 * On success, prints:
 *   probe: status=NNN
 *   (connect+handshake OK, got HTTP status NNN)
 *
 * On any failure, prints a single line identifying the failing
 * stage (DNS, CONNECT, TLS_MISSING, TLS_HANDSHAKE, SEND, RECV,
 * PROTOCOL) and exits RETURN_ERROR. Test harness greps stdout for
 * "status=" as the success marker.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "amigit.h"
#include "http_client.h"

#include <dos/dos.h>  /* RETURN_OK / RETURN_ERROR */

/* Parse https://host[:port]/path into host, port, path. Writes
 * into caller-provided buffers. Returns RETURN_OK on success. */
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

    if (url == NULL || strncmp(url, "https://", 8) != 0) {
        return RETURN_ERROR;
    }
    p = url + 8;

    /* Find end of authority (first '/' or NUL). */
    slash = strchr(p, '/');
    if (slash == NULL) {
        hlen = strlen(p);
        strncpy(path, "/", path_sz - 1);
        path[path_sz - 1] = '\0';
    } else {
        hlen = (size_t)(slash - p);
        strncpy(path, slash, path_sz - 1);
        path[path_sz - 1] = '\0';
    }

    /* Split optional :port from host. */
    port = 443;
    colon = memchr(p, ':', hlen);
    if (colon != NULL) {
        size_t host_only = (size_t)(colon - p);
        if (host_only + 1 >= host_sz) {
            return RETURN_ERROR;
        }
        memcpy(host, p, host_only);
        host[host_only] = '\0';
        port = atoi(colon + 1);
        if (port <= 0 || port > 65535) {
            return RETURN_ERROR;
        }
    } else {
        if (hlen + 1 >= host_sz) {
            return RETURN_ERROR;
        }
        memcpy(host, p, hlen);
        host[hlen] = '\0';
    }

    *port_out = port;
    return RETURN_OK;
}

int
amigit_cmd_https_probe(int argc, char **argv)
{
    const char *url;
    char host[256];
    char path[512];
    char headers[512];
    int  port = 443;
    http_conn_t *conn = NULL;
    int rc;
    int status = -1;

    if (argc < 3 || argv[2] == NULL) {
        printf("probe: usage: amigit _https-probe <https-url>\n");
        return RETURN_ERROR;
    }
    url = argv[2];

    if (parse_https_url(url, host, sizeof(host), &port,
                        path, sizeof(path)) != RETURN_OK) {
        printf("probe: invalid URL (expected https://host[:port][/path])\n");
        return RETURN_ERROR;
    }

    /* Open the HTTPS connection. This drives:
     *  - amiport_getaddrinfo (bsdsocket-shim)
     *  - amiport_socket + amiport_connect
     *  - manual OpenLibrary(amisslmaster.library)
     *  - SSL_CTX_new + SSL_new + SSL_set_fd + SSL_connect
     *    with X509_VERIFY_PARAM_set1_host for hostname binding.
     */
    rc = http_connect(&conn, host, port, 1 /* use_tls */);
    if (rc != HTTP_OK) {
        switch (rc) {
        case HTTP_ERR_DNS:
            printf("probe: DNS failed for %s\n", host);
            break;
        case HTTP_ERR_CONNECT:
            printf("probe: CONNECT failed to %s:%d\n", host, port);
            break;
        case HTTP_ERR_TLS_MISSING:
            printf("probe: HTTPS not available (AmiSSL not installed); "
                   "run `amiport install amissl`\n");
            break;
        case HTTP_ERR_TLS_HANDSHAKE:
            printf("probe: TLS handshake failed for %s:%d\n", host, port);
            break;
        default:
            printf("probe: http_connect returned %d\n", rc);
            break;
        }
        return RETURN_ERROR;
    }

    /* Minimal HTTP/1.1 GET. Close the connection when we're done
     * so we don't have to deal with keepalive in Phase 3. */
    snprintf(headers, sizeof(headers),
             "Host: %s\r\n"
             "User-Agent: amigit/0.2 (PDR-012 Phase 3 probe)\r\n"
             "Connection: close\r\n"
             "Accept: */*\r\n",
             host);

    rc = http_send_request(conn, "GET", path, headers, NULL, 0);
    if (rc != HTTP_OK) {
        printf("probe: SEND failed (%d)\n", rc);
        http_close(conn);
        return RETURN_ERROR;
    }

    rc = http_read_response_status(conn, &status);
    if (rc != HTTP_OK) {
        printf("probe: RECV/PROTOCOL failed reading status (%d)\n", rc);
        http_close(conn);
        return RETURN_ERROR;
    }

    printf("probe: status=%d\n", status);

    /* Drain headers for cleanliness, not strictly required. */
    for (;;) {
        const char *name, *value;
        int hr = http_read_response_header(conn, &name, &value);
        if (hr <= 0) {
            break;
        }
        (void)name;
        (void)value;
    }

    http_close(conn);
    return RETURN_OK;
}
