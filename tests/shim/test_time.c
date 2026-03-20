/*
 * test_time.c — Tests for amiport time functions
 */

#include "test_framework.h"
#include <amiport/sys/time.h>

static const char *verstag = "$VER: test_time 1.0 (19.03.2026)";
long __stack = 32768;

TEST(gettimeofday_valid)
{
    struct amiport_timeval tv;

    ASSERT_EQ(amiport_gettimeofday(&tv), 0);
    /* Should be after Amiga epoch + offset (i.e., a Unix timestamp > 0) */
    ASSERT(tv.tv_sec > AMIGA_EPOCH_OFFSET);
    ASSERT(tv.tv_usec >= 0);
    ASSERT(tv.tv_usec < 1000000);
}

TEST(gettimeofday_null)
{
    ASSERT_EQ(amiport_gettimeofday(NULL), -1);
}

TEST(time_returns_value)
{
    LONG t = amiport_time(NULL);
    ASSERT(t > AMIGA_EPOCH_OFFSET);
}

TEST(time_stores_in_tloc)
{
    LONG tloc = 0;
    LONG t = amiport_time(&tloc);
    ASSERT_EQ(t, tloc);
    ASSERT(tloc > AMIGA_EPOCH_OFFSET);
}

TEST(time_monotonic)
{
    LONG t1 = amiport_time(NULL);
    LONG t2 = amiport_time(NULL);
    ASSERT(t2 >= t1);
}

int main(void)
{
    (void)verstag;
    printf("=== Time Tests ===\n");

    RUN_TEST(gettimeofday_valid);
    RUN_TEST(gettimeofday_null);
    RUN_TEST(time_returns_value);
    RUN_TEST(time_stores_in_tloc);
    RUN_TEST(time_monotonic);

    return test_summary();
}
