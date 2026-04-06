/*
 * http.c -- HTTP/1.0 GET client for AmigaOS
 *
 * Built on bsdsocket-shim. Static buffers only, no malloc for I/O.
 * Handles redirects, Content-Length validation, progress, timeouts.
 *
 * amiport: original code for lib/http-shim
 */

#include <amiport-net/http.h>
#include <amiport-net/socket.h>
#include <amiport-net/netdb.h>
#include <amiport-net/netinet/in.h>
#include <amiport-net/arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __AMIGA__
#include <amiport/signal.h>
#endif

/* --- URL Parsing --- */

int amiport_http_parse_url(const char *url, char *host, int hostsize,
                           int *port, char *path, int pathsize)
{
    const char *p;
    const char *host_start;
    const char *host_end;
    const char *port_start;
    const char *path_start;
    int hostlen;
    int pathlen;

    if (!url || !host || !port || !path) return -1;
    if (hostsize < 2 || pathsize < 2) return -1;

    /* Must be http:// */
    if (strncmp(url, "http://", 7) != 0) return -1;

    /* Reject https:// explicitly */
    if (strncmp(url, "https://", 8) == 0) return -1;

    p = url + 7; /* skip "http://" */
    host_start = p;

    /* Find end of host (: for port, / for path, \0 for end) */
    host_end = NULL;
    port_start = NULL;
    path_start = NULL;

    while (*p && *p != ':' && *p != '/') p++;

    host_end = p;
    *port = 80; /* default */

    if (*p == ':') {
        /* Parse port */
        p++;
        port_start = p;
        *port = 0;
        while (*p >= '0' && *p <= '9') {
            *port = *port * 10 + (*p - '0');
            p++;
        }
        if (*port <= 0 || *port > 65535) return -1;
        if (port_start == p) return -1; /* no digits after : */
    }

    if (*p == '/') {
        path_start = p;
    }

    /* Validate host */
    hostlen = (int)(host_end - host_start);
    if (hostlen <= 0 || hostlen >= hostsize) return -1;
    memcpy(host, host_start, hostlen);
    host[hostlen] = '\0';

    /* Copy path */
    if (path_start) {
        pathlen = (int)strlen(path_start);
        if (pathlen >= pathsize) return -1;
        memcpy(path, path_start, pathlen);
        path[pathlen] = '\0';
    } else {
        if (pathsize < 2) return -1;
        path[0] = '/';
        path[1] = '\0';
    }

    return 0;
}

/* --- HTTP GET Implementation --- */

/* perf: 8KB recv buffer reduces recv() syscalls 4x for typical downloads.
 * Each recv() on AmigaOS is a context switch to bsdsocket.library. */
#define HTTP_RECV_BUF 8192
#define HTTP_MAX_REDIRECTS 3
#define HTTP_TIMEOUT_SECS 30
#define HTTP_HOST_MAX 128
#define HTTP_PATH_MAX 512

/* Static socket fd for atexit cleanup */
static int http_active_sockfd = -1;
static FILE *http_active_fp = NULL;
static char http_active_tmppath[128];

static void http_atexit_cleanup(void)
{
    if (http_active_fp) {
        fclose(http_active_fp);
        http_active_fp = NULL;
    }
    if (http_active_sockfd >= 0) {
        amiport_closesocket(http_active_sockfd);
        http_active_sockfd = -1;
    }
    if (http_active_tmppath[0]) {
        remove(http_active_tmppath);
        http_active_tmppath[0] = '\0';
    }
}

static int http_atexit_registered = 0;

/* Find \r\n\r\n boundary in buffer. Returns offset of body start, or -1. */
static int find_header_end(const char *buf, int len)
{
    int i;
    for (i = 0; i < len - 3; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n' &&
            buf[i+2] == '\r' && buf[i+3] == '\n') {
            return i + 4;
        }
    }
    return -1;
}

/* Parse HTTP status line. Returns status code or -1. */
static int parse_status_line(const char *buf)
{
    /* HTTP/1.x NNN reason */
    const char *p = buf;
    int code;

    if (strncmp(p, "HTTP/1.", 7) != 0) return -1;
    p += 7;
    if (*p != '0' && *p != '1') return -1;
    p++;
    if (*p != ' ') return -1;
    p++;

    code = 0;
    while (*p >= '0' && *p <= '9') {
        code = code * 10 + (*p - '0');
        p++;
    }
    if (code < 100 || code > 599) return -1;
    return code;
}

/* Extract header value (case-insensitive key match). Returns 0 on found. */
static int find_header(const char *headers, int hdr_len,
                       const char *key, char *value, int valsize)
{
    const char *p = headers;
    const char *end = headers + hdr_len;
    int keylen = (int)strlen(key);

    while (p < end) {
        /* Find start of line */
        const char *line = p;
        const char *eol = p;
        while (eol < end && *eol != '\r' && *eol != '\n') eol++;

        /* Case-insensitive key match */
        if ((int)(eol - line) > keylen + 1) {
            int match = 1;
            int i;
            for (i = 0; i < keylen; i++) {
                char a = line[i];
                char b = key[i];
                if (a >= 'A' && a <= 'Z') a += 32;
                if (b >= 'A' && b <= 'Z') b += 32;
                if (a != b) { match = 0; break; }
            }
            if (match && line[keylen] == ':') {
                const char *vs = line + keylen + 1;
                int vlen;
                while (vs < eol && *vs == ' ') vs++;
                vlen = (int)(eol - vs);
                if (vlen >= valsize) vlen = valsize - 1;
                memcpy(value, vs, vlen);
                value[vlen] = '\0';
                return 0;
            }
        }

        /* Skip to next line */
        p = eol;
        if (p < end && *p == '\r') p++;
        if (p < end && *p == '\n') p++;
    }
    return -1;
}

/* Single HTTP GET attempt. Returns HTTP status code, or -1 on error. */
static int http_get_one(const char *host, int port, const char *path,
                        const char *dest_path,
                        char *redirect_url, int redirect_url_size,
                        void (*progress)(long received, long total))
{
    struct hostent *he;
    struct sockaddr_in sa;
    int sockfd;
    char request[HTTP_HOST_MAX + HTTP_PATH_MAX + 128];
    int reqlen;
    static char recv_buf[HTTP_RECV_BUF];
    int total_hdr = 0;
    int hdr_end;
    int status;
    long content_length = -1;
    long received = 0;
    FILE *fp = NULL;
    char hdr_val[512];
    int n;
    struct timeval tv;

    /* DNS resolve */
    he = amiport_gethostbyname(host);
    if (!he) {
        fprintf(stderr, "amiport: cannot resolve %s\n", host);
        return -1;
    }

    /* Create socket */
    sockfd = amiport_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "amiport: socket() failed\n");
        return -1;
    }
    http_active_sockfd = sockfd;

    /* Set timeouts */
    tv.tv_sec = HTTP_TIMEOUT_SECS;
    tv.tv_usec = 0;
    amiport_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    amiport_setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    /* Connect */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)port);
    memcpy(&sa.sin_addr, he->h_addr, he->h_length);

    if (amiport_connect(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        fprintf(stderr, "amiport: cannot connect to %s:%d\n", host, port);
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return -1;
    }

    /* Build HTTP/1.0 request */
    reqlen = snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "User-Agent: amiport/1.0 (AmigaOS; 68k)\r\n"
        "\r\n",
        path, host);
    if (reqlen < 0 || reqlen >= (int)sizeof(request)) {
        fprintf(stderr, "amiport: request too large\n");
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return -1;
    }

    /* Send request */
    if (amiport_send(sockfd, request, reqlen, 0) != reqlen) {
        fprintf(stderr, "amiport: send failed\n");
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return -1;
    }

    /* Receive headers (may include start of body) */
    total_hdr = 0;
    hdr_end = -1;
    while (total_hdr < HTTP_RECV_BUF - 1) {
#ifdef __AMIGA__
        if (amiport_check_break()) {
            fprintf(stderr, "\namiget: interrupted\n");
            amiport_closesocket(sockfd);
            http_active_sockfd = -1;
            return -1;
        }
#endif
        n = amiport_recv(sockfd, recv_buf + total_hdr,
                         HTTP_RECV_BUF - 1 - total_hdr, 0);
        if (n <= 0) break;
        total_hdr += n;
        recv_buf[total_hdr] = '\0';

        hdr_end = find_header_end(recv_buf, total_hdr);
        if (hdr_end >= 0) break;
    }

    if (hdr_end < 0) {
        fprintf(stderr, "amiport: invalid HTTP response\n");
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return -1;
    }

    /* Parse status */
    status = parse_status_line(recv_buf);
    if (status < 0) {
        fprintf(stderr, "amiport: malformed HTTP status\n");
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return -1;
    }

    /* Handle redirects */
    if (status == 301 || status == 302) {
        if (find_header(recv_buf, hdr_end, "Location",
                        hdr_val, sizeof(hdr_val)) == 0) {
            if (strncmp(hdr_val, "https://", 8) == 0) {
                fprintf(stderr, "amiport: HTTPS not supported - "
                        "check server configuration\n");
                amiport_closesocket(sockfd);
                http_active_sockfd = -1;
                return -1;
            }
            if ((int)strlen(hdr_val) < redirect_url_size) {
                strcpy(redirect_url, hdr_val);
            }
        }
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return status;
    }

    /* Parse Content-Length */
    if (find_header(recv_buf, hdr_end, "Content-Length",
                    hdr_val, sizeof(hdr_val)) == 0) {
        content_length = 0;
        {
            const char *cp = hdr_val;
            while (*cp >= '0' && *cp <= '9') {
                content_length = content_length * 10 + (*cp - '0');
                cp++;
            }
        }
    }

    /* Check for chunked encoding (not supported) */
    if (find_header(recv_buf, hdr_end, "Transfer-Encoding",
                    hdr_val, sizeof(hdr_val)) == 0) {
        if (strstr(hdr_val, "chunked")) {
            fprintf(stderr, "amiport: chunked encoding not supported\n");
            amiport_closesocket(sockfd);
            http_active_sockfd = -1;
            return -1;
        }
    }

    /* Only write body for 200 OK */
    if (status != 200) {
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return status;
    }

    /* Open output file */
    fp = fopen(dest_path, "wb");
    if (!fp) {
        fprintf(stderr, "amiport: cannot open %s for writing\n", dest_path);
        amiport_closesocket(sockfd);
        http_active_sockfd = -1;
        return -1;
    }
    http_active_fp = fp;

    /* Write spill bytes (body data that arrived with headers) */
    if (total_hdr > hdr_end) {
        int spill = total_hdr - hdr_end;
        if (fwrite(recv_buf + hdr_end, 1, spill, fp) != (size_t)spill) {
            fprintf(stderr, "amiport: write error\n");
            fclose(fp);
            http_active_fp = NULL;
            amiport_closesocket(sockfd);
            http_active_sockfd = -1;
            return -1;
        }
        received = spill;
        if (progress) progress(received, content_length);
    }

    /* Read remaining body */
    for (;;) {
#ifdef __AMIGA__
        if (amiport_check_break()) {
            fprintf(stderr, "\namiget: interrupted\n");
            fclose(fp);
            http_active_fp = NULL;
            amiport_closesocket(sockfd);
            http_active_sockfd = -1;
            return -1;
        }
#endif
        n = amiport_recv(sockfd, recv_buf, HTTP_RECV_BUF, 0);
        if (n <= 0) break;

        if (fwrite(recv_buf, 1, n, fp) != (size_t)n) {
            fprintf(stderr, "amiport: write error\n");
            fclose(fp);
            http_active_fp = NULL;
            amiport_closesocket(sockfd);
            http_active_sockfd = -1;
            return -1;
        }
        received += n;
        if (progress) progress(received, content_length);
    }

    fclose(fp);
    http_active_fp = NULL;
    amiport_closesocket(sockfd);
    http_active_sockfd = -1;

    /* Validate Content-Length if provided */
    if (content_length >= 0 && received != content_length) {
        fprintf(stderr, "amiport: download incomplete "
                "(%ld of %ld bytes)\n", received, content_length);
        return -1;
    }

    return status;
}

int amiport_http_get(const char *url, const char *dest_path,
                     int *http_status,
                     void (*progress)(long received, long total))
{
    char host[HTTP_HOST_MAX];
    char path[HTTP_PATH_MAX];
    int port;
    char redirect_url[HTTP_HOST_MAX + HTTP_PATH_MAX + 16];
    char current_url[HTTP_HOST_MAX + HTTP_PATH_MAX + 16];
    int redirects = 0;
    int status;

    if (!http_atexit_registered) {
        atexit(http_atexit_cleanup);
        http_atexit_registered = 1;
    }

    /* Store dest path for atexit cleanup */
    if (dest_path && strlen(dest_path) < sizeof(http_active_tmppath)) {
        strcpy(http_active_tmppath, dest_path);
    }

    /* Copy URL for redirect chasing */
    if (strlen(url) >= sizeof(current_url)) {
        fprintf(stderr, "amiport: URL too long\n");
        return -1;
    }
    strcpy(current_url, url);

    while (redirects <= HTTP_MAX_REDIRECTS) {
        if (amiport_http_parse_url(current_url, host, sizeof(host),
                                    &port, path, sizeof(path)) != 0) {
            fprintf(stderr, "amiport: invalid URL: %s\n", current_url);
            return -1;
        }

        redirect_url[0] = '\0';
        status = http_get_one(host, port, path, dest_path,
                              redirect_url, sizeof(redirect_url),
                              progress);

        if (http_status) *http_status = status;

        if (status == 301 || status == 302) {
            if (redirect_url[0] == '\0') {
                fprintf(stderr, "amiport: redirect without Location\n");
                return -1;
            }
            /* Handle relative redirects */
            if (redirect_url[0] == '/') {
                if (snprintf(current_url, sizeof(current_url),
                             "http://%s:%d%s",
                             host, port, redirect_url)
                        >= (int)sizeof(current_url)) {
                    fprintf(stderr, "amiport: redirect URL too long\n");
                    return -1;
                }
            } else {
                if (strlen(redirect_url) >= sizeof(current_url)) {
                    fprintf(stderr, "amiport: redirect URL too long\n");
                    return -1;
                }
                strcpy(current_url, redirect_url);
            }
            redirects++;
            continue;
        }

        /* Clear tmppath on success so atexit doesn't delete it */
        if (status == 200) {
            http_active_tmppath[0] = '\0';
        }

        return (status == 200) ? 0 : -1;
    }

    fprintf(stderr, "amiport: too many redirects\n");
    return -1;
}
