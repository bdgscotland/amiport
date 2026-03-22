/*
 * test_argv_expand.c — Tests for amiport argv wildcard expansion
 *
 * Tests amiport_expand_argv() and amiport_free_argv() directly.
 */

#include "test_framework.h"
#include <amiport/glob.h>
#include <amiport/dirent.h>
#include <amiport/unistd.h>
#include <string.h>

/* From glob.c */
extern int amiport_glob_has_magic(const char *pattern);

static const char *verstag = "$VER: test_argv_expand 1.0 (21.03.2026)";
long __stack = 32768;

#define TEST_DIR "T:argv_test"

static void create_file(const char *path)
{
	int fd = amiport_open(path, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd >= 0) {
		amiport_write(fd, "x", 1);
		amiport_close(fd);
	}
}

static void setup(void)
{
	amiport_mkdir(TEST_DIR, 0755);
	create_file(TEST_DIR "/alpha.c");
	create_file(TEST_DIR "/beta.c");
	create_file(TEST_DIR "/gamma.h");
}

static void teardown(void)
{
	amiport_unlink(TEST_DIR "/alpha.c");
	amiport_unlink(TEST_DIR "/beta.c");
	amiport_unlink(TEST_DIR "/gamma.h");
	amiport_rmdir(TEST_DIR);
}

TEST(no_wildcards_unchanged)
{
	char *args[] = {"prog", "file.c", "-v", NULL};
	char **argv = args;
	int argc = 3;

	amiport_expand_argv(&argc, &argv);

	ASSERT_EQ(argc, 3);
	ASSERT_STR_EQ(argv[0], "prog");
	ASSERT_STR_EQ(argv[1], "file.c");
	ASSERT_STR_EQ(argv[2], "-v");

	amiport_free_argv();
}

TEST(expand_star_c)
{
	char *args[] = {"prog", TEST_DIR "/*.c", NULL};
	char **argv = args;
	int argc = 2;
	int found_alpha = 0, found_beta = 0;
	int i;

	setup();
	amiport_expand_argv(&argc, &argv);

	ASSERT_EQ(argc, 3);
	ASSERT_STR_EQ(argv[0], "prog");
	for (i = 1; i < argc; i++) {
		if (strstr(argv[i], "alpha.c")) found_alpha = 1;
		if (strstr(argv[i], "beta.c")) found_beta = 1;
	}
	ASSERT(found_alpha);
	ASSERT(found_beta);

	amiport_free_argv();
	teardown();
}

TEST(option_args_not_expanded)
{
	char *args[] = {"prog", "-e", TEST_DIR "/*.c", NULL};
	char **argv = args;
	int argc = 3;

	setup();
	amiport_expand_argv(&argc, &argv);

	/* -e should not be expanded */
	ASSERT_STR_EQ(argv[0], "prog");
	ASSERT_STR_EQ(argv[1], "-e");
	/* argv[2..] are the expanded .c files */
	ASSERT(argc >= 3);

	amiport_free_argv();
	teardown();
}

TEST(argv0_never_expanded)
{
	char *args[] = {"prog*", TEST_DIR "/*.c", NULL};
	char **argv = args;
	int argc = 2;

	setup();
	amiport_expand_argv(&argc, &argv);

	ASSERT_STR_EQ(argv[0], "prog*");

	amiport_free_argv();
	teardown();
}

TEST(no_matches_keeps_literal)
{
	char *args[] = {"prog", TEST_DIR "/*.zzz", NULL};
	char **argv = args;
	int argc = 2;

	setup();
	amiport_expand_argv(&argc, &argv);

	/* GLOB_NOCHECK: pattern with no matches kept as literal */
	ASSERT_EQ(argc, 2);
	ASSERT(strstr(argv[1], "*.zzz") != NULL);

	amiport_free_argv();
	teardown();
}

TEST(mixed_literal_and_wildcard)
{
	char *args[] = {"prog", "literal.txt", TEST_DIR "/*.c", "-v", NULL};
	char **argv = args;
	int argc = 4;

	setup();
	amiport_expand_argv(&argc, &argv);

	/* literal.txt stays, *.c expands to 2, -v stays */
	ASSERT_EQ(argc, 5);
	ASSERT_STR_EQ(argv[0], "prog");
	ASSERT_STR_EQ(argv[1], "literal.txt");
	ASSERT_STR_EQ(argv[4], "-v");

	amiport_free_argv();
	teardown();
}

TEST(amiga_hashq_pattern)
{
	char *args[] = {"prog", TEST_DIR "/#?.c", NULL};
	char **argv = args;
	int argc = 2;

	setup();
	amiport_expand_argv(&argc, &argv);

	ASSERT_EQ(argc, 3);

	amiport_free_argv();
	teardown();
}

TEST(free_argv_safe_double_call)
{
	amiport_free_argv();
	amiport_free_argv();
}

TEST(free_argv_no_expand)
{
	amiport_free_argv();
}

TEST(null_params_safe)
{
	int argc = 1;
	char **argv = NULL;

	amiport_expand_argv(NULL, NULL);
	amiport_expand_argv(&argc, NULL);
	amiport_expand_argv(NULL, &argv);
}

int main(void)
{
	(void)verstag;
	printf("=== Argv Expand Tests ===\n");

	RUN_TEST(no_wildcards_unchanged);
	RUN_TEST(expand_star_c);
	RUN_TEST(option_args_not_expanded);
	RUN_TEST(argv0_never_expanded);
	RUN_TEST(no_matches_keeps_literal);
	RUN_TEST(mixed_literal_and_wildcard);
	RUN_TEST(amiga_hashq_pattern);
	RUN_TEST(free_argv_safe_double_call);
	RUN_TEST(free_argv_no_expand);
	RUN_TEST(null_params_safe);

	return test_summary();
}
