/*
 * test_select.c — Tests for amiport Tier 2 select emulation
 */

#include "test_framework.h"
#include <amiport/unistd.h>
#include <amiport/sys/time.h>
#include <amiport-emu/select.h>
#include <stdio.h>
#include <string.h>

static const char *verstag = "$VER: test_select 1.0 (20.03.2026)";
long __stack = 32768;

TEST(fd_set_operations)
{
    amiport_emu_fd_set fds;

    AMIPORT_EMU_FD_ZERO(&fds);

    /* Initially all clear */
    ASSERT(!AMIPORT_EMU_FD_ISSET(0, &fds));
    ASSERT(!AMIPORT_EMU_FD_ISSET(5, &fds));

    /* Set some bits */
    AMIPORT_EMU_FD_SET(5, &fds);
    ASSERT(AMIPORT_EMU_FD_ISSET(5, &fds));
    ASSERT(!AMIPORT_EMU_FD_ISSET(4, &fds));

    /* Clear */
    AMIPORT_EMU_FD_CLR(5, &fds);
    ASSERT(!AMIPORT_EMU_FD_ISSET(5, &fds));
}

TEST(select_write_ready)
{
    int fd;
    amiport_emu_fd_set writefds;
    struct amiport_timeval tv;
    int result;

    /* Open a file for writing — should always be "write ready" */
    fd = amiport_open("T:test_select.tmp", O_WRONLY | O_CREAT | O_TRUNC);
    ASSERT(fd >= 0);

    AMIPORT_EMU_FD_ZERO(&writefds);
    AMIPORT_EMU_FD_SET(fd, &writefds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    result = amiport_emu_select(fd + 1, NULL, &writefds, NULL, &tv);
    ASSERT(result >= 0);

    amiport_close(fd);
    amiport_unlink("T:test_select.tmp");
}

TEST(select_timeout_zero)
{
    amiport_emu_fd_set readfds;
    struct amiport_timeval tv;
    int result;

    AMIPORT_EMU_FD_ZERO(&readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    /* Select with no fds set and zero timeout should return 0 */
    result = amiport_emu_select(0, &readfds, NULL, NULL, &tv);
    ASSERT_EQ(result, 0);
}

int main(void)
{
    (void)verstag;
    printf("=== Select Emulation Tests ===\n");

    RUN_TEST(fd_set_operations);
    RUN_TEST(select_write_ready);
    RUN_TEST(select_timeout_zero);

    return test_summary();
}
