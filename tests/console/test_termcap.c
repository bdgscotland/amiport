/*
 * test_termcap.c — Tests for termcap API in console-shim
 *
 * Tests tgetent, tgetnum, tgetflag, tgetstr, tgoto, tparm.
 * Does NOT call initscr() — these are pure lookup functions
 * that run on vamos without a real console.
 */

#include <amiport-console/term.h>
#include "../common/test_framework.h"

#include <string.h>

/* --- tgetent --- */

TEST(tgetent_succeeds)
{
    char buf[1024];
    ASSERT_EQ(tgetent(buf, "amiga"), 1);
}

TEST(tgetent_null_args)
{
    ASSERT_EQ(tgetent(NULL, NULL), 1);
}

/* --- tgetnum --- */

TEST(tgetnum_columns)
{
    int co = tgetnum("co");
    ASSERT(co > 0);
    ASSERT(co <= 200);  /* reasonable range */
}

TEST(tgetnum_lines)
{
    int li = tgetnum("li");
    ASSERT(li > 0);
    ASSERT(li <= 100);
}

TEST(tgetnum_unknown)
{
    ASSERT_EQ(tgetnum("zz"), -1);
}

TEST(tgetnum_null)
{
    ASSERT_EQ(tgetnum(NULL), -1);
}

/* --- tgetflag --- */

TEST(tgetflag_am)
{
    ASSERT_EQ(tgetflag("am"), 1);  /* auto margins */
}

TEST(tgetflag_bs)
{
    ASSERT_EQ(tgetflag("bs"), 1);  /* backspace works */
}

TEST(tgetflag_xn)
{
    ASSERT_EQ(tgetflag("xn"), 0);  /* no newline glitch */
}

TEST(tgetflag_unknown)
{
    ASSERT_EQ(tgetflag("zz"), 0);
}

TEST(tgetflag_null)
{
    ASSERT_EQ(tgetflag(NULL), 0);
}

/* --- tgetstr --- */

TEST(tgetstr_ce)
{
    char *s = tgetstr("ce", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "\033[K");
}

TEST(tgetstr_cl)
{
    char *s = tgetstr("cl", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "\033[2J\033[H");
}

TEST(tgetstr_cd)
{
    char *s = tgetstr("cd", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "\033[J");
}

TEST(tgetstr_so)
{
    char *s = tgetstr("so", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "\033[7m");
}

TEST(tgetstr_se)
{
    char *s = tgetstr("se", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "\033[27m");
}

TEST(tgetstr_md)
{
    char *s = tgetstr("md", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "\033[1m");
}

TEST(tgetstr_me)
{
    char *s = tgetstr("me", NULL);
    ASSERT_NOT_NULL(s);
    ASSERT_STR_EQ(s, "\033[0m");
}

TEST(tgetstr_cm)
{
    char *s = tgetstr("cm", NULL);
    ASSERT_NOT_NULL(s);
    /* cm returns a parameterized string — just verify it's non-empty */
    ASSERT(strlen(s) > 0);
}

TEST(tgetstr_unknown)
{
    ASSERT_NULL(tgetstr("zz", NULL));
}

TEST(tgetstr_null)
{
    ASSERT_NULL(tgetstr(NULL, NULL));
}

/* tgetstr with area pointer */
TEST(tgetstr_area_pointer)
{
    char area_buf[256];
    char *area = area_buf;
    char *result;

    result = tgetstr("ce", &area);
    ASSERT_NOT_NULL(result);
    ASSERT(result == area_buf);  /* result points to start of area */
    ASSERT_STR_EQ(result, "\033[K");
    ASSERT(area > area_buf);  /* area pointer was advanced */
    ASSERT_EQ((long)(area - area_buf), (long)(strlen("\033[K") + 1));
}

TEST(tgetstr_area_multiple)
{
    char area_buf[256];
    char *area = area_buf;
    char *r1;
    char *r2;

    r1 = tgetstr("ce", &area);
    r2 = tgetstr("cl", &area);

    ASSERT_NOT_NULL(r1);
    ASSERT_NOT_NULL(r2);
    ASSERT(r1 != r2);  /* different addresses */
    ASSERT_STR_EQ(r1, "\033[K");
    ASSERT_STR_EQ(r2, "\033[2J\033[H");
}

/* --- tgoto --- */

TEST(tgoto_basic)
{
    char *cm = tgetstr("cm", NULL);
    char *result;
    ASSERT_NOT_NULL(cm);

    result = tgoto(cm, 10, 5);
    ASSERT_NOT_NULL(result);
    /* Row 5, Col 10 → 1-based: \033[6;11H */
    ASSERT_STR_EQ(result, "\033[6;11H");
}

TEST(tgoto_origin)
{
    char *cm = tgetstr("cm", NULL);
    char *result;
    ASSERT_NOT_NULL(cm);

    result = tgoto(cm, 0, 0);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "\033[1;1H");
}

TEST(tgoto_null)
{
    char *result = tgoto(NULL, 5, 5);
    ASSERT_STR_EQ(result, "");
}

/* --- tparm --- */

TEST(tparm_simple_decimal)
{
    char *result = tparm("\033[%dm", 1L);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "\033[1m");
}

TEST(tparm_two_params)
{
    char *result = tparm("\033[%p1%d;%p2%dH", 5L, 10L);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "\033[5;10H");
}

TEST(tparm_increment)
{
    /* %i increments both params (0-based → 1-based coords) */
    char *result = tparm("\033[%i%p1%d;%p2%dH", 5L, 10L);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "\033[6;11H");
}

TEST(tparm_literal_percent)
{
    char *result = tparm("100%%", 0L);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ(result, "100%");
}

TEST(tparm_null)
{
    char *result = tparm(NULL);
    ASSERT_STR_EQ(result, "");
}

/* --- tputs --- */

TEST(tputs_null_str)
{
    ASSERT_EQ(tputs(NULL, 1, NULL), -1);
}

TEST(tputs_not_found_str)
{
    /* tigetstr returns (char*)-1 for not found — tputs should handle */
    ASSERT_EQ(tputs((char *)-1, 1, NULL), -1);
}

int main(void)
{
    printf("=== test_termcap ===\n");

    RUN_TEST(tgetent_succeeds);
    RUN_TEST(tgetent_null_args);

    RUN_TEST(tgetnum_columns);
    RUN_TEST(tgetnum_lines);
    RUN_TEST(tgetnum_unknown);
    RUN_TEST(tgetnum_null);

    RUN_TEST(tgetflag_am);
    RUN_TEST(tgetflag_bs);
    RUN_TEST(tgetflag_xn);
    RUN_TEST(tgetflag_unknown);
    RUN_TEST(tgetflag_null);

    RUN_TEST(tgetstr_ce);
    RUN_TEST(tgetstr_cl);
    RUN_TEST(tgetstr_cd);
    RUN_TEST(tgetstr_so);
    RUN_TEST(tgetstr_se);
    RUN_TEST(tgetstr_md);
    RUN_TEST(tgetstr_me);
    RUN_TEST(tgetstr_cm);
    RUN_TEST(tgetstr_unknown);
    RUN_TEST(tgetstr_null);
    RUN_TEST(tgetstr_area_pointer);
    RUN_TEST(tgetstr_area_multiple);

    RUN_TEST(tgoto_basic);
    RUN_TEST(tgoto_origin);
    RUN_TEST(tgoto_null);

    RUN_TEST(tparm_simple_decimal);
    RUN_TEST(tparm_two_params);
    RUN_TEST(tparm_increment);
    RUN_TEST(tparm_literal_percent);
    RUN_TEST(tparm_null);

    RUN_TEST(tputs_null_str);
    RUN_TEST(tputs_not_found_str);

    return test_summary();
}
