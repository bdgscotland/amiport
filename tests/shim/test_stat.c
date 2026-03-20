/*
 * test_stat.c — Tests for amiport stat/fstat
 */

#include "test_framework.h"
#include <amiport/sys/stat.h>
#include <amiport/unistd.h>
#include <amiport/errno_map.h>
#include <errno.h>
#include <string.h>

static const char *verstag = "$VER: test_stat 1.0 (19.03.2026)";
long __stack = 32768;

TEST(stat_regular_file)
{
    struct amiport_stat st;
    int fd;

    /* Create a file with known size */
    fd = amiport_open("T:test_stat.txt", O_WRONLY | O_CREAT | O_TRUNC);
    ASSERT(fd >= 0);
    amiport_write(fd, "hello world", 11);
    amiport_close(fd);

    ASSERT_EQ(amiport_stat("T:test_stat.txt", &st), 0);
    ASSERT(AMIPORT_S_ISREG(st.st_mode));
    ASSERT(!AMIPORT_S_ISDIR(st.st_mode));
    ASSERT_EQ(st.st_isdir, 0);
    ASSERT_EQ(st.st_size, 11);
    ASSERT(st.st_mtime > 0);

    amiport_unlink("T:test_stat.txt");
}

TEST(stat_directory)
{
    struct amiport_stat st;

    /* T: is always a valid directory on AmigaOS */
    ASSERT_EQ(amiport_stat("T:", &st), 0);
    ASSERT(AMIPORT_S_ISDIR(st.st_mode));
    ASSERT_EQ(st.st_isdir, 1);
}

TEST(stat_nonexistent)
{
    struct amiport_stat st;
    ASSERT_EQ(amiport_stat("T:no_such_file_stat_999.txt", &st), -1);
    ASSERT_EQ(errno, ENOENT);
}

TEST(stat_null_buf)
{
    ASSERT_EQ(amiport_stat("T:", NULL), -1);
    ASSERT_EQ(errno, EINVAL);
}

TEST(fstat_open_file)
{
    struct amiport_stat st;
    int fd;

    fd = amiport_open("T:test_fstat.txt", O_WRONLY | O_CREAT | O_TRUNC);
    ASSERT(fd >= 0);
    amiport_write(fd, "fstat test data", 15);
    amiport_close(fd);

    fd = amiport_open("T:test_fstat.txt", O_RDONLY);
    ASSERT(fd >= 0);

    ASSERT_EQ(amiport_fstat(fd, &st), 0);
    ASSERT(AMIPORT_S_ISREG(st.st_mode));
    ASSERT_EQ(st.st_size, 15);

    amiport_close(fd);
    amiport_unlink("T:test_fstat.txt");
}

TEST(fstat_invalid_fd)
{
    struct amiport_stat st;
    ASSERT_EQ(amiport_fstat(-1, &st), -1);
    ASSERT_EQ(errno, EBADF);
}

TEST(fstat_null_buf)
{
    ASSERT_EQ(amiport_fstat(0, NULL), -1);
    ASSERT_EQ(errno, EINVAL);
}

int main(void)
{
    (void)verstag;
    printf("=== Stat Tests ===\n");

    RUN_TEST(stat_regular_file);
    RUN_TEST(stat_directory);
    RUN_TEST(stat_nonexistent);
    RUN_TEST(stat_null_buf);
    RUN_TEST(fstat_open_file);
    RUN_TEST(fstat_invalid_fd);
    RUN_TEST(fstat_null_buf);

    return test_summary();
}
