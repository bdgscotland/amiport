/*
 * test_pipe.c — Tests for amiport Tier 2 pipe emulation
 */

#include "test_framework.h"
#include <amiport/unistd.h>
#include <amiport-emu/pipe.h>
#include <stdio.h>
#include <string.h>

static const char *verstag = "$VER: test_pipe 1.0 (20.03.2026)";
long __stack = 32768;

TEST(pipe_create)
{
    int pipefd[2];
    int rc = amiport_emu_pipe(pipefd);
    ASSERT_EQ(rc, 0);
    ASSERT(pipefd[0] >= 0);
    ASSERT(pipefd[1] >= 0);
    ASSERT(pipefd[0] != pipefd[1]);

    amiport_close(pipefd[0]);
    amiport_close(pipefd[1]);
}

TEST(pipe_write_read)
{
    int pipefd[2];
    const char *msg = "hello pipe";
    char buf[32];
    LONG n;

    ASSERT_EQ(amiport_emu_pipe(pipefd), 0);

    /* Write to write end */
    n = amiport_write(pipefd[1], msg, strlen(msg));
    ASSERT_EQ(n, (LONG)strlen(msg));

    /* Read from read end */
    memset(buf, 0, sizeof(buf));
    n = amiport_read(pipefd[0], buf, sizeof(buf) - 1);
    ASSERT(n > 0);
    buf[n] = '\0';
    ASSERT_STR_EQ(buf, msg);

    amiport_close(pipefd[0]);
    amiport_close(pipefd[1]);
}

TEST(pipe_null_arg)
{
    int rc = amiport_emu_pipe(NULL);
    ASSERT_EQ(rc, -1);
}

int main(void)
{
    (void)verstag;
    printf("=== Pipe Emulation Tests ===\n");

    RUN_TEST(pipe_create);
    RUN_TEST(pipe_write_read);
    RUN_TEST(pipe_null_arg);

    return test_summary();
}
