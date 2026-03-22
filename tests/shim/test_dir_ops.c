/*
 * test_dir_ops.c — Tests for amiport directory operations
 */

#include "test_framework.h"
#include <amiport/dirent.h>
#include <amiport/unistd.h>
#include <amiport/errno_map.h>
#include <errno.h>
#include <string.h>

static const char *verstag = "$VER: test_dir_ops 1.0 (19.03.2026)";
long __stack = 32768;

TEST(opendir_valid)
{
    AMIPORT_DIR *dir = amiport_opendir("T:");
    ASSERT_NOT_NULL(dir);
    amiport_closedir(dir);
}

TEST(opendir_nonexistent)
{
    AMIPORT_DIR *dir = amiport_opendir("T:nonexistent_dir_999");
    ASSERT_NULL(dir);
    ASSERT_EQ(errno, ENOENT);
}

TEST(opendir_file_not_dir)
{
    int fd;
    AMIPORT_DIR *dir;

    /* Create a regular file */
    fd = amiport_open("T:test_notadir.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        amiport_write(fd, "x", 1);
        amiport_close(fd);
    }

    dir = amiport_opendir("T:test_notadir.txt");
    ASSERT_NULL(dir);
    ASSERT_EQ(errno, ENOTDIR);

    amiport_unlink("T:test_notadir.txt");
}

TEST(readdir_finds_entries)
{
    AMIPORT_DIR *dir;
    struct amiport_dirent *ent;
    int count = 0;
    int fd;

    /* Create a known file in T: */
    fd = amiport_open("T:test_readdir_marker.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        amiport_write(fd, "x", 1);
        amiport_close(fd);
    }

    dir = amiport_opendir("T:");
    ASSERT_NOT_NULL(dir);

    while ((ent = amiport_readdir(dir)) != NULL) {
        ASSERT(strlen(ent->d_name) > 0);
        count++;
    }

    ASSERT(count > 0);
    amiport_closedir(dir);
    amiport_unlink("T:test_readdir_marker.txt");
}

TEST(closedir_null)
{
    ASSERT_EQ(amiport_closedir(NULL), -1);
    ASSERT_EQ(errno, EBADF);
}

TEST(mkdir_and_rmdir)
{
    /* Create a temporary directory in T: then remove it */
    ASSERT_EQ(amiport_mkdir("T:test_mkdir_999", 0755), 0);

    /* Should be openable as a directory */
    {
        AMIPORT_DIR *dir = amiport_opendir("T:test_mkdir_999");
        ASSERT_NOT_NULL(dir);
        amiport_closedir(dir);
    }

    /* Remove it — should succeed */
    ASSERT_EQ(amiport_rmdir("T:test_mkdir_999"), 0);

    /* Should no longer exist */
    {
        AMIPORT_DIR *dir = amiport_opendir("T:test_mkdir_999");
        ASSERT_NULL(dir);
    }
}

TEST(mkdir_already_exists)
{
    /* Creating a directory that already exists should fail */
    ASSERT_EQ(amiport_mkdir("T:test_mkdir_dup", 0755), 0);
    /* Second mkdir must fail */
    ASSERT_EQ(amiport_mkdir("T:test_mkdir_dup", 0755), -1);
    ASSERT(errno != 0);
    /* Clean up */
    amiport_rmdir("T:test_mkdir_dup");
}

TEST(rmdir_nonexistent)
{
    /* Removing a directory that does not exist must return -1 */
    ASSERT_EQ(amiport_rmdir("T:test_rmdir_ghost_999"), -1);
    ASSERT(errno != 0);
}

int main(void)
{
    (void)verstag;
    printf("=== Directory Ops Tests ===\n");

    RUN_TEST(opendir_valid);
    RUN_TEST(opendir_nonexistent);
    RUN_TEST(opendir_file_not_dir);
    RUN_TEST(readdir_finds_entries);
    RUN_TEST(closedir_null);
    RUN_TEST(mkdir_and_rmdir);
    RUN_TEST(mkdir_already_exists);
    RUN_TEST(rmdir_nonexistent);

    return test_summary();
}
