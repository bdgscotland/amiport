/*
 * http.h -- HTTP/1.0 client library for AmigaOS
 *
 * Reusable HTTP GET client built on bsdsocket-shim. Handles:
 * - URL parsing (http:// only, no https)
 * - HTTP/1.0 GET with Host header
 * - 301/302 redirect following (up to 3)
 * - Content-Length validation
 * - Progress callback with speed tracking
 * - 30-second socket timeout via SO_RCVTIMEO/SO_SNDTIMEO
 * - Ctrl-C checking via amiport_check_break()
 *
 * See ADR-010 for bsdsocket-shim design rationale.
 */

#ifndef AMIPORT_NET_HTTP_H
#define AMIPORT_NET_HTTP_H

/* Fetch URL contents to a file. Returns 0 on success, -1 on error.
 * Sets *http_status to HTTP status code (200, 301, 404, etc.).
 * Pass NULL for progress to suppress progress output.
 * Pass NULL for http_status if you don't need it. */
int amiport_http_get(const char *url, const char *dest_path,
                     int *http_status,
                     void (*progress)(long received, long total));

/* Parse URL into components. Returns 0 on success, -1 on error.
 * Only http:// URLs are accepted. https:// returns -1.
 * host buffer must be >= hostsize bytes.
 * path buffer must be >= pathsize bytes. */
int amiport_http_parse_url(const char *url, char *host, int hostsize,
                           int *port, char *path, int pathsize);

#endif /* AMIPORT_NET_HTTP_H */
