/*
 * test_string_bsd.c — Tests for amiport BSD string functions
 *                     (strlcpy, strlcat, reallocarray, recallocarray,
 *                      strcasecmp, strncasecmp, strcasestr)
 */

#include "test_framework.h"
#define AMIPORT_NO_STRING_MACROS
#include <amiport/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *verstag = "$VER: test_string_bsd 1.0 (20.03.2026)";
long __stack = 32768;

/* --- strlcpy tests --- */

TEST(strlcpy_basic)
{
    char dst[16];
    size_t ret;

    ret = amiport_strlcpy(dst, "hello", sizeof(dst));
    ASSERT_EQ(ret, 5);
    ASSERT_STR_EQ(dst, "hello");
}

TEST(strlcpy_truncation)
{
    char dst[4];
    size_t ret;

    ret = amiport_strlcpy(dst, "hello world", sizeof(dst));
    /* Returns strlen(src) so truncation can be detected */
    ASSERT_EQ(ret, 11);
    ASSERT_STR_EQ(dst, "hel");
}

TEST(strlcpy_exact_fit)
{
    char dst[6];
    size_t ret;

    ret = amiport_strlcpy(dst, "hello", sizeof(dst));
    ASSERT_EQ(ret, 5);
    ASSERT_STR_EQ(dst, "hello");
}

TEST(strlcpy_empty_src)
{
    char dst[8];
    size_t ret;

    memset(dst, 'X', sizeof(dst));
    ret = amiport_strlcpy(dst, "", sizeof(dst));
    ASSERT_EQ(ret, 0);
    ASSERT_STR_EQ(dst, "");
}

TEST(strlcpy_zero_dst_size)
{
    char dst[4] = "old";
    size_t ret;

    ret = amiport_strlcpy(dst, "new", 0);
    ASSERT_EQ(ret, 3);
    /* dst should be unchanged */
    ASSERT_STR_EQ(dst, "old");
}

/* --- strlcat tests --- */

TEST(strlcat_basic)
{
    char dst[16] = "hello";
    size_t ret;

    ret = amiport_strlcat(dst, " world", sizeof(dst));
    ASSERT_EQ(ret, 11);
    ASSERT_STR_EQ(dst, "hello world");
}

TEST(strlcat_truncation)
{
    char dst[10] = "hello";
    size_t ret;

    ret = amiport_strlcat(dst, " world!!", sizeof(dst));
    /* Returns dst_len + src_len */
    ASSERT_EQ(ret, 13);
    ASSERT_STR_EQ(dst, "hello wor");
}

TEST(strlcat_empty_dst)
{
    char dst[8] = "";
    size_t ret;

    ret = amiport_strlcat(dst, "hello", sizeof(dst));
    ASSERT_EQ(ret, 5);
    ASSERT_STR_EQ(dst, "hello");
}

TEST(strlcat_empty_src)
{
    char dst[8] = "hello";
    size_t ret;

    ret = amiport_strlcat(dst, "", sizeof(dst));
    ASSERT_EQ(ret, 5);
    ASSERT_STR_EQ(dst, "hello");
}

/* --- reallocarray tests --- */

TEST(reallocarray_basic)
{
    int *arr;

    arr = (int *)amiport_reallocarray(NULL, 10, sizeof(int));
    ASSERT_NOT_NULL(arr);

    arr[0] = 42;
    arr[9] = 99;
    ASSERT_EQ(arr[0], 42);
    ASSERT_EQ(arr[9], 99);

    free(arr);
}

TEST(reallocarray_overflow)
{
    void *p;

    /* This should overflow and return NULL */
    p = amiport_reallocarray(NULL, (size_t)-1, (size_t)-1);
    ASSERT_NULL(p);
}

TEST(reallocarray_zero)
{
    void *p;

    /* Zero elements should succeed (implementation-defined) */
    p = amiport_reallocarray(NULL, 0, sizeof(int));
    /* realloc(NULL, 0) may return NULL or a unique pointer */
    /* Just verify no crash */
    free(p);
}

TEST(reallocarray_resize)
{
    int *arr;

    arr = (int *)amiport_reallocarray(NULL, 5, sizeof(int));
    ASSERT_NOT_NULL(arr);
    arr[0] = 1;
    arr[4] = 5;

    arr = (int *)amiport_reallocarray(arr, 10, sizeof(int));
    ASSERT_NOT_NULL(arr);
    /* Original data should be preserved */
    ASSERT_EQ(arr[0], 1);
    ASSERT_EQ(arr[4], 5);

    free(arr);
}

/* --- recallocarray tests --- */

TEST(recallocarray_basic)
{
    int *arr;

    arr = (int *)amiport_recallocarray(NULL, 0, 10, sizeof(int));
    ASSERT_NOT_NULL(arr);

    /* New memory should be zeroed */
    ASSERT_EQ(arr[0], 0);
    ASSERT_EQ(arr[5], 0);
    ASSERT_EQ(arr[9], 0);

    free(arr);
}

TEST(recallocarray_grow_zeroes_new)
{
    int *arr;

    arr = (int *)amiport_recallocarray(NULL, 0, 5, sizeof(int));
    ASSERT_NOT_NULL(arr);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;

    /* Grow from 5 to 10 elements — new elements should be zero */
    arr = (int *)amiport_recallocarray(arr, 5, 10, sizeof(int));
    ASSERT_NOT_NULL(arr);

    /* Original data preserved */
    ASSERT_EQ(arr[0], 10);
    ASSERT_EQ(arr[4], 50);

    /* New elements zeroed */
    ASSERT_EQ(arr[5], 0);
    ASSERT_EQ(arr[9], 0);

    free(arr);
}

TEST(recallocarray_shrink)
{
    int *arr;

    arr = (int *)amiport_recallocarray(NULL, 0, 10, sizeof(int));
    ASSERT_NOT_NULL(arr);
    arr[0] = 42;
    arr[4] = 99;

    /* Shrink from 10 to 5 */
    arr = (int *)amiport_recallocarray(arr, 10, 5, sizeof(int));
    ASSERT_NOT_NULL(arr);

    /* Preserved data within new bounds */
    ASSERT_EQ(arr[0], 42);
    ASSERT_EQ(arr[4], 99);

    free(arr);
}

TEST(recallocarray_overflow)
{
    void *p;

    p = amiport_recallocarray(NULL, 0, (size_t)-1, (size_t)-1);
    ASSERT_NULL(p);
}

/* --- strcasecmp tests --- */

TEST(strcasecmp_equal)
{
    ASSERT_EQ(amiport_strcasecmp("hello", "hello"), 0);
}

TEST(strcasecmp_case_insensitive)
{
    ASSERT_EQ(amiport_strcasecmp("Hello", "hELLO"), 0);
    ASSERT_EQ(amiport_strcasecmp("ABC", "abc"), 0);
}

TEST(strcasecmp_inequality)
{
    ASSERT(amiport_strcasecmp("apple", "banana") < 0);
    ASSERT(amiport_strcasecmp("zebra", "Ant") > 0);
}

TEST(strcasecmp_empty)
{
    ASSERT_EQ(amiport_strcasecmp("", ""), 0);
    ASSERT(amiport_strcasecmp("", "a") < 0);
    ASSERT(amiport_strcasecmp("a", "") > 0);
}

/* --- strncasecmp tests --- */

TEST(strncasecmp_equal)
{
    ASSERT_EQ(amiport_strncasecmp("hello", "HELLO", 5), 0);
}

TEST(strncasecmp_partial)
{
    /* Compare only first 3 chars — "hel" == "HEL" */
    ASSERT_EQ(amiport_strncasecmp("hello", "HELP", 3), 0);
    /* Full 4 chars differ: 'l' vs 'p' */
    ASSERT(amiport_strncasecmp("hello", "HELP", 4) < 0);
}

TEST(strncasecmp_zero_n)
{
    ASSERT_EQ(amiport_strncasecmp("abc", "xyz", 0), 0);
}

/* --- strcasestr tests --- */

TEST(strcasestr_found)
{
    const char *result;
    result = amiport_strcasestr("Hello World", "world");
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "World");
}

TEST(strcasestr_not_found)
{
    const char *result;
    result = amiport_strcasestr("Hello World", "planet");
    ASSERT_NULL(result);
}

TEST(strcasestr_empty_needle)
{
    const char *result;
    result = amiport_strcasestr("Hello", "");
    /* Empty needle returns pointer to haystack */
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "Hello");
}

TEST(strcasestr_case_variants)
{
    const char *result;
    result = amiport_strcasestr("Commodore Amiga", "AMIGA");
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "Amiga");
}

int main(void)
{
    (void)verstag;
    printf("=== BSD String Function Tests ===\n");

    RUN_TEST(strlcpy_basic);
    RUN_TEST(strlcpy_truncation);
    RUN_TEST(strlcpy_exact_fit);
    RUN_TEST(strlcpy_empty_src);
    RUN_TEST(strlcpy_zero_dst_size);

    RUN_TEST(strlcat_basic);
    RUN_TEST(strlcat_truncation);
    RUN_TEST(strlcat_empty_dst);
    RUN_TEST(strlcat_empty_src);

    RUN_TEST(reallocarray_basic);
    RUN_TEST(reallocarray_overflow);
    RUN_TEST(reallocarray_zero);
    RUN_TEST(reallocarray_resize);

    RUN_TEST(recallocarray_basic);
    RUN_TEST(recallocarray_grow_zeroes_new);
    RUN_TEST(recallocarray_shrink);
    RUN_TEST(recallocarray_overflow);

    printf("\n--- Case-insensitive String Tests ---\n");

    RUN_TEST(strcasecmp_equal);
    RUN_TEST(strcasecmp_case_insensitive);
    RUN_TEST(strcasecmp_inequality);
    RUN_TEST(strcasecmp_empty);

    RUN_TEST(strncasecmp_equal);
    RUN_TEST(strncasecmp_partial);
    RUN_TEST(strncasecmp_zero_n);

    RUN_TEST(strcasestr_found);
    RUN_TEST(strcasestr_not_found);
    RUN_TEST(strcasestr_empty_needle);
    RUN_TEST(strcasestr_case_variants);

    return test_summary();
}
