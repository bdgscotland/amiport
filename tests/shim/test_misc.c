/*
 * test_misc.c — Tests for amiport misc functions (strtok_r, tmpfile, usleep)
 */

#include "test_framework.h"
#include <amiport/unistd.h>
#include <amiport/sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *verstag = "$VER: test_misc 1.0 (19.03.2026)";
long __stack = 32768;

/* --- strtok_r tests --- */

TEST(strtok_r_basic)
{
    char str[] = "hello,world,foo";
    char *saveptr;
    char *tok;

    tok = amiport_strtok_r(str, ",", &saveptr);
    ASSERT_NOT_NULL(tok);
    ASSERT_STR_EQ(tok, "hello");

    tok = amiport_strtok_r(NULL, ",", &saveptr);
    ASSERT_NOT_NULL(tok);
    ASSERT_STR_EQ(tok, "world");

    tok = amiport_strtok_r(NULL, ",", &saveptr);
    ASSERT_NOT_NULL(tok);
    ASSERT_STR_EQ(tok, "foo");

    tok = amiport_strtok_r(NULL, ",", &saveptr);
    ASSERT_NULL(tok);
}

TEST(strtok_r_leading_delimiters)
{
    char str[] = ",,hello,,world,,";
    char *saveptr;
    char *tok;

    tok = amiport_strtok_r(str, ",", &saveptr);
    ASSERT_NOT_NULL(tok);
    ASSERT_STR_EQ(tok, "hello");

    tok = amiport_strtok_r(NULL, ",", &saveptr);
    ASSERT_NOT_NULL(tok);
    ASSERT_STR_EQ(tok, "world");

    tok = amiport_strtok_r(NULL, ",", &saveptr);
    ASSERT_NULL(tok);
}

TEST(strtok_r_empty_string)
{
    char str[] = "";
    char *saveptr;
    char *tok;

    tok = amiport_strtok_r(str, ",", &saveptr);
    ASSERT_NULL(tok);
}

TEST(strtok_r_no_delimiters)
{
    char str[] = "hello";
    char *saveptr;
    char *tok;

    tok = amiport_strtok_r(str, ",", &saveptr);
    ASSERT_NOT_NULL(tok);
    ASSERT_STR_EQ(tok, "hello");

    tok = amiport_strtok_r(NULL, ",", &saveptr);
    ASSERT_NULL(tok);
}

TEST(strtok_r_multiple_delimiters)
{
    char str[] = "one:two;three:four";
    char *saveptr;
    char *tok;

    tok = amiport_strtok_r(str, ":;", &saveptr);
    ASSERT_STR_EQ(tok, "one");
    tok = amiport_strtok_r(NULL, ":;", &saveptr);
    ASSERT_STR_EQ(tok, "two");
    tok = amiport_strtok_r(NULL, ":;", &saveptr);
    ASSERT_STR_EQ(tok, "three");
    tok = amiport_strtok_r(NULL, ":;", &saveptr);
    ASSERT_STR_EQ(tok, "four");
    tok = amiport_strtok_r(NULL, ":;", &saveptr);
    ASSERT_NULL(tok);
}

TEST(strtok_r_independent_contexts)
{
    char str1[] = "a,b";
    char str2[] = "x,y";
    char *save1, *save2;
    char *t1, *t2;

    t1 = amiport_strtok_r(str1, ",", &save1);
    t2 = amiport_strtok_r(str2, ",", &save2);
    ASSERT_STR_EQ(t1, "a");
    ASSERT_STR_EQ(t2, "x");

    t1 = amiport_strtok_r(NULL, ",", &save1);
    t2 = amiport_strtok_r(NULL, ",", &save2);
    ASSERT_STR_EQ(t1, "b");
    ASSERT_STR_EQ(t2, "y");
}

/* --- tmpfile tests --- */

TEST(tmpfile_creates_writable)
{
    FILE *fp = amiport_tmpfile();
    ASSERT_NOT_NULL(fp);

    ASSERT(fputs("test data", fp) >= 0);
    fflush(fp);

    /* Seek back and read */
    rewind(fp);
    {
        char buf[32];
        char *result;
        memset(buf, 0, sizeof(buf));
        result = fgets(buf, sizeof(buf), fp);
        ASSERT_NOT_NULL(result);
        ASSERT_STR_EQ(buf, "test data");
    }

    fclose(fp);
}

TEST(tmpfile_unique_files)
{
    FILE *fp1 = amiport_tmpfile();
    FILE *fp2 = amiport_tmpfile();
    ASSERT_NOT_NULL(fp1);
    ASSERT_NOT_NULL(fp2);
    ASSERT(fp1 != fp2);
    fclose(fp1);
    fclose(fp2);
}

/* --- usleep tests --- */

TEST(usleep_zero)
{
    ASSERT_EQ(amiport_usleep(0), 0);
}

TEST(usleep_small)
{
    /* 50ms — should map to at least 1 tick */
    ASSERT_EQ(amiport_usleep(50000), 0);
}

TEST(usleep_returns_zero)
{
    ASSERT_EQ(amiport_usleep(1), 0);
}

int main(void)
{
    (void)verstag;
    printf("=== Misc Function Tests ===\n");

    RUN_TEST(strtok_r_basic);
    RUN_TEST(strtok_r_leading_delimiters);
    RUN_TEST(strtok_r_empty_string);
    RUN_TEST(strtok_r_no_delimiters);
    RUN_TEST(strtok_r_multiple_delimiters);
    RUN_TEST(strtok_r_independent_contexts);
    RUN_TEST(tmpfile_creates_writable);
    RUN_TEST(tmpfile_unique_files);
    RUN_TEST(usleep_zero);
    RUN_TEST(usleep_small);
    RUN_TEST(usleep_returns_zero);

    return test_summary();
}
