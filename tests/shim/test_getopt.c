/*
 * test_getopt.c — Tests for amiport getopt implementation
 */

#include "test_framework.h"
#include <amiport/getopt.h>

static const char *verstag = "$VER: test_getopt 1.0 (19.03.2026)";
long __stack = 32768;

/* Reset getopt state between tests */
static void reset_getopt(void)
{
    amiport_optind = 1;
    amiport_opterr = 0; /* suppress error output during tests */
    amiport_optarg = NULL;
    amiport_optopt = '?';
}

TEST(single_option)
{
    char *argv[] = {"prog", "-l", NULL};
    int argc = 2;
    int ch;

    reset_getopt();
    ch = amiport_getopt(argc, argv, "lwc");
    ASSERT_EQ(ch, 'l');

    ch = amiport_getopt(argc, argv, "lwc");
    ASSERT_EQ(ch, -1);
}

TEST(multiple_options)
{
    char *argv[] = {"prog", "-l", "-w", "-c", NULL};
    int argc = 4;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), 'l');
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), 'w');
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), 'c');
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), -1);
}

TEST(bundled_options)
{
    char *argv[] = {"prog", "-lwc", NULL};
    int argc = 2;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), 'l');
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), 'w');
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), 'c');
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), -1);
}

TEST(option_with_argument)
{
    char *argv[] = {"prog", "-o", "output.txt", NULL};
    int argc = 3;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "o:"), 'o');
    ASSERT_NOT_NULL(amiport_optarg);
    ASSERT_STR_EQ(amiport_optarg, "output.txt");
}

TEST(option_with_attached_argument)
{
    char *argv[] = {"prog", "-ooutput.txt", NULL};
    int argc = 2;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "o:"), 'o');
    ASSERT_NOT_NULL(amiport_optarg);
    ASSERT_STR_EQ(amiport_optarg, "output.txt");
}

TEST(unknown_option)
{
    char *argv[] = {"prog", "-x", NULL};
    int argc = 2;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), '?');
}

TEST(missing_argument)
{
    char *argv[] = {"prog", "-o", NULL};
    int argc = 2;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "o:"), '?');
}

TEST(double_dash_terminator)
{
    char *argv[] = {"prog", "--", "-l", NULL};
    int argc = 3;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), -1);
    /* optind should point past "--" to the non-option arg */
    ASSERT_EQ(amiport_optind, 2);
}

TEST(no_options)
{
    char *argv[] = {"prog", "file.txt", NULL};
    int argc = 2;

    reset_getopt();
    ASSERT_EQ(amiport_getopt(argc, argv, "lwc"), -1);
    ASSERT_EQ(amiport_optind, 1);
}

int main(void)
{
    (void)verstag;
    printf("=== Getopt Tests ===\n");

    RUN_TEST(single_option);
    RUN_TEST(multiple_options);
    RUN_TEST(bundled_options);
    RUN_TEST(option_with_argument);
    RUN_TEST(option_with_attached_argument);
    RUN_TEST(unknown_option);
    RUN_TEST(missing_argument);
    RUN_TEST(double_dash_terminator);
    RUN_TEST(no_options);

    return test_summary();
}
