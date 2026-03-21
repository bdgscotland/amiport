/*
 * test_dup.c -- Tests for amiport_dup() and amiport_dup2()
 */

#include "test_framework.h"
#include <amiport/unistd.h>
#include <amiport/errno_map.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const char *verstag = "$VER: test_dup 1.0 (21.03.2026)";
long __stack = 32768;

/* Helper: create a temp file with known content */
static int create_test_file(const char *path, const char *content)
{
    int fd = amiport_open(path, O_WRONLY | O_CREAT | O_TRUNC);
    LONG len;
    if (fd < 0) return -1;
    len = (LONG)strlen(content);
    amiport_write(fd, content, len);
    amiport_close(fd);
    return 0;
}

/* --- dup tests --- */

TEST(dup_valid_fd)
{
    int fd, fd2;
    char buf1[32], buf2[32];
    LONG n;

    create_test_file("T:test_dup.txt", "hello world");

    fd = amiport_open("T:test_dup.txt", O_RDONLY);
    ASSERT(fd >= 0);

    fd2 = amiport_dup(fd);
    ASSERT(fd2 >= 0);
    ASSERT(fd2 != fd);

    /* Read from original fd */
    memset(buf1, 0, sizeof(buf1));
    n = amiport_read(fd, buf1, 5);
    ASSERT_EQ(n, 5);

    /* Read from dup'd fd -- shares file position, so continues from offset 5 */
    memset(buf2, 0, sizeof(buf2));
    n = amiport_read(fd2, buf2, 6);
    ASSERT_EQ(n, 6);
    ASSERT_STR_EQ(buf2, " world");

    amiport_close(fd);
    amiport_close(fd2);
    amiport_unlink("T:test_dup.txt");
}

TEST(dup_invalid_fd)
{
    int fd;

    fd = amiport_dup(-1);
    ASSERT_EQ(fd, -1);
    ASSERT_EQ(errno, EBADF);

    fd = amiport_dup(99);
    ASSERT_EQ(fd, -1);
    ASSERT_EQ(errno, EBADF);
}

/* --- dup2 tests --- */

TEST(dup2_same_fd)
{
    int fd, result;

    create_test_file("T:test_dup2_same.txt", "data");
    fd = amiport_open("T:test_dup2_same.txt", O_RDONLY);
    ASSERT(fd >= 0);

    /* dup2(fd, fd) should return fd without doing anything */
    result = amiport_dup2(fd, fd);
    ASSERT_EQ(result, fd);

    /* fd should still be usable */
    {
        char buf[16];
        LONG n;
        memset(buf, 0, sizeof(buf));
        n = amiport_read(fd, buf, sizeof(buf) - 1);
        ASSERT(n > 0);
        ASSERT_STR_EQ(buf, "data");
    }

    amiport_close(fd);
    amiport_unlink("T:test_dup2_same.txt");
}

TEST(dup2_close_existing)
{
    int fd_old, fd_new;
    char buf[32];
    LONG n;

    create_test_file("T:test_dup2_a.txt", "alpha");
    create_test_file("T:test_dup2_b.txt", "beta");

    fd_old = amiport_open("T:test_dup2_a.txt", O_RDONLY);
    ASSERT(fd_old >= 0);

    fd_new = amiport_open("T:test_dup2_b.txt", O_RDONLY);
    ASSERT(fd_new >= 0);

    /* dup2(fd_old, fd_new) should close fd_new's file and make fd_new
     * point to the same handle as fd_old */
    ASSERT_EQ(amiport_dup2(fd_old, fd_new), fd_new);

    /* Reading from fd_new should now give the content from file A,
     * starting from wherever the shared file position is */
    memset(buf, 0, sizeof(buf));
    amiport_lseek(fd_new, 0, SEEK_SET);
    n = amiport_read(fd_new, buf, sizeof(buf) - 1);
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "alpha");

    amiport_close(fd_old);
    amiport_close(fd_new);
    amiport_unlink("T:test_dup2_a.txt");
    amiport_unlink("T:test_dup2_b.txt");
}

TEST(dup2_invalid_old)
{
    int result;

    result = amiport_dup2(-1, 5);
    ASSERT_EQ(result, -1);
    ASSERT_EQ(errno, EBADF);
}

/* --- close interaction tests --- */

TEST(close_dup_keeps_original)
{
    int fd, fd2;
    char buf[32];
    LONG n;

    create_test_file("T:test_dup_close1.txt", "survive");

    fd = amiport_open("T:test_dup_close1.txt", O_RDONLY);
    ASSERT(fd >= 0);

    fd2 = amiport_dup(fd);
    ASSERT(fd2 >= 0);

    /* Close the dup'd fd */
    ASSERT_EQ(amiport_close(fd2), 0);

    /* Original fd should still work */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd, buf, sizeof(buf) - 1);
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "survive");

    amiport_close(fd);
    amiport_unlink("T:test_dup_close1.txt");
}

TEST(close_original_keeps_dup)
{
    int fd, fd2;
    char buf[32];
    LONG n;

    create_test_file("T:test_dup_close2.txt", "persist");

    fd = amiport_open("T:test_dup_close2.txt", O_RDONLY);
    ASSERT(fd >= 0);

    fd2 = amiport_dup(fd);
    ASSERT(fd2 >= 0);

    /* Close the original fd */
    ASSERT_EQ(amiport_close(fd), 0);

    /* Dup'd fd should still work */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd2, buf, sizeof(buf) - 1);
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "persist");

    amiport_close(fd2);
    amiport_unlink("T:test_dup_close2.txt");
}

TEST(chaos)
{
    int fd1, fd2, fd3;
    char buf[32];
    LONG n;

    create_test_file("T:test_dup_chaos.txt", "ABCDEFGHIJ");

    fd1 = amiport_open("T:test_dup_chaos.txt", O_RDONLY);
    ASSERT(fd1 >= 0);

    /* Chain: fd1 -> fd2 -> fd3, all share same BPTR */
    fd2 = amiport_dup(fd1);
    ASSERT(fd2 >= 0);

    fd3 = amiport_dup(fd2);
    ASSERT(fd3 >= 0);

    /* Read 3 bytes from fd1 */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd1, buf, 3);
    ASSERT_EQ(n, 3);
    ASSERT_STR_EQ(buf, "ABC");

    /* Close the middle one (fd2) */
    ASSERT_EQ(amiport_close(fd2), 0);

    /* fd1 still works -- continues from position 3 */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd1, buf, 3);
    ASSERT_EQ(n, 3);
    ASSERT_STR_EQ(buf, "DEF");

    /* fd3 still works -- continues from shared position 6 */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd3, buf, 4);
    ASSERT_EQ(n, 4);
    ASSERT_STR_EQ(buf, "GHIJ");

    amiport_close(fd1);
    amiport_close(fd3);
    amiport_unlink("T:test_dup_chaos.txt");
}

int main(void)
{
    (void)verstag;
    printf("=== dup/dup2 Tests ===\n");

    RUN_TEST(dup_valid_fd);
    RUN_TEST(dup_invalid_fd);
    RUN_TEST(dup2_same_fd);
    RUN_TEST(dup2_close_existing);
    RUN_TEST(dup2_invalid_old);
    RUN_TEST(close_dup_keeps_original);
    RUN_TEST(close_original_keeps_dup);
    RUN_TEST(chaos);

    return test_summary();
}
