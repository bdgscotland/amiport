/*
 * test_alarm.c — Tests for amiport alarm() emulation
 *
 * Tests cooperative alarm checking via timer.device.
 * Note: alarm emulation is non-async (requires periodic check calls),
 * so tests verify the API contract rather than real-time behavior.
 */

#include "test_framework.h"
#include <amiport-emu/alarm.h>

TEST(alarm_init_cleanup)
{
    /* Init should succeed */
    ASSERT_EQ(amiport_emu_alarm_init(), 0);
    /* Cleanup should not crash */
    amiport_emu_alarm_cleanup();
}

TEST(alarm_set_and_cancel)
{
    unsigned int prev;

    ASSERT_EQ(amiport_emu_alarm_init(), 0);

    /* Setting alarm should return 0 (no previous alarm) */
    prev = amiport_emu_alarm(5);
    ASSERT_EQ(prev, 0);

    /* Setting new alarm should return remaining time of previous */
    prev = amiport_emu_alarm(10);
    /* prev should be > 0 (some time remaining from the 5-second alarm) */

    /* Cancel alarm by setting to 0 */
    prev = amiport_emu_alarm(0);

    /* Check should report no alarm pending */
    ASSERT_EQ(amiport_emu_check_alarm(), 0);

    amiport_emu_alarm_cleanup();
}

TEST(alarm_check_no_alarm)
{
    ASSERT_EQ(amiport_emu_alarm_init(), 0);

    /* With no alarm set, check should return 0 (not expired) */
    ASSERT_EQ(amiport_emu_check_alarm(), 0);

    amiport_emu_alarm_cleanup();
}

int main(void)
{
    printf("=== Alarm Emulation Tests ===\n\n");

    RUN_TEST(alarm_init_cleanup);
    RUN_TEST(alarm_set_and_cancel);
    RUN_TEST(alarm_check_no_alarm);

    return test_summary();
}
