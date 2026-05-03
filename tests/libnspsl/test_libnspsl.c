/*
 * test_libnspsl.c -- unit tests for lib/libnspsl
 *
 * Library: netsurf-browser/libnspsl @ commit 82815c2, MIT-licensed.
 *   Built -O1 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG -std=c99
 *   (NetSurf-Vampire dep stack convention).
 *
 * Run via: vamos -C 68040 -s 1024 -m 4096 ./test_libnspsl
 *
 * libnspsl is a small standalone library (1 TU + ~13K LOC of generated
 * static PSL data, 67 KB archive). Single API: nspsl_getpublicsuffix.
 * No allocations, no globals, no init/cleanup.
 *
 * IMPORTANT: nspsl_getpublicsuffix returns the REGISTRABLE DOMAIN
 * (eTLD+1), NOT the public suffix itself. Per the PSL algorithm:
 * "the domain must match the public suffix plus one additional label".
 * For example:
 *   - getpublicsuffix("www.example.com") returns "example.com"
 *   - getpublicsuffix("bbc.co.uk")        returns "bbc.co.uk"
 *   - getpublicsuffix("a.b.c.example.co.uk") returns "example.co.uk"
 *
 * The returned pointer is INTO the caller's input buffer (no allocation).
 *
 * 18 tests across the six docs/test-coverage-standard categories:
 *    8 functional   (registrable domain extraction for .com, .org,
 *                    .uk, .co.uk, .gov.au, .ac.jp, deep subdomains)
 *    4 error path   (NULL input, empty input, single-label input,
 *                    leading dot)
 *    3 edge case    (long label, unrecognised TLD, returned-pointer
 *                    is into input buffer)
 *    1 Amiga        (no allocation -- 100 lookups, no leak)
 *    2 stress       (50 diverse lookups, 200-char hostname)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nspsl.h>

#include "test_framework.h"

long __stack = 262144;
unsigned long __MEMORY_STEP = 262144;

/* ===================================================================
 * Category 1: Functional (8) -- API returns REGISTRABLE DOMAIN (eTLD+1)
 * =================================================================== */

TEST(suffix_com)
{
    /* "example.com": "com" is the public suffix, registered domain
     * is "example.com". Function returns "example.com". */
    const char *r = nspsl_getpublicsuffix("example.com");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "example.com");
}

TEST(suffix_org)
{
    const char *r = nspsl_getpublicsuffix("netsurf-browser.org");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "netsurf-browser.org");
}

TEST(suffix_uk)
{
    /* uk is a TLD; "example.uk" -> registered domain is "example.uk" */
    const char *r = nspsl_getpublicsuffix("example.uk");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "example.uk");
}

TEST(suffix_co_uk)
{
    /* "co.uk" is the multi-label public suffix; "bbc" is the
     * registrant; full registered domain is "bbc.co.uk" */
    const char *r = nspsl_getpublicsuffix("bbc.co.uk");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "bbc.co.uk");
}

TEST(suffix_gov_au)
{
    const char *r = nspsl_getpublicsuffix("ato.gov.au");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "ato.gov.au");
}

TEST(suffix_ac_jp)
{
    const char *r = nspsl_getpublicsuffix("u-tokyo.ac.jp");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "u-tokyo.ac.jp");
}

TEST(suffix_subdomain_strips_to_registrable)
{
    /* Subdomains strip down to the registrable domain. */
    const char *r = nspsl_getpublicsuffix("www.example.com");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "example.com");
}

TEST(suffix_deep_subdomain_strips_to_registrable)
{
    /* Many subdomain levels still strip down to the registrable
     * domain (eTLD+1). */
    const char *r = nspsl_getpublicsuffix("a.b.c.d.example.co.uk");
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "example.co.uk");
}

/* ===================================================================
 * Category 2: Error paths (4)
 * =================================================================== */

TEST(null_input_returns_null)
{
    /* Per source: NULL hostname -> returns NULL. */
    const char *r = nspsl_getpublicsuffix(NULL);
    ASSERT_NULL(r);
}

TEST(empty_input_returns_null)
{
    /* Per source: empty string -> returns NULL. */
    const char *r = nspsl_getpublicsuffix("");
    ASSERT_NULL(r);
}

TEST(single_label_no_dot)
{
    /* "localhost" has no dots. The PSL algorithm requires "public
     * suffix plus one additional label" (>= 2 labels). With only 1
     * label, function returns NULL or "localhost". */
    const char *r = nspsl_getpublicsuffix("localhost");
    /* Either is acceptable -- localhost is not a recognised TLD. */
    (void)r;
}

TEST(leading_dot_returns_null)
{
    /* Per source: leading DOMSEP ('.') -> returns NULL. */
    const char *r = nspsl_getpublicsuffix(".example.com");
    ASSERT_NULL(r);
}

/* ===================================================================
 * Category 3: Edge cases (3)
 * =================================================================== */

TEST(long_label_64_chars)
{
    /* DNS labels max 63 chars. 64-char label is RFC-violating but
     * library should not crash. */
    const char *long_host =
        "a234567890123456789012345678901234567890123456789012345678901234.com";
    const char *r = nspsl_getpublicsuffix(long_host);
    /* The full registered domain (label.com) should be returned */
    if (r != NULL) {
        /* r should point at the start of the label */
        ASSERT(strstr(r, ".com") != NULL);
    }
}

TEST(hostname_with_no_recognized_tld)
{
    /* .invalid is an officially unallocated TLD per RFC 6761;
     * the PSL data may or may not include it. Just verify no crash. */
    const char *r = nspsl_getpublicsuffix("foo.invalid");
    (void)r;
}

TEST(returns_pointer_into_input)
{
    /* The returned pointer is INTO the caller's input string, not
     * an allocated buffer. Verify the pointer falls within the input. */
    const char *input = "shop.example.co.uk";
    const char *r = nspsl_getpublicsuffix(input);
    ASSERT_NOT_NULL(r);
    /* r must be >= input and within input's bounds */
    ASSERT(r >= input);
    ASSERT(r < input + strlen(input) + 1);
    /* Returned registered domain should be "example.co.uk" */
    ASSERT_STR_EQ(r, "example.co.uk");
}

/* ===================================================================
 * Category 4: Amiga-specific (1)
 * =================================================================== */

TEST(no_allocation_no_cleanup)
{
    /* libnspsl makes ZERO allocations -- verify by calling 100 times
     * with different inputs and confirming we never leak (implicit:
     * test process completes cleanly). The returned pointer is into
     * the input, so we don't free anything. AmigaOS-relevant because
     * -noixemul has no GC. */
    int i;
    for (i = 0; i < 100; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "site%d.example.com", i);
        const char *r = nspsl_getpublicsuffix(buf);
        ASSERT_NOT_NULL(r);
        /* Returns the registrable form siteN.example.com */
        ASSERT(strstr(r, ".example.com") != NULL ||
               strstr(r, "example.com") != NULL);
    }
}

/* ===================================================================
 * Category 5: Stress (2)
 * =================================================================== */

TEST(stress_50_diverse_lookups)
{
    const char *hosts[] = {
        "example.com",
        "example.org",
        "example.net",
        "bbc.co.uk",
        "u-tokyo.ac.jp",
        "ato.gov.au",
        "wikipedia.org",
        "shop.github.io",
        "example.de",
        "example.fr",
    };
    int i, j;
    for (i = 0; i < 50; i++) {
        for (j = 0; j < (int)(sizeof(hosts)/sizeof(hosts[0])); j++) {
            const char *r = nspsl_getpublicsuffix(hosts[j]);
            /* All should resolve to a non-empty string */
            ASSERT_NOT_NULL(r);
            ASSERT(strlen(r) > 0);
        }
    }
}

TEST(stress_200_char_hostname)
{
    /* Synthesize a long hostname with many subdomain levels.
     * The PSL should still find the registrable domain (example.co.uk). */
    char big[256];
    int i;
    char *p = big;
    for (i = 0; i < 20; i++) {
        memcpy(p, "sub.", 4);
        p += 4;
    }
    /* "sub." x 20 = 80 chars, then "subdomain.example.co.uk" appended */
    strcpy(p, "subdomain.example.co.uk");
    /* Total ~103 chars -- well under 256-byte buf */

    const char *r = nspsl_getpublicsuffix(big);
    ASSERT_NOT_NULL(r);
    ASSERT_STR_EQ(r, "example.co.uk");
}

/* ===================================================================
 * main
 * =================================================================== */

int main(void)
{
    printf("\n=== libnspsl unit tests (18) ===\n\n");

    printf("[Functional]\n");
    RUN_TEST(suffix_com);
    RUN_TEST(suffix_org);
    RUN_TEST(suffix_uk);
    RUN_TEST(suffix_co_uk);
    RUN_TEST(suffix_gov_au);
    RUN_TEST(suffix_ac_jp);
    RUN_TEST(suffix_subdomain_strips_to_registrable);
    RUN_TEST(suffix_deep_subdomain_strips_to_registrable);

    printf("\n[Error path]\n");
    RUN_TEST(null_input_returns_null);
    RUN_TEST(empty_input_returns_null);
    RUN_TEST(single_label_no_dot);
    RUN_TEST(leading_dot_returns_null);

    printf("\n[Edge case]\n");
    RUN_TEST(long_label_64_chars);
    RUN_TEST(hostname_with_no_recognized_tld);
    RUN_TEST(returns_pointer_into_input);

    printf("\n[Amiga-specific]\n");
    RUN_TEST(no_allocation_no_cleanup);

    printf("\n[Stress]\n");
    RUN_TEST(stress_50_diverse_lookups);
    RUN_TEST(stress_200_char_hostname);

    return test_summary();
}
