/*
 * test_framework.h — Minimal test framework for amiport shim tests
 *
 * Cross-compiles for AmigaOS and runs via vamos.
 * Usage:
 *   TEST(name) { ... ASSERT(cond); ASSERT_EQ(a, b); ... }
 *   int main(void) { RUN_ALL_TESTS(); return test_summary(); }
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

static int _tests_run = 0;
static int _tests_passed = 0;
static int _tests_failed = 0;
static const char *_current_test = "";

#define TEST(name) \
    static void test_##name(void); \
    static void _run_test_##name(void) { \
        _current_test = #name; \
        _tests_run++; \
        test_##name(); \
    } \
    static void test_##name(void)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("  FAIL: %s — %s (line %d)\n", _current_test, #cond, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("  FAIL: %s — %s != %s (got %ld vs %ld, line %d)\n", \
               _current_test, #a, #b, (long)(a), (long)(b), __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("  FAIL: %s — \"%s\" != \"%s\" (line %d)\n", \
               _current_test, (a), (b), __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("  FAIL: %s — %s is NULL (line %d)\n", \
               _current_test, #ptr, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("  FAIL: %s — %s is not NULL (line %d)\n", \
               _current_test, #ptr, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define RUN_TEST(name) _run_test_##name()

/* Call this after all RUN_TEST calls to mark passed tests */
static int test_summary(void)
{
    _tests_passed = _tests_run - _tests_failed;
    printf("\n%d/%d tests passed", _tests_passed, _tests_run);
    if (_tests_failed > 0) {
        printf(", %d FAILED", _tests_failed);
    }
    printf("\n");
    return _tests_failed > 0 ? 1 : 0;
}

#endif /* TEST_FRAMEWORK_H */
