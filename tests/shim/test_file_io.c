/*
 * test_file_io.c — Tests for amiport file I/O wrappers
 */

#include "test_framework.h"
#include <amiport/unistd.h>
#include <amiport/errno_map.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const char *verstag = "$VER: test_file_io 1.0 (19.03.2026)";
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

/* --- open/close tests --- */

TEST(open_read_existing)
{
    int fd;
    create_test_file("T:test_open.txt", "hello");
    fd = amiport_open("T:test_open.txt", O_RDONLY);
    ASSERT(fd >= 0);
    ASSERT_EQ(amiport_close(fd), 0);
    amiport_unlink("T:test_open.txt");
}

TEST(open_nonexistent_fails)
{
    int fd = amiport_open("T:does_not_exist_12345.txt", O_RDONLY);
    ASSERT_EQ(fd, -1);
    ASSERT_EQ(errno, ENOENT);
}

TEST(open_write_creates_file)
{
    int fd;
    amiport_unlink("T:test_create.txt");
    fd = amiport_open("T:test_create.txt", O_WRONLY | O_CREAT | O_TRUNC);
    ASSERT(fd >= 0);
    amiport_close(fd);
    /* Verify file exists */
    fd = amiport_open("T:test_create.txt", O_RDONLY);
    ASSERT(fd >= 0);
    amiport_close(fd);
    amiport_unlink("T:test_create.txt");
}

TEST(open_creat_no_trunc_preserves)
{
    int fd;
    char buf[32];
    LONG n;

    /* Create file with content */
    create_test_file("T:test_notrunc.txt", "original");

    /* Open with O_CREAT but without O_TRUNC — should preserve content */
    fd = amiport_open("T:test_notrunc.txt", O_RDWR | O_CREAT);
    ASSERT(fd >= 0);

    /* Read back — should get original content */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd, buf, sizeof(buf) - 1);
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "original");

    amiport_close(fd);
    amiport_unlink("T:test_notrunc.txt");
}

TEST(open_creat_no_trunc_creates_if_missing)
{
    int fd;
    amiport_unlink("T:test_creat_new.txt");

    fd = amiport_open("T:test_creat_new.txt", O_RDWR | O_CREAT);
    ASSERT(fd >= 0);
    amiport_close(fd);

    /* Verify it exists */
    ASSERT_EQ(amiport_access("T:test_creat_new.txt", F_OK), 0);
    amiport_unlink("T:test_creat_new.txt");
}

TEST(close_invalid_fd)
{
    ASSERT_EQ(amiport_close(-1), -1);
    ASSERT_EQ(errno, EBADF);
    ASSERT_EQ(amiport_close(99), -1);
    ASSERT_EQ(errno, EBADF);
}

/* --- read/write tests --- */

TEST(read_write_roundtrip)
{
    int fd;
    char buf[64];
    LONG n;
    const char *msg = "test data 12345";

    create_test_file("T:test_rw.txt", msg);

    fd = amiport_open("T:test_rw.txt", O_RDONLY);
    ASSERT(fd >= 0);

    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd, buf, sizeof(buf) - 1);
    ASSERT_EQ(n, (LONG)strlen(msg));
    ASSERT_STR_EQ(buf, msg);

    amiport_close(fd);
    amiport_unlink("T:test_rw.txt");
}

TEST(read_invalid_fd)
{
    char buf[8];
    ASSERT_EQ(amiport_read(-1, buf, 8), -1);
    ASSERT_EQ(errno, EBADF);
}

TEST(write_invalid_fd)
{
    ASSERT_EQ(amiport_write(-1, "x", 1), -1);
    ASSERT_EQ(errno, EBADF);
}

/* --- lseek tests --- */

TEST(lseek_set_cur_end)
{
    int fd;
    LONG pos;

    create_test_file("T:test_seek.txt", "0123456789");

    fd = amiport_open("T:test_seek.txt", O_RDONLY);
    ASSERT(fd >= 0);

    /* SEEK_SET to position 5 */
    pos = amiport_lseek(fd, 5, SEEK_SET);
    ASSERT_EQ(pos, 5);

    /* SEEK_CUR +2 → position 7 */
    pos = amiport_lseek(fd, 2, SEEK_CUR);
    ASSERT_EQ(pos, 7);

    /* SEEK_END -3 → position 7 (10 - 3) */
    pos = amiport_lseek(fd, -3, SEEK_END);
    ASSERT_EQ(pos, 7);

    amiport_close(fd);
    amiport_unlink("T:test_seek.txt");
}

TEST(lseek_invalid_whence)
{
    int fd;
    create_test_file("T:test_seek2.txt", "data");
    fd = amiport_open("T:test_seek2.txt", O_RDONLY);
    ASSERT(fd >= 0);

    ASSERT_EQ(amiport_lseek(fd, 0, 99), -1);
    ASSERT_EQ(errno, EINVAL);

    amiport_close(fd);
    amiport_unlink("T:test_seek2.txt");
}

/* --- unlink/rename/access tests --- */

TEST(unlink_existing)
{
    create_test_file("T:test_unlink.txt", "x");
    ASSERT_EQ(amiport_unlink("T:test_unlink.txt"), 0);
    ASSERT_EQ(amiport_access("T:test_unlink.txt", F_OK), -1);
}

TEST(unlink_nonexistent)
{
    ASSERT_EQ(amiport_unlink("T:nonexistent_99.txt"), -1);
    ASSERT_EQ(errno, ENOENT);
}

TEST(rename_file)
{
    create_test_file("T:test_ren_a.txt", "data");
    amiport_unlink("T:test_ren_b.txt");
    ASSERT_EQ(amiport_rename("T:test_ren_a.txt", "T:test_ren_b.txt"), 0);
    ASSERT_EQ(amiport_access("T:test_ren_a.txt", F_OK), -1);
    ASSERT_EQ(amiport_access("T:test_ren_b.txt", F_OK), 0);
    amiport_unlink("T:test_ren_b.txt");
}

TEST(access_existing)
{
    create_test_file("T:test_access.txt", "x");
    ASSERT_EQ(amiport_access("T:test_access.txt", F_OK), 0);
    amiport_unlink("T:test_access.txt");
}

TEST(access_nonexistent)
{
    ASSERT_EQ(amiport_access("T:no_such_file_777.txt", F_OK), -1);
    ASSERT_EQ(errno, ENOENT);
}

/* --- chmod test --- */

TEST(chmod_noop)
{
    /* chmod is a no-op stub — must return 0 on any valid path */
    create_test_file("T:test_chmod.txt", "data");
    ASSERT_EQ(amiport_chmod("T:test_chmod.txt", 0644), 0);
    ASSERT_EQ(amiport_chmod("T:test_chmod.txt", 0755), 0);
    ASSERT_EQ(amiport_chmod("T:test_chmod.txt", 0000), 0);
    amiport_unlink("T:test_chmod.txt");
}

/* --- realpath tests --- */

TEST(realpath_existing_file)
{
    char resolved[256];
    char *result;

    create_test_file("T:test_realpath.txt", "data");

    result = amiport_realpath("T:test_realpath.txt", resolved);
    ASSERT_NOT_NULL(result);
    /* Must be non-empty and start with a volume name */
    ASSERT(strlen(result) > 0);
    /* Must contain a colon (AmigaOS volume: prefix) */
    ASSERT(strchr(result, ':') != NULL);

    amiport_unlink("T:test_realpath.txt");
}

TEST(realpath_null_resolved)
{
    /* When resolved is NULL, realpath() must malloc the buffer */
    char *result;

    create_test_file("T:test_realpath2.txt", "data");

    result = amiport_realpath("T:test_realpath2.txt", NULL);
    ASSERT_NOT_NULL(result);
    ASSERT(strlen(result) > 0);
    ASSERT(strchr(result, ':') != NULL);

    /* Caller must free the result (malloc'd inside realpath) */
    free(result);

    amiport_unlink("T:test_realpath2.txt");
}

TEST(realpath_nonexistent)
{
    char resolved[256];
    char *result;

    result = amiport_realpath("T:no_such_realpath_999.txt", resolved);
    ASSERT_NULL(result);
    ASSERT(errno != 0);
}

/* --- append test --- */

TEST(open_append)
{
    int fd;
    char buf[64];
    LONG n;

    create_test_file("T:test_append.txt", "first");

    fd = amiport_open("T:test_append.txt", O_WRONLY | O_APPEND);
    ASSERT(fd >= 0);
    amiport_write(fd, "second", 6);
    amiport_close(fd);

    /* Read back */
    fd = amiport_open("T:test_append.txt", O_RDONLY);
    ASSERT(fd >= 0);
    memset(buf, 0, sizeof(buf));
    n = amiport_read(fd, buf, sizeof(buf) - 1);
    ASSERT_EQ(n, 11);
    ASSERT_STR_EQ(buf, "firstsecond");

    amiport_close(fd);
    amiport_unlink("T:test_append.txt");
}

int main(void)
{
    (void)verstag;
    printf("=== File I/O Tests ===\n");

    RUN_TEST(open_read_existing);
    RUN_TEST(open_nonexistent_fails);
    RUN_TEST(open_write_creates_file);
    RUN_TEST(open_creat_no_trunc_preserves);
    RUN_TEST(open_creat_no_trunc_creates_if_missing);
    RUN_TEST(close_invalid_fd);
    RUN_TEST(read_write_roundtrip);
    RUN_TEST(read_invalid_fd);
    RUN_TEST(write_invalid_fd);
    RUN_TEST(lseek_set_cur_end);
    RUN_TEST(lseek_invalid_whence);
    RUN_TEST(unlink_existing);
    RUN_TEST(unlink_nonexistent);
    RUN_TEST(rename_file);
    RUN_TEST(access_existing);
    RUN_TEST(access_nonexistent);
    RUN_TEST(open_append);
    RUN_TEST(chmod_noop);
    RUN_TEST(realpath_existing_file);
    RUN_TEST(realpath_null_resolved);
    RUN_TEST(realpath_nonexistent);

    return test_summary();
}
