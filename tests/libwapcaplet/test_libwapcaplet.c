/*
 * test_libwapcaplet.c -- unit tests for lib/libwapcaplet
 *
 * Test plan: 36 tests across 7 categories per docs/test-coverage-standard.md
 *   1. Functional (13 tests)
 *   2. Error path (3 tests, 1 documentation-only)
 *   3. Edge cases (6 tests)
 *   4. Reference counting lifecycle (5 tests)
 *   5. Caseless behavior (4 tests)
 *   6. Amiga-specific (2 tests)
 *   7. Stress / real-world (3 tests)
 *
 * Library built -m68040 -m68881 (matches NetSurf-Vampire consumer).
 * Run via: vamos -C 68040 -s 1024 -m 4096 ./test_libwapcaplet
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libwapcaplet/libwapcaplet.h>
#include "test_framework.h"

long __stack = 262144;
unsigned long __MEMORY_STEP = 262144;

/* ===================================================================
 * Category 1: Functional (13 tests, one per public API entry point)
 * =================================================================== */

TEST(intern_basic) {
    lwc_string *str = NULL;
    lwc_error rc = lwc_intern_string("hello", 5, &str);
    ASSERT_EQ(rc, lwc_error_ok);
    ASSERT_NOT_NULL(str);
    ASSERT_EQ(lwc_string_length(str), 5);
    ASSERT_STR_EQ(lwc_string_data(str), "hello");
    lwc_string_unref(str);
}

TEST(intern_duplicate_returns_same) {
    lwc_string *a = NULL, *b = NULL;
    ASSERT_EQ(lwc_intern_string("dupe", 4, &a), lwc_error_ok);
    ASSERT_EQ(lwc_intern_string("dupe", 4, &b), lwc_error_ok);
    ASSERT(a == b); /* same pointer because interned */
    /* refcnt should now be 2 from 2 interns */
    ASSERT_EQ(a->refcnt, 2);
    lwc_string_unref(a);
    lwc_string_unref(b);
}

TEST(string_ref_increments) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("ref", 3, &str), lwc_error_ok);
    ASSERT_EQ(str->refcnt, 1);
    (void)lwc_string_ref(str);
    ASSERT_EQ(str->refcnt, 2);
    (void)lwc_string_ref(str);
    ASSERT_EQ(str->refcnt, 3);
    lwc_string_unref(str);
    lwc_string_unref(str);
    lwc_string_unref(str);
}

TEST(string_unref_decrements) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("decr", 4, &str), lwc_error_ok);
    (void)lwc_string_ref(str);
    (void)lwc_string_ref(str);
    ASSERT_EQ(str->refcnt, 3);
    lwc_string_unref(str);
    ASSERT_EQ(str->refcnt, 2);
    lwc_string_unref(str);
    ASSERT_EQ(str->refcnt, 1);
    lwc_string_unref(str); /* destroys */
}

TEST(string_destroy_at_zero_refcnt) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("destroy", 7, &str), lwc_error_ok);
    /* unref to zero — must not crash, must not leak (verified by memory-checker) */
    lwc_string_unref(str);
    /* Re-interning the same string should now succeed cleanly (new alloc) */
    lwc_string *str2 = NULL;
    ASSERT_EQ(lwc_intern_string("destroy", 7, &str2), lwc_error_ok);
    ASSERT_NOT_NULL(str2);
    ASSERT_EQ(str2->refcnt, 1);
    lwc_string_unref(str2);
}

TEST(isequal_same_pointer) {
    lwc_string *str = NULL;
    bool match = false;
    ASSERT_EQ(lwc_intern_string("same", 4, &str), lwc_error_ok);
    (void)lwc_string_isequal(str, str, &match);
    ASSERT(match == true);
    lwc_string_unref(str);
}

TEST(isequal_different_strings) {
    lwc_string *a = NULL, *b = NULL;
    bool match = true;
    ASSERT_EQ(lwc_intern_string("foo", 3, &a), lwc_error_ok);
    ASSERT_EQ(lwc_intern_string("bar", 3, &b), lwc_error_ok);
    (void)lwc_string_isequal(a, b, &match);
    ASSERT(match == false);
    lwc_string_unref(a);
    lwc_string_unref(b);
}

TEST(caseless_isequal_match) {
    lwc_string *a = NULL, *b = NULL;
    bool match = false;
    ASSERT_EQ(lwc_intern_string("Hello", 5, &a), lwc_error_ok);
    ASSERT_EQ(lwc_intern_string("HELLO", 5, &b), lwc_error_ok);
    ASSERT_EQ(lwc_string_caseless_isequal(a, b, &match), lwc_error_ok);
    ASSERT(match == true);
    lwc_string_unref(a);
    lwc_string_unref(b);
}

TEST(string_data_returns_content) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("content", 7, &str), lwc_error_ok);
    ASSERT_STR_EQ(lwc_string_data(str), "content");
    lwc_string_unref(str);
}

TEST(string_length_returns_len) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("length", 6, &str), lwc_error_ok);
    ASSERT_EQ(lwc_string_length(str), 6);
    lwc_string_unref(str);
}

TEST(string_hash_value) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("hash", 4, &str), lwc_error_ok);
    /* FNV-1a of "hash" with seed 0x811c9dc5 — non-zero, deterministic */
    lwc_hash h = lwc_string_hash_value(str);
    ASSERT(h != 0);
    /* Same content always hashes to same value */
    ASSERT_EQ(h, lwc_string_hash_value(str));
    lwc_string_unref(str);
}

TEST(intern_substring_valid) {
    lwc_string *str = NULL, *sub = NULL;
    ASSERT_EQ(lwc_intern_string("abcdef", 6, &str), lwc_error_ok);
    ASSERT_EQ(lwc_intern_substring(str, 2, 3, &sub), lwc_error_ok);
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(lwc_string_length(sub), 3);
    ASSERT_STR_EQ(lwc_string_data(sub), "cde");
    lwc_string_unref(str);
    lwc_string_unref(sub);
}

TEST(string_tolower_basic) {
    lwc_string *str = NULL, *lower = NULL;
    ASSERT_EQ(lwc_intern_string("MiXeD", 5, &str), lwc_error_ok);
    ASSERT_EQ(lwc_string_tolower(str, &lower), lwc_error_ok);
    ASSERT_NOT_NULL(lower);
    ASSERT_STR_EQ(lwc_string_data(lower), "mixed");
    ASSERT_EQ(lwc_string_length(lower), 5);
    lwc_string_unref(str);
    lwc_string_unref(lower);
}

/* ===================================================================
 * Category 2: Error path (2 actual tests; OOM is documentation-only)
 * =================================================================== */

TEST(intern_substring_offset_out_of_range) {
    lwc_string *str = NULL, *sub = NULL;
    ASSERT_EQ(lwc_intern_string("short", 5, &str), lwc_error_ok);
    /* offset == len triggers >= check → range error */
    lwc_error rc = lwc_intern_substring(str, 5, 1, &sub);
    ASSERT_EQ(rc, lwc_error_range);
    /* offset > len also triggers */
    rc = lwc_intern_substring(str, 6, 1, &sub);
    ASSERT_EQ(rc, lwc_error_range);
    lwc_string_unref(str);
}

TEST(intern_substring_length_overflow) {
    lwc_string *str = NULL, *sub = NULL;
    ASSERT_EQ(lwc_intern_string("test", 4, &str), lwc_error_ok);
    /* offset 2 + sslen 3 = 5 > len 4 → range error */
    lwc_error rc = lwc_intern_substring(str, 2, 3, &sub);
    ASSERT_EQ(rc, lwc_error_range);
    lwc_string_unref(str);
}

/* Test 16 (lwc_error_oom) is intentionally absent: malloc failure cannot be
 * triggered on vamos without a custom allocator hook, and libwapcaplet does
 * not expose one. Upstream Check-based tests verify the OOM path; we inherit
 * that coverage. Documenting here so coverage is honest. */

/* ===================================================================
 * Category 3: Edge cases (6 tests)
 * =================================================================== */

TEST(intern_empty_string) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("", 0, &str), lwc_error_ok);
    ASSERT_NOT_NULL(str);
    ASSERT_EQ(lwc_string_length(str), 0);
    ASSERT_EQ(lwc_string_data(str)[0], '\0');
    lwc_string_unref(str);
}

TEST(intern_single_char) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("x", 1, &str), lwc_error_ok);
    ASSERT_EQ(lwc_string_length(str), 1);
    ASSERT_EQ(lwc_string_data(str)[0], 'x');
    ASSERT_EQ(lwc_string_data(str)[1], '\0'); /* null-terminated per current spec */
    lwc_string_unref(str);
}

TEST(intern_substring_zero_length) {
    lwc_string *str = NULL, *sub = NULL;
    ASSERT_EQ(lwc_intern_string("test", 4, &str), lwc_error_ok);
    ASSERT_EQ(lwc_intern_substring(str, 2, 0, &sub), lwc_error_ok);
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(lwc_string_length(sub), 0);
    lwc_string_unref(str);
    lwc_string_unref(sub);
}

TEST(intern_substring_full_string) {
    lwc_string *str = NULL, *sub = NULL;
    ASSERT_EQ(lwc_intern_string("full", 4, &str), lwc_error_ok);
    ASSERT_EQ(lwc_intern_substring(str, 0, 4, &sub), lwc_error_ok);
    ASSERT_STR_EQ(lwc_string_data(sub), "full");
    /* Substring of full content should intern to same pointer (same content) */
    ASSERT(sub == str);
    lwc_string_unref(str);
    lwc_string_unref(sub);
}

TEST(caseless_same_case_match) {
    lwc_string *a = NULL, *b = NULL;
    bool match = false;
    ASSERT_EQ(lwc_intern_string("lower", 5, &a), lwc_error_ok);
    ASSERT_EQ(lwc_intern_string("lower", 5, &b), lwc_error_ok);
    ASSERT_EQ(lwc_string_caseless_isequal(a, b, &match), lwc_error_ok);
    ASSERT(match == true);
    lwc_string_unref(a);
    lwc_string_unref(b);
}

TEST(hash_value_empty_string) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("", 0, &str), lwc_error_ok);
    /* FNV-1a seed only when len=0 (no iterations) */
    lwc_hash h = lwc_string_hash_value(str);
    ASSERT_EQ(h, (lwc_hash)0x811c9dc5);
    lwc_string_unref(str);
}

/* ===================================================================
 * Category 4: Reference counting lifecycle (5 tests)
 * =================================================================== */

TEST(unref_with_self_insensitive) {
    lwc_string *str = NULL, *lower = NULL;
    /* Intern mixed-case, get lowercase via tolower */
    ASSERT_EQ(lwc_intern_string("Test", 4, &str), lwc_error_ok);
    ASSERT_EQ(lwc_string_tolower(str, &lower), lwc_error_ok);
    /* The lowercase "test" interned via tolower is a NEW string;
     * its insensitive will point to itself once a caseless op is run on it. */
    bool match;
    ASSERT_EQ(lwc_string_caseless_isequal(lower, lower, &match), lwc_error_ok);
    ASSERT(match == true);
    ASSERT(lower->insensitive == lower); /* self-insensitive sentinel */
    /* Now unref lower: refcnt was 1 (from tolower) and insensitive==self →
     * the lwc_string_unref macro's second branch should destroy. */
    lwc_string_unref(lower);
    /* Original "Test" still alive via str->refcnt=1 */
    ASSERT_EQ(str->refcnt, 1);
    lwc_string_unref(str);
}

TEST(multiple_refs_no_destroy) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("multi", 5, &str), lwc_error_ok);
    (void)lwc_string_ref(str);
    (void)lwc_string_ref(str);
    ASSERT_EQ(str->refcnt, 3);
    lwc_string_unref(str);
    lwc_string_unref(str);
    /* refcnt now 1, str still alive */
    ASSERT_EQ(str->refcnt, 1);
    ASSERT_STR_EQ(lwc_string_data(str), "multi");
    lwc_string_unref(str); /* final destroy */
}

TEST(caseless_lazy_evaluation) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("Lazy", 4, &str), lwc_error_ok);
    /* insensitive must be NULL until first caseless op */
    ASSERT(str->insensitive == NULL);
    bool match;
    ASSERT_EQ(lwc_string_caseless_isequal(str, str, &match), lwc_error_ok);
    /* Now insensitive should be populated (points to interned "lazy") */
    ASSERT(str->insensitive != NULL);
    ASSERT_STR_EQ(lwc_string_data(str->insensitive), "lazy");
    lwc_string_unref(str);
}

TEST(caseless_reuses_insensitive) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("Reuse", 5, &str), lwc_error_ok);
    bool match;
    ASSERT_EQ(lwc_string_caseless_isequal(str, str, &match), lwc_error_ok);
    lwc_string *first_insensitive = str->insensitive;
    ASSERT_NOT_NULL(first_insensitive);
    /* Second call must NOT re-allocate — same insensitive pointer */
    ASSERT_EQ(lwc_string_caseless_isequal(str, str, &match), lwc_error_ok);
    ASSERT(str->insensitive == first_insensitive);
    lwc_string_unref(str);
}

TEST(destroy_frees_insensitive) {
    lwc_string *str = NULL, *lower = NULL;
    ASSERT_EQ(lwc_intern_string("Free", 4, &str), lwc_error_ok);
    /* tolower allocates the insensitive twin and refs it */
    ASSERT_EQ(lwc_string_tolower(str, &lower), lwc_error_ok);
    ASSERT_NOT_NULL(lower);
    ASSERT(str->insensitive != NULL);
    /* Both must be unref'd; lower first releases the tolower ref,
     * then str's destroy will unref the insensitive (per libwapcaplet.c:196-197). */
    lwc_string_unref(lower);
    lwc_string_unref(str);
    /* If we got here without crash, the cleanup chain held. */
}

/* ===================================================================
 * Category 5: Caseless behavior (4 tests)
 * =================================================================== */

TEST(caseless_hash_value_test) {
    lwc_string *str = NULL;
    lwc_hash h = 0;
    ASSERT_EQ(lwc_intern_string("CaseHash", 8, &str), lwc_error_ok);
    ASSERT_EQ(lwc_string_caseless_hash_value(str, &h), lwc_error_ok);
    ASSERT(h != 0);
    /* Must equal hash of the lowercased form */
    ASSERT_EQ(h, lwc_string_hash_value(str->insensitive));
    lwc_string_unref(str);
}

TEST(caseless_mixed_case_match) {
    lwc_string *a = NULL, *b = NULL;
    bool match = false;
    ASSERT_EQ(lwc_intern_string("MiXeD", 5, &a), lwc_error_ok);
    ASSERT_EQ(lwc_intern_string("mIxEd", 5, &b), lwc_error_ok);
    ASSERT_EQ(lwc_string_caseless_isequal(a, b, &match), lwc_error_ok);
    ASSERT(match == true);
    lwc_string_unref(a);
    lwc_string_unref(b);
}

TEST(caseless_different_content_no_match) {
    lwc_string *a = NULL, *b = NULL;
    bool match = true;
    ASSERT_EQ(lwc_intern_string("Different", 9, &a), lwc_error_ok);
    ASSERT_EQ(lwc_intern_string("Content", 7, &b), lwc_error_ok);
    ASSERT_EQ(lwc_string_caseless_isequal(a, b, &match), lwc_error_ok);
    ASSERT(match == false);
    lwc_string_unref(a);
    lwc_string_unref(b);
}

TEST(tolower_already_lowercase) {
    lwc_string *str = NULL, *lower = NULL;
    ASSERT_EQ(lwc_intern_string("lowercase", 9, &str), lwc_error_ok);
    ASSERT_EQ(lwc_string_tolower(str, &lower), lwc_error_ok);
    /* tolower of an already-lowercase string interns "lowercase" (same content)
     * which finds the existing bucket entry → same pointer. */
    ASSERT(lower == str);
    lwc_string_unref(str);
    lwc_string_unref(lower);
}

/* ===================================================================
 * Category 6: Amiga-specific (2 tests)
 * =================================================================== */

TEST(hash_collision_chain) {
    /* Intern 50 distinct short strings; statistically some will hash-collide
     * given 4091 buckets. Verify all are retrievable independently. */
    lwc_string *strs[50];
    char buf[16];
    int i;
    for (i = 0; i < 50; i++) {
        snprintf(buf, sizeof(buf), "key%d", i);
        ASSERT_EQ(lwc_intern_string(buf, strlen(buf), &strs[i]), lwc_error_ok);
        ASSERT_NOT_NULL(strs[i]);
    }
    /* Re-intern: should return same pointers */
    for (i = 0; i < 50; i++) {
        snprintf(buf, sizeof(buf), "key%d", i);
        lwc_string *check = NULL;
        ASSERT_EQ(lwc_intern_string(buf, strlen(buf), &check), lwc_error_ok);
        ASSERT(check == strs[i]);
        ASSERT_EQ(check->refcnt, 2); /* original + this lookup */
        lwc_string_unref(check);
    }
    /* Cleanup */
    for (i = 0; i < 50; i++) {
        lwc_string_unref(strs[i]);
    }
}

TEST(large_string_no_stack_overflow) {
    /* 8 KB string -- well below the 32 KB safe-allocation ceiling for
     * libnix malloc within our 256 KB stack. NetSurf's typical interned
     * URLs / class names / element-text fragments are << 1 KB but we
     * verify the larger boundary works. */
    static char buf[8192];
    lwc_string *str = NULL;
    memset(buf, 'a', sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    ASSERT_EQ(lwc_intern_string(buf, sizeof(buf) - 1, &str), lwc_error_ok);
    ASSERT_NOT_NULL(str);
    ASSERT_EQ(lwc_string_length(str), sizeof(buf) - 1);
    /* Spot-check first and last bytes */
    ASSERT_EQ(lwc_string_data(str)[0], 'a');
    ASSERT_EQ(lwc_string_data(str)[sizeof(buf) - 2], 'a');
    lwc_string_unref(str);
}

/* ===================================================================
 * Category 7: Stress / real-world (3 tests)
 * =================================================================== */

TEST(intern_1000_unique_strings) {
    /* 1000 unique strings: realistic upper bound for a small NetSurf page
     * (selectors + element names + classes + URLs interned during parse). */
    lwc_string *strs[1000];
    char buf[16];
    int i;
    for (i = 0; i < 1000; i++) {
        snprintf(buf, sizeof(buf), "str%d", i);
        ASSERT_EQ(lwc_intern_string(buf, strlen(buf), &strs[i]), lwc_error_ok);
        ASSERT_NOT_NULL(strs[i]);
    }
    /* Verify a sampling are still retrievable as same pointers */
    for (i = 0; i < 1000; i += 47) {
        snprintf(buf, sizeof(buf), "str%d", i);
        lwc_string *check = NULL;
        ASSERT_EQ(lwc_intern_string(buf, strlen(buf), &check), lwc_error_ok);
        ASSERT(check == strs[i]);
        lwc_string_unref(check);
    }
    /* Cleanup all */
    for (i = 0; i < 1000; i++) {
        lwc_string_unref(strs[i]);
    }
}

static int _iter_count = 0;
static void _iter_cb(lwc_string *str, void *pw) {
    (void)str;
    (void)pw;
    _iter_count++;
}

TEST(iterate_strings_cleanup) {
    lwc_string *str = NULL;
    ASSERT_EQ(lwc_intern_string("temp", 4, &str), lwc_error_ok);
    /* With 1 string in ctx, iterate visits it */
    _iter_count = 0;
    lwc_iterate_strings(_iter_cb, NULL);
    ASSERT(_iter_count >= 1); /* at least our temp; may be others from prior tests */
    /* Now unref to zero — string should be removed from buckets */
    lwc_string_unref(str);
    /* Iterate again — when zero strings remain, ctx is freed (libwapcaplet.c:286-291).
     * We can't directly observe ctx==NULL, but we verify that a subsequent intern
     * succeeds (proves ctx was either freed-and-re-init'd OR still alive — either way
     * the API contract holds). */
    _iter_count = 0;
    lwc_iterate_strings(_iter_cb, NULL);
    /* (Other prior tests may have leaked strings; iter count may be > 0)
     * The key behavioral check is the post-iterate intern works: */
    lwc_string *str2 = NULL;
    ASSERT_EQ(lwc_intern_string("after-iterate", 13, &str2), lwc_error_ok);
    ASSERT_NOT_NULL(str2);
    lwc_string_unref(str2);
}

TEST(substring_chain) {
    lwc_string *str = NULL, *sub1 = NULL, *sub2 = NULL;
    ASSERT_EQ(lwc_intern_string("abcdefgh", 8, &str), lwc_error_ok);
    /* "cdef" */
    ASSERT_EQ(lwc_intern_substring(str, 2, 4, &sub1), lwc_error_ok);
    ASSERT_STR_EQ(lwc_string_data(sub1), "cdef");
    ASSERT_EQ(lwc_string_length(sub1), 4);
    /* "de" from sub1 */
    ASSERT_EQ(lwc_intern_substring(sub1, 1, 2, &sub2), lwc_error_ok);
    ASSERT_STR_EQ(lwc_string_data(sub2), "de");
    ASSERT_EQ(lwc_string_length(sub2), 2);
    lwc_string_unref(str);
    lwc_string_unref(sub1);
    lwc_string_unref(sub2);
}

/* ===================================================================
 * Self-insensitive destruction path (added 2026-05-02)
 *
 * The Stage 6 memory-checker audit raised a concern that the
 * destroy-via-macro-condition-2 path (refcnt==1 && insensitive==self)
 * could recurse into itself and cause a refcnt underflow. Source
 * trace shows lwc_string_destroy() correctly gates the recursive
 * unref via `str->insensitive != NULL && str->refcnt == 0`, which is
 * FALSE when entering from condition 2 (refcnt is now 1, not 0).
 * This test exercises the exact scenario directly to empirically
 * confirm no crash and no UAF on the destroy.
 * =================================================================== */

TEST(self_insensitive_destroy_path) {
    /* Intern an already-lowercase string so the caseless intern of itself
     * produces a self-loop insensitive. */
    lwc_string *s = NULL;
    ASSERT_EQ(lwc_intern_string("self", 4, &s), lwc_error_ok);
    /* refcnt = 1, insensitive = NULL */
    ASSERT_EQ(s->refcnt, 1);
    ASSERT(s->insensitive == NULL);

    /* Trigger the lazy caseless init. lwc__intern_caseless_string("self") finds
     * "self" in the bucket (lcase("self") == "self"), increments refcnt, sets
     * insensitive = self. */
    bool match = false;
    ASSERT_EQ(lwc_string_caseless_isequal(s, s, &match), lwc_error_ok);
    ASSERT(match == true);
    ASSERT(s->insensitive == s); /* the self-loop sentinel */
    ASSERT_EQ(s->refcnt, 2);     /* original + caseless intern bump */

    /* Now unref. The macro decrements refcnt 2->1, then condition 2 fires
     * (refcnt==1 && insensitive==self) -> destroy(s). Inside destroy, the
     * gate `str->refcnt == 0` is FALSE (refcnt is 1), so the recursive
     * unref(insensitive=self) is correctly SKIPPED. The struct is freed
     * exactly once with no underflow. */
    lwc_string_unref(s);

    /* If we got here, no crash. To verify no UAF, intern the same content
     * again -- if the struct was freed cleanly (vs being marked free but
     * left in the bucket), the new intern allocates fresh. */
    lwc_string *s2 = NULL;
    ASSERT_EQ(lwc_intern_string("self", 4, &s2), lwc_error_ok);
    ASSERT_NOT_NULL(s2);
    ASSERT_EQ(s2->refcnt, 1);             /* fresh allocation */
    ASSERT(s2->insensitive == NULL);      /* lazy caseless still NULL */
    lwc_string_unref(s2);
}

/* ===================================================================
 * Test runner
 * =================================================================== */

int main(void) {
    printf("=== lib/libwapcaplet unit tests ===\n");

    /* Category 1: Functional */
    RUN_TEST(intern_basic);
    RUN_TEST(intern_duplicate_returns_same);
    RUN_TEST(string_ref_increments);
    RUN_TEST(string_unref_decrements);
    RUN_TEST(string_destroy_at_zero_refcnt);
    RUN_TEST(isequal_same_pointer);
    RUN_TEST(isequal_different_strings);
    RUN_TEST(caseless_isequal_match);
    RUN_TEST(string_data_returns_content);
    RUN_TEST(string_length_returns_len);
    RUN_TEST(string_hash_value);
    RUN_TEST(intern_substring_valid);
    RUN_TEST(string_tolower_basic);

    /* Category 2: Error path */
    RUN_TEST(intern_substring_offset_out_of_range);
    RUN_TEST(intern_substring_length_overflow);

    /* Category 3: Edge cases */
    RUN_TEST(intern_empty_string);
    RUN_TEST(intern_single_char);
    RUN_TEST(intern_substring_zero_length);
    RUN_TEST(intern_substring_full_string);
    RUN_TEST(caseless_same_case_match);
    RUN_TEST(hash_value_empty_string);

    /* Category 4: Reference counting lifecycle */
    RUN_TEST(unref_with_self_insensitive);
    RUN_TEST(multiple_refs_no_destroy);
    RUN_TEST(caseless_lazy_evaluation);
    RUN_TEST(caseless_reuses_insensitive);
    RUN_TEST(destroy_frees_insensitive);

    /* Category 5: Caseless behavior */
    RUN_TEST(caseless_hash_value_test);
    RUN_TEST(caseless_mixed_case_match);
    RUN_TEST(caseless_different_content_no_match);
    RUN_TEST(tolower_already_lowercase);

    /* Category 6: Amiga-specific */
    RUN_TEST(hash_collision_chain);
    RUN_TEST(large_string_no_stack_overflow);

    /* Category 7: Stress / real-world */
    RUN_TEST(intern_1000_unique_strings);
    RUN_TEST(iterate_strings_cleanup);
    RUN_TEST(substring_chain);

    /* Memory-checker audit verification (added 2026-05-02) */
    RUN_TEST(self_insensitive_destroy_path);

    /* Final cleanup — release the library global context if no strings remain */
    lwc_iterate_strings(_iter_cb, NULL);

    return test_summary();
}
