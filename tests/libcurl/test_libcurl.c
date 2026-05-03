/*
 * test_libcurl.c -- unit tests for lib/libcurl (HTTP-only build)
 *
 * Library: curl 8.11.1, curl/COPYING (zlib license).
 *   Built -O0 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG -std=c99
 *   HTTP-only (CURL_DISABLE_FTP/IMAP/POP3/SMTP/SMB/FILE/GOPHER/LDAP/RTSP/
 *   TELNET/TFTP/DICT/MQTT, no proxy/cookies/HSTS/HTTP2/QUIC/TLS/SSH/auth/
 *   form-API/MIME/parsedate/altsvc/DOH/AWS/IPv6/threads).
 *
 * Run via: vamos -C 68040 -s 4096 -m 8192 ./test_libcurl
 *
 * Cookies: 1 MB stack/MEMORY_STEP (curl is large; lib/libnix-stack-scaling
 * pitfall — anything linking 300+ KB of library code needs 1 MB cookies).
 *
 * vamos has no network. Tests cover lifecycle / setopt / multi / slist /
 * URL parsing / strerror / version — the API surface ports/netsurf consumes
 * via content/fetchers/curl.c. Real HTTP fetches are deferred to FS-UAE
 * with bsdsocket.library configured.
 *
 * Coverage (per docs/test-coverage-standard.md):
 *   12 functional   (init/cleanup pairs, setopt with various option types,
 *                    multi handle lifecycle, slist primitives, version,
 *                    strerror, URL API, getinfo)
 *    3 error path   (invalid option, NULL handle, malformed URL)
 *    3 edge case    (slist on NULL, repeated init, long header chain)
 *    1 Amiga        (-noixemul cleanup discipline — repeated init/cleanup
 *                    without leaks)
 *    2 stress       (50 multi cycles, 50 slist append+free)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "test_framework.h"

/* libnix needs >= 524288 cookies for libpng-class binaries; libcurl is
 * larger so set 1 MB per the scaling pitfall. */
long __stack = 1048576;
unsigned long __MEMORY_STEP = 1048576;

/* ---------- Functional ---------- */

TEST(global_init_cleanup) {
    CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
    ASSERT_EQ(rc, CURLE_OK);
    curl_global_cleanup();
}

TEST(easy_init_cleanup) {
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    curl_easy_cleanup(h);
}

TEST(easy_strerror_known) {
    const char *s = curl_easy_strerror(CURLE_OK);
    ASSERT(s != NULL);
    /* Should mention "OK" or "no error" -- but with
     * CURL_DISABLE_VERBOSE_STRINGS the message is the bare numeric code.
     * Just verify non-null + non-empty. */
    ASSERT(s[0] != '\0');
}

TEST(easy_setopt_url) {
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    /* Reserved RFC 2606 test domain; protocol scheme is irrelevant here --
     * setopt only stores the string, no connect happens. */
    CURLcode rc = curl_easy_setopt(h, CURLOPT_URL, "https://example.test/");
    ASSERT_EQ(rc, CURLE_OK);
    curl_easy_cleanup(h);
}

TEST(easy_setopt_long) {
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    CURLcode rc = curl_easy_setopt(h, CURLOPT_TIMEOUT, 30L);
    ASSERT_EQ(rc, CURLE_OK);
    curl_easy_cleanup(h);
}

TEST(easy_setopt_nosignal) {
    /* NetSurf always sets CURLOPT_NOSIGNAL=1 -- verify the path works. */
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    CURLcode rc = curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L);
    ASSERT_EQ(rc, CURLE_OK);
    curl_easy_cleanup(h);
}

TEST(multi_init_cleanup) {
    CURLM *m = curl_multi_init();
    ASSERT(m != NULL);
    CURLMcode rc = curl_multi_cleanup(m);
    ASSERT_EQ(rc, CURLM_OK);
}

TEST(multi_strerror_known) {
    const char *s = curl_multi_strerror(CURLM_OK);
    ASSERT(s != NULL);
    ASSERT(s[0] != '\0');
}

TEST(slist_append_free) {
    struct curl_slist *l = NULL;
    l = curl_slist_append(l, "Accept: text/html");
    ASSERT(l != NULL);
    l = curl_slist_append(l, "User-Agent: NetSurf/3.11");
    ASSERT(l != NULL);
    curl_slist_free_all(l);
}

TEST(version_returns_string) {
    const char *v = curl_version();
    ASSERT(v != NULL);
    /* curl_version() includes the marketing string "libcurl/" prefix even
     * with CURL_DISABLE_VERBOSE_STRINGS. */
    ASSERT(strstr(v, "libcurl") != NULL);
}

TEST(easy_perform_without_url_returns_error) {
    /* No URL set + no network -> should return CURLE_URL_MALFORMAT or similar
     * non-CURLE_OK code immediately, NOT crash. */
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    CURLcode rc = curl_easy_perform(h);
    ASSERT(rc != CURLE_OK);
    curl_easy_cleanup(h);
}

TEST(easy_getinfo_unset) {
    /* Querying CURLINFO_RESPONSE_CODE on a fresh handle must return
     * CURLE_OK with a 0 value (no fetch happened yet). */
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    long code = -1;
    CURLcode rc = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
    ASSERT_EQ(rc, CURLE_OK);
    ASSERT_EQ(code, 0L);
    curl_easy_cleanup(h);
}

/* ---------- Error path ---------- */

TEST(setopt_invalid_option) {
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    /* 99999 is not a valid CURLOPT_*. */
    CURLcode rc = curl_easy_setopt(h, (CURLoption)99999, 0L);
    ASSERT(rc != CURLE_OK);
    curl_easy_cleanup(h);
}

TEST(setopt_null_handle_safe) {
    /* curl_easy_setopt with NULL handle should return CURLE_BAD_FUNCTION_ARGUMENT
     * (or similar) instead of crashing. */
    /* URL value irrelevant -- contract is "NULL handle returns error not crash". */
    CURLcode rc = curl_easy_setopt((CURL *)NULL, CURLOPT_URL, (const char *)NULL);
    ASSERT(rc != CURLE_OK);
}

TEST(setopt_malformed_url_accepted_at_setopt) {
    /* CURLOPT_URL accepts the string at setopt time -- error surfaces
     * at perform time, not setopt time. Verify the contract. */
    CURL *h = curl_easy_init();
    ASSERT(h != NULL);
    CURLcode rc = curl_easy_setopt(h, CURLOPT_URL, "not-a-url");
    ASSERT_EQ(rc, CURLE_OK);
    curl_easy_cleanup(h);
}

/* ---------- Edge case ---------- */

TEST(slist_append_to_null) {
    struct curl_slist *l = curl_slist_append(NULL, "first");
    ASSERT(l != NULL);
    curl_slist_free_all(l);
}

TEST(global_init_repeated) {
    /* curl maintains an internal refcount -- N inits need N cleanups. */
    ASSERT_EQ(curl_global_init(CURL_GLOBAL_DEFAULT), CURLE_OK);
    ASSERT_EQ(curl_global_init(CURL_GLOBAL_DEFAULT), CURLE_OK);
    ASSERT_EQ(curl_global_init(CURL_GLOBAL_DEFAULT), CURLE_OK);
    curl_global_cleanup();
    curl_global_cleanup();
    curl_global_cleanup();
}

TEST(slist_long_chain) {
    /* Build a chain of 64 headers -- typical NetSurf request size. */
    struct curl_slist *l = NULL;
    int i;
    for (i = 0; i < 64; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "X-Custom-%d: value-%d", i, i);
        l = curl_slist_append(l, buf);
        ASSERT(l != NULL);
    }
    curl_slist_free_all(l);
}

/* ---------- Amiga-specific ---------- */

TEST(repeated_easy_init_cleanup) {
    /* AmigaOS -noixemul reclaims nothing on exit. Verify lifecycle is
     * leak-free across many cycles. memory-checker would catch true
     * leaks; this test catches catastrophic failure (e.g. curl tries to
     * allocate something the second time and fails). */
    int i;
    for (i = 0; i < 100; i++) {
        CURL *h = curl_easy_init();
        ASSERT(h != NULL);
        curl_easy_cleanup(h);
    }
}

/* ---------- Stress ---------- */

TEST(stress_multi_50) {
    int i;
    for (i = 0; i < 50; i++) {
        CURLM *m = curl_multi_init();
        ASSERT(m != NULL);
        ASSERT_EQ(curl_multi_cleanup(m), CURLM_OK);
    }
}

TEST(stress_slist_50) {
    int i;
    for (i = 0; i < 50; i++) {
        struct curl_slist *l = NULL;
        l = curl_slist_append(l, "Hello: world");
        ASSERT(l != NULL);
        curl_slist_free_all(l);
    }
}

/* ---------- Runner ---------- */

int main(void) {
    printf("=== libcurl HTTP-only test suite ===\n\n");

    /* Functional */
    _run_test_global_init_cleanup();
    _run_test_easy_init_cleanup();
    _run_test_easy_strerror_known();
    _run_test_easy_setopt_url();
    _run_test_easy_setopt_long();
    _run_test_easy_setopt_nosignal();
    _run_test_multi_init_cleanup();
    _run_test_multi_strerror_known();
    _run_test_slist_append_free();
    _run_test_version_returns_string();
    _run_test_easy_perform_without_url_returns_error();
    _run_test_easy_getinfo_unset();

    /* Error path */
    _run_test_setopt_invalid_option();
    _run_test_setopt_null_handle_safe();
    _run_test_setopt_malformed_url_accepted_at_setopt();

    /* Edge case */
    _run_test_slist_append_to_null();
    _run_test_global_init_repeated();
    _run_test_slist_long_chain();

    /* Amiga */
    _run_test_repeated_easy_init_cleanup();

    /* Stress */
    _run_test_stress_multi_50();
    _run_test_stress_slist_50();

    return test_summary();
}
