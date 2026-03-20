/*
 * test_errno.c — Tests for amiport errno mapping
 */

#include "test_framework.h"
#include <amiport/errno_map.h>
#include <dos/dos.h>
#include <errno.h>

static const char *verstag = "$VER: test_errno 1.0 (19.03.2026)";
long __stack = 32768;

TEST(object_not_found)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_OBJECT_NOT_FOUND), ENOENT);
}

TEST(dir_not_found)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_DIR_NOT_FOUND), ENOENT);
}

TEST(object_exists)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_OBJECT_EXISTS), EEXIST);
}

TEST(disk_full)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_DISK_FULL), ENOSPC);
}

TEST(write_protected)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_DISK_WRITE_PROTECTED), EROFS);
}

TEST(read_protected)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_READ_PROTECTED), EACCES);
}

TEST(no_free_store)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_NO_FREE_STORE), ENOMEM);
}

TEST(no_more_entries)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_NO_MORE_ENTRIES), 0);
}

TEST(zero_no_error)
{
    ASSERT_EQ(amiport_errno_from_ioerr(0), 0);
}

TEST(unknown_maps_to_eio)
{
    ASSERT_EQ(amiport_errno_from_ioerr(99999), EIO);
}

TEST(action_not_known)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_ACTION_NOT_KNOWN), ENOSYS);
}

TEST(object_in_use)
{
    ASSERT_EQ(amiport_errno_from_ioerr(ERROR_OBJECT_IN_USE), EBUSY);
}

int main(void)
{
    (void)verstag;
    printf("=== Errno Mapping Tests ===\n");

    RUN_TEST(object_not_found);
    RUN_TEST(dir_not_found);
    RUN_TEST(object_exists);
    RUN_TEST(disk_full);
    RUN_TEST(write_protected);
    RUN_TEST(read_protected);
    RUN_TEST(no_free_store);
    RUN_TEST(no_more_entries);
    RUN_TEST(zero_no_error);
    RUN_TEST(unknown_maps_to_eio);
    RUN_TEST(action_not_known);
    RUN_TEST(object_in_use);

    return test_summary();
}
