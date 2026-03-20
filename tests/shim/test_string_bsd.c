/*
 * test_string_bsd.c — Tests for amiport BSD string functions
 *                     (strlcpy, strlcat, reallocarray)
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

    return test_summary();
}
