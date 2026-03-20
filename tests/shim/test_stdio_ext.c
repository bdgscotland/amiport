/*
 * test_stdio_ext.c — Tests for amiport extended stdio functions
 *                    (vasprintf, asprintf, mkstemp, pread, pwrite)
 */

#include "test_framework.h"
#define AMIPORT_NO_STDIO_EXT_MACROS
#include <amiport/unistd.h>
#include <amiport/stdio_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *verstag = "$VER: test_stdio_ext 1.0 (20.03.2026)";
long __stack = 32768;

/* --- asprintf tests --- */

TEST(asprintf_basic)
{
    char *str = NULL;
    int ret;

    ret = amiport_asprintf(&str, "hello %s %d", "world", 42);
    ASSERT(ret > 0);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "hello world 42");
    ASSERT_EQ(ret, 14);
    free(str);
}

TEST(asprintf_empty)
{
    char *str = NULL;
    int ret;

    ret = amiport_asprintf(&str, "");
    ASSERT_EQ(ret, 0);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "");
    free(str);
}

TEST(asprintf_long_string)
{
    char *str = NULL;
    int ret;

    ret = amiport_asprintf(&str, "%d + %d = %d", 12345, 67890, 80235);
    ASSERT(ret > 0);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "12345 + 67890 = 80235");
    free(str);
}

/* --- mkstemp tests --- */

TEST(mkstemp_creates_file)
{
    char tmpl[] = "T:amiport_testXXXXXX";
    int fd;

    fd = amiport_mkstemp(tmpl);
    ASSERT(fd >= 0);

    /* Template should have been modified */
    ASSERT(strcmp(tmpl + strlen(tmpl) - 6, "XXXXXX") != 0);

    /* Should be writable */
    ASSERT_EQ(amiport_write(fd, "test", 4), 4);

    amiport_close(fd);
    amiport_unlink(tmpl);
}

TEST(mkstemp_unique_names)
{
    char tmpl1[] = "T:amiport_testXXXXXX";
    char tmpl2[] = "T:amiport_testXXXXXX";
    int fd1, fd2;

    fd1 = amiport_mkstemp(tmpl1);
    fd2 = amiport_mkstemp(tmpl2);
    ASSERT(fd1 >= 0);
    ASSERT(fd2 >= 0);
    ASSERT(fd1 != fd2);

    /* Templates should differ */
    ASSERT(strcmp(tmpl1, tmpl2) != 0);

    amiport_close(fd1);
    amiport_close(fd2);
    amiport_unlink(tmpl1);
    amiport_unlink(tmpl2);
}

TEST(mkstemp_bad_template)
{
    char tmpl[] = "T:amiport_testXXX";
    int fd;

    fd = amiport_mkstemp(tmpl);
    ASSERT_EQ(fd, -1);
}

/* --- pread/pwrite tests --- */

TEST(pread_basic)
{
    const char *data = "ABCDEFGHIJ";
    int fd;
    char buf[4];
    LONG n;

    /* Create test file */
    fd = amiport_open("T:test_pread.tmp", O_WRONLY | O_CREAT | O_TRUNC);
    ASSERT(fd >= 0);
    amiport_write(fd, data, 10);
    amiport_close(fd);

    /* Read at offset */
    fd = amiport_open("T:test_pread.tmp", O_RDONLY);
    ASSERT(fd >= 0);

    memset(buf, 0, sizeof(buf));
    n = amiport_pread(fd, buf, 3, 5);
    ASSERT_EQ(n, 3);
    buf[3] = '\0';
    ASSERT_STR_EQ(buf, "FGH");

    /* Verify file position wasn't changed (should still be at 0) */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd, buf, 3);
    ASSERT_EQ(n, 3);
    buf[3] = '\0';
    ASSERT_STR_EQ(buf, "ABC");

    amiport_close(fd);
    amiport_unlink("T:test_pread.tmp");
}

TEST(pwrite_basic)
{
    int fd;
    char buf[16];
    LONG n;

    /* Create file with initial content */
    fd = amiport_open("T:test_pwrite.tmp", O_RDWR | O_CREAT | O_TRUNC);
    ASSERT(fd >= 0);
    amiport_write(fd, "AAAAAAAAAA", 10);

    /* Write at offset 3 */
    amiport_pwrite(fd, "XYZ", 3, 3);

    /* Read full file to verify */
    amiport_lseek(fd, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd, buf, 16);
    ASSERT_EQ(n, 10);
    buf[n] = '\0';
    ASSERT_STR_EQ(buf, "AAAXYZAAAA");

    amiport_close(fd);
    amiport_unlink("T:test_pwrite.tmp");
}

int main(void)
{
    (void)verstag;
    printf("=== Extended Stdio Function Tests ===\n");

    RUN_TEST(asprintf_basic);
    RUN_TEST(asprintf_empty);
    RUN_TEST(asprintf_long_string);

    RUN_TEST(mkstemp_creates_file);
    RUN_TEST(mkstemp_unique_names);
    RUN_TEST(mkstemp_bad_template);

    RUN_TEST(pread_basic);
    RUN_TEST(pwrite_basic);

    return test_summary();
}
