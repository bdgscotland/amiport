/*
 * test_signal.c — Tests for amiport signal emulation
 */

#include "test_framework.h"
#include <amiport/signal.h>

static const char *verstag = "$VER: test_signal 1.0 (19.03.2026)";
long __stack = 32768;

static int handler_called = 0;
static int handler_signum = 0;

static void test_handler(int sig)
{
    handler_called = 1;
    handler_signum = sig;
}

TEST(set_and_get_handler)
{
    amiport_sighandler_t old;

    old = amiport_signal(AMIPORT_SIGINT, test_handler);
    ASSERT(old != AMIPORT_SIG_ERR);

    /* Set again — should return our handler */
    old = amiport_signal(AMIPORT_SIGINT, AMIPORT_SIG_DFL);
    ASSERT(old == test_handler);
}

TEST(invalid_signal)
{
    amiport_sighandler_t result;
    result = amiport_signal(0, test_handler);
    ASSERT(result == AMIPORT_SIG_ERR);

    result = amiport_signal(16, test_handler);
    ASSERT(result == AMIPORT_SIG_ERR);
}

TEST(raise_calls_handler)
{
    handler_called = 0;
    handler_signum = 0;

    amiport_signal(AMIPORT_SIGTERM, test_handler);
    amiport_raise(AMIPORT_SIGTERM);

    ASSERT_EQ(handler_called, 1);
    ASSERT_EQ(handler_signum, AMIPORT_SIGTERM);

    /* Reset */
    amiport_signal(AMIPORT_SIGTERM, AMIPORT_SIG_DFL);
}

TEST(raise_sig_ign)
{
    handler_called = 0;

    amiport_signal(AMIPORT_SIGHUP, AMIPORT_SIG_IGN);
    ASSERT_EQ(amiport_raise(AMIPORT_SIGHUP), 0);
    ASSERT_EQ(handler_called, 0);

    amiport_signal(AMIPORT_SIGHUP, AMIPORT_SIG_DFL);
}

TEST(raise_sig_dfl)
{
    handler_called = 0;

    amiport_signal(AMIPORT_SIGQUIT, AMIPORT_SIG_DFL);
    ASSERT_EQ(amiport_raise(AMIPORT_SIGQUIT), 0);
    ASSERT_EQ(handler_called, 0);
}

TEST(raise_invalid)
{
    ASSERT_EQ(amiport_raise(0), -1);
    ASSERT_EQ(amiport_raise(16), -1);
}

TEST(check_break_no_pending)
{
    /* Should return 0 when no Ctrl-C is pending */
    ASSERT_EQ(amiport_check_break(), 0);
}

int main(void)
{
    (void)verstag;
    printf("=== Signal Tests ===\n");

    RUN_TEST(set_and_get_handler);
    RUN_TEST(invalid_signal);
    RUN_TEST(raise_calls_handler);
    RUN_TEST(raise_sig_ign);
    RUN_TEST(raise_sig_dfl);
    RUN_TEST(raise_invalid);
    RUN_TEST(check_break_no_pending);

    return test_summary();
}
