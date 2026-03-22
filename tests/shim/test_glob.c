/*
 * test_glob.c — Tests for amiport glob/globfree
 *
 * Creates temp files in T:glob_test/ and verifies glob expansion.
 */

#include "test_framework.h"
#include <amiport/glob.h>
#include <amiport/dirent.h>
#include <amiport/unistd.h>
#include <string.h>

/* From glob.c */
extern int amiport_glob_has_magic(const char *pattern);

static const char *verstag = "$VER: test_glob 1.0 (21.03.2026)";
long __stack = 32768;

#define TEST_DIR "T:glob_test"

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
	create_file(TEST_DIR "/foo.c");
	create_file(TEST_DIR "/bar.c");
	create_file(TEST_DIR "/baz.h");
	create_file(TEST_DIR "/readme.txt");
}

static void teardown(void)
{
	amiport_unlink(TEST_DIR "/foo.c");
	amiport_unlink(TEST_DIR "/bar.c");
	amiport_unlink(TEST_DIR "/baz.h");
	amiport_unlink(TEST_DIR "/readme.txt");
	amiport_rmdir(TEST_DIR);
}

/* --- glob_has_magic tests --- */

TEST(magic_star)
{
	ASSERT_EQ(amiport_glob_has_magic("*.c"), 1);
}

TEST(magic_question)
{
	ASSERT_EQ(amiport_glob_has_magic("?.c"), 1);
}

TEST(magic_bracket)
{
	ASSERT_EQ(amiport_glob_has_magic("[abc].c"), 1);
}

TEST(magic_amiga_hashq)
{
	ASSERT_EQ(amiport_glob_has_magic("#?.c"), 1);
}

TEST(magic_amiga_tilde)
{
	ASSERT_EQ(amiport_glob_has_magic("~(test)"), 1);
}

TEST(magic_amiga_alternation)
{
	ASSERT_EQ(amiport_glob_has_magic("(foo|bar)"), 1);
}

TEST(magic_literal)
{
	ASSERT_EQ(amiport_glob_has_magic("hello.txt"), 0);
}

TEST(magic_null)
{
	ASSERT_EQ(amiport_glob_has_magic(NULL), 0);
}

TEST(magic_empty)
{
	ASSERT_EQ(amiport_glob_has_magic(""), 0);
}

/* --- glob() tests --- */

TEST(glob_star_c)
{
	amiport_glob_t g;
	int rc;
	int found_foo = 0, found_bar = 0;
	size_t i;

	setup();
	rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 2);

	for (i = 0; i < g.gl_pathc; i++) {
		if (strstr(g.gl_pathv[i], "foo.c")) found_foo = 1;
		if (strstr(g.gl_pathv[i], "bar.c")) found_bar = 1;
	}
	ASSERT(found_foo);
	ASSERT(found_bar);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_sorted)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 2);

	/* Results should be sorted: bar.c before foo.c */
	ASSERT(strstr(g.gl_pathv[0], "bar.c") != NULL);
	ASSERT(strstr(g.gl_pathv[1], "foo.c") != NULL);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_nosort)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/*.c", GLOB_NOSORT, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 2);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_nomatch)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/*.zzz", 0, NULL, &g);
	ASSERT_EQ(rc, GLOB_NOMATCH);

	teardown();
}

TEST(glob_nocheck)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/*.zzz", GLOB_NOCHECK, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 1);
	ASSERT(strstr(g.gl_pathv[0], "*.zzz") != NULL);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_append)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 2);

	rc = amiport_glob(TEST_DIR "/*.h", GLOB_APPEND, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 3);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_err_bad_dir)
{
	amiport_glob_t g;
	int rc;

	rc = amiport_glob("T:nonexistent_glob_dir_999/*.c", GLOB_ERR, NULL, &g);
	ASSERT_EQ(rc, GLOB_ABORTED);
}

TEST(glob_amiga_hashq)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/#?.c", 0, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 2);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_question_mark)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/???.c", 0, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 2);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_literal)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/foo.c", 0, NULL, &g);
	ASSERT_EQ(rc, 0);
	ASSERT_EQ(g.gl_pathc, 1);
	ASSERT(strstr(g.gl_pathv[0], "foo.c") != NULL);

	amiport_globfree(&g);
	teardown();
}

TEST(glob_literal_noexist)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/nope.c", 0, NULL, &g);
	ASSERT_EQ(rc, GLOB_NOMATCH);

	teardown();
}

TEST(globfree_null_safe)
{
	amiport_glob_t g;
	g.gl_pathc = 0;
	g.gl_pathv = NULL;
	amiport_globfree(&g);
	amiport_globfree(NULL);
}

TEST(globfree_double_free)
{
	amiport_glob_t g;
	int rc;

	setup();
	rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
	ASSERT_EQ(rc, 0);

	amiport_globfree(&g);
	ASSERT_NULL(g.gl_pathv);
	ASSERT_EQ(g.gl_pathc, 0);

	teardown();
}

int main(void)
{
	(void)verstag;
	printf("=== Glob Tests ===\n");

	/* glob_has_magic */
	RUN_TEST(magic_star);
	RUN_TEST(magic_question);
	RUN_TEST(magic_bracket);
	RUN_TEST(magic_amiga_hashq);
	RUN_TEST(magic_amiga_tilde);
	RUN_TEST(magic_amiga_alternation);
	RUN_TEST(magic_literal);
	RUN_TEST(magic_null);
	RUN_TEST(magic_empty);

	/* glob() */
	RUN_TEST(glob_star_c);
	RUN_TEST(glob_sorted);
	RUN_TEST(glob_nosort);
	RUN_TEST(glob_nomatch);
	RUN_TEST(glob_nocheck);
	RUN_TEST(glob_append);
	RUN_TEST(glob_err_bad_dir);
	RUN_TEST(glob_amiga_hashq);
	RUN_TEST(glob_question_mark);
	RUN_TEST(glob_literal);
	RUN_TEST(glob_literal_noexist);

	/* globfree() */
	RUN_TEST(globfree_null_safe);
	RUN_TEST(globfree_double_free);

	return test_summary();
}
