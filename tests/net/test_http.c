/*
 * test_http.c -- Unit tests for http-shim URL parser
 *
 * Tests amiport_http_parse_url() with various URL formats.
 * Runs on vamos (no network required).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* amiport: include http-shim header */
#include <amiport-net/http.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  %s... ", name); \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf("OK\n"); \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
} while(0)

static void test_basic_url(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("basic http URL");
    rc = amiport_http_parse_url("http://example.com/foo/bar",
                                 host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != 0) { FAIL("returned error"); return; }
    if (strcmp(host, "example.com") != 0) { FAIL("wrong host"); return; }
    if (port != 80) { FAIL("wrong port"); return; }
    if (strcmp(path, "/foo/bar") != 0) { FAIL("wrong path"); return; }
    PASS();
}

static void test_url_with_port(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("URL with explicit port");
    rc = amiport_http_parse_url("http://example.com:8080/api/v1",
                                 host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != 0) { FAIL("returned error"); return; }
    if (strcmp(host, "example.com") != 0) { FAIL("wrong host"); return; }
    if (port != 8080) { FAIL("wrong port"); return; }
    if (strcmp(path, "/api/v1") != 0) { FAIL("wrong path"); return; }
    PASS();
}

static void test_url_no_path(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("URL with no path");
    rc = amiport_http_parse_url("http://example.com",
                                 host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != 0) { FAIL("returned error"); return; }
    if (strcmp(host, "example.com") != 0) { FAIL("wrong host"); return; }
    if (port != 80) { FAIL("wrong port"); return; }
    if (strcmp(path, "/") != 0) { FAIL("wrong path"); return; }
    PASS();
}

static void test_https_rejected(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("https URL rejected");
    rc = amiport_http_parse_url("https://example.com/secure",
                                 host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != -1) { FAIL("should reject https"); return; }
    PASS();
}

static void test_malformed_no_scheme(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("malformed URL (no scheme)");
    rc = amiport_http_parse_url("example.com/foo",
                                 host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != -1) { FAIL("should reject"); return; }
    PASS();
}

static void test_empty_host(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("empty host");
    rc = amiport_http_parse_url("http:///path",
                                 host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != -1) { FAIL("should reject empty host"); return; }
    PASS();
}

static void test_oversized_host(void)
{
    char url[300];
    char host[16]; /* deliberately small */
    char path[256];
    int port;
    int rc;

    TEST("oversized host");
    strcpy(url, "http://");
    memset(url + 7, 'a', 200);
    url[207] = '/';
    url[208] = '\0';

    rc = amiport_http_parse_url(url, host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != -1) { FAIL("should reject oversized host"); return; }
    PASS();
}

static void test_oversized_path(void)
{
    char url[600];
    char host[128];
    char path[16]; /* deliberately small */
    int port;
    int rc;

    TEST("oversized path");
    strcpy(url, "http://example.com/");
    memset(url + 19, 'x', 400);
    url[419] = '\0';

    rc = amiport_http_parse_url(url, host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != -1) { FAIL("should reject oversized path"); return; }
    PASS();
}

static void test_null_inputs(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("NULL URL");
    rc = amiport_http_parse_url(NULL, host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != -1) { FAIL("should reject NULL"); return; }
    PASS();
}

static void test_port_zero(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("port 0 rejected");
    rc = amiport_http_parse_url("http://example.com:0/path",
                                 host, sizeof(host),
                                 &port, path, sizeof(path));
    if (rc != -1) { FAIL("should reject port 0"); return; }
    PASS();
}

static void test_amiport_url(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("real amiport URL");
    rc = amiport_http_parse_url(
        "http://amiport.platesteel.net/api/v1/packages.php",
        host, sizeof(host), &port, path, sizeof(path));
    if (rc != 0) { FAIL("returned error"); return; }
    if (strcmp(host, "amiport.platesteel.net") != 0) { FAIL("wrong host"); return; }
    if (port != 80) { FAIL("wrong port"); return; }
    if (strcmp(path, "/api/v1/packages.php") != 0) { FAIL("wrong path"); return; }
    PASS();
}

static void test_query_string(void)
{
    char host[128];
    char path[256];
    int port;
    int rc;

    TEST("URL with query string");
    rc = amiport_http_parse_url(
        "http://amiport.platesteel.net/api/v1/download.php?name=grep&format=machine",
        host, sizeof(host), &port, path, sizeof(path));
    if (rc != 0) { FAIL("returned error"); return; }
    if (strcmp(path, "/api/v1/download.php?name=grep&format=machine") != 0) {
        FAIL("wrong path");
        return;
    }
    PASS();
}

int main(void)
{
    printf("=== http-shim URL parser tests ===\n");

    test_basic_url();
    test_url_with_port();
    test_url_no_path();
    test_https_rejected();
    test_malformed_no_scheme();
    test_empty_host();
    test_oversized_host();
    test_oversized_path();
    test_null_inputs();
    test_port_zero();
    test_amiport_url();
    test_query_string();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 10;
}
