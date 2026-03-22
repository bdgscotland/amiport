/*
 * test_process.c — Tests for amiport process/environment functions
 */

#include "test_framework.h"
#include <amiport/unistd.h>
#include <amiport/errno_map.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const char *verstag = "$VER: test_process 1.0 (19.03.2026)";
long __stack = 32768;

TEST(getcwd_returns_nonempty)
{
    char buf[256];
    char *result = amiport_getcwd(buf, sizeof(buf));
    ASSERT_NOT_NULL(result);
    ASSERT(strlen(buf) > 0);
}

TEST(chdir_valid)
{
    char before[256];
    char after[256];

    amiport_getcwd(before, sizeof(before));

    /* T: always exists on AmigaOS */
    ASSERT_EQ(amiport_chdir("T:"), 0);
    amiport_getcwd(after, sizeof(after));

    /* Should have changed */
    ASSERT(strlen(after) > 0);

    /* Restore */
    amiport_chdir(before);
}

TEST(chdir_nonexistent)
{
    ASSERT_EQ(amiport_chdir("NONEXISTENT_VOL:xyz"), -1);
    ASSERT(errno != 0);
}

TEST(chdir_twice_no_crash)
{
    char original[256];
    amiport_getcwd(original, sizeof(original));

    /* Two consecutive chdir calls — the bug was crashing on first call
     * by UnLocking the system's initial directory lock */
    ASSERT_EQ(amiport_chdir("T:"), 0);
    ASSERT_EQ(amiport_chdir("T:"), 0);

    /* Restore */
    amiport_chdir(original);
}

TEST(getenv_nonexistent)
{
    /* Note: some vamos versions may return empty string instead of NULL
     * for nonexistent env vars. Accept either as correct. */
    char *val = amiport_getenv("AMIPORT_NONEXISTENT_VAR_99");
    if (val != NULL) {
        ASSERT_STR_EQ(val, "");
        free(val);
    }
}

TEST(getenv_returns_allocated)
{
    char *val1;
    char *val2;

    /* These may or may not exist — we just test the mechanics.
     * If they don't exist, the test for nonexistent covers it. */
    val1 = amiport_getenv("Workbench");
    val2 = amiport_getenv("Workbench");

    /* If both non-NULL, they should be independent allocations */
    if (val1 != NULL && val2 != NULL) {
        ASSERT(val1 != val2); /* Different pointers */
        ASSERT_STR_EQ(val1, val2); /* Same content */
        free(val1);
        free(val2);
    }
}

TEST(getpid_nonzero)
{
    LONG pid = amiport_getpid();
    ASSERT(pid != 0);
}

TEST(isatty_stdout)
{
    /* In vamos, stdout should be interactive */
    int result = amiport_isatty(1);
    /* This may or may not be 1 depending on vamos — just verify no crash */
    ASSERT(result == 0 || result == 1);
}

TEST(setenv_and_getenv)
{
    char *val;

    /* Set a test variable and read it back */
    ASSERT_EQ(amiport_setenv("AMIPORT_TEST_VAR", "hello", 1), 0);

    val = amiport_getenv("AMIPORT_TEST_VAR");
    ASSERT_NOT_NULL(val);
    ASSERT_STR_EQ(val, "hello");
    free(val);

    /* Clean up */
    amiport_unsetenv("AMIPORT_TEST_VAR");
}

TEST(setenv_overwrite_zero)
{
    char *val;

    /* Set initial value */
    ASSERT_EQ(amiport_setenv("AMIPORT_TEST_OW", "original", 1), 0);

    /* Attempt to overwrite with overwrite=0 — must not change */
    ASSERT_EQ(amiport_setenv("AMIPORT_TEST_OW", "changed", 0), 0);

    val = amiport_getenv("AMIPORT_TEST_OW");
    ASSERT_NOT_NULL(val);
    ASSERT_STR_EQ(val, "original");
    free(val);

    /* Clean up */
    amiport_unsetenv("AMIPORT_TEST_OW");
}

TEST(setenv_overwrite_one)
{
    char *val;

    /* Set initial value then overwrite it */
    ASSERT_EQ(amiport_setenv("AMIPORT_TEST_OW2", "first", 1), 0);
    ASSERT_EQ(amiport_setenv("AMIPORT_TEST_OW2", "second", 1), 0);

    val = amiport_getenv("AMIPORT_TEST_OW2");
    ASSERT_NOT_NULL(val);
    ASSERT_STR_EQ(val, "second");
    free(val);

    amiport_unsetenv("AMIPORT_TEST_OW2");
}

TEST(unsetenv_removes_var)
{
    char *val;

    /* Set then unset */
    ASSERT_EQ(amiport_setenv("AMIPORT_TEST_UNSET", "value", 1), 0);
    ASSERT_EQ(amiport_unsetenv("AMIPORT_TEST_UNSET"), 0);

    /* Variable must no longer exist (NULL or empty) */
    val = amiport_getenv("AMIPORT_TEST_UNSET");
    if (val != NULL) {
        /* vamos quirk: may return empty string instead of NULL */
        ASSERT_STR_EQ(val, "");
        free(val);
    }
}

TEST(unsetenv_nonexistent)
{
    /* Unsetting a nonexistent variable must succeed (POSIX requirement) */
    ASSERT_EQ(amiport_unsetenv("AMIPORT_TEST_GHOST_VAR_999"), 0);
}

int main(void)
{
    (void)verstag;
    printf("=== Process/Environment Tests ===\n");

    RUN_TEST(getcwd_returns_nonempty);
    RUN_TEST(chdir_valid);
    RUN_TEST(chdir_nonexistent);
    RUN_TEST(chdir_twice_no_crash);
    RUN_TEST(getenv_nonexistent);
    RUN_TEST(getenv_returns_allocated);
    RUN_TEST(getpid_nonzero);
    RUN_TEST(isatty_stdout);
    RUN_TEST(setenv_and_getenv);
    RUN_TEST(setenv_overwrite_zero);
    RUN_TEST(setenv_overwrite_one);
    RUN_TEST(unsetenv_removes_var);
    RUN_TEST(unsetenv_nonexistent);

    return test_summary();
}
