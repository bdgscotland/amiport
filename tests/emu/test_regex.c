/*
 * test_regex.c — Tests for amiport Tier 2 regex emulation
 */

#include "test_framework.h"
#define AMIPORT_NO_REGEX_MACROS
#include <amiport-emu/regex.h>
#include <stdio.h>
#include <string.h>

static const char *verstag = "$VER: test_regex 1.0 (20.03.2026)";
long __stack = 32768;

/* --- Basic literal matching --- */

TEST(literal_match)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "hello", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "say hello world", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

TEST(literal_no_match)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "xyz", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "hello world", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

/* --- Dot (any character) --- */

TEST(dot_matches_any)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "h.llo", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "hello", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "hallo", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "hllo", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

/* --- Anchors --- */

TEST(anchor_bol)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "^hello", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "hello world", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "say hello", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

TEST(anchor_eol)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "world$", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "hello world", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "world hello", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

/* --- Character classes --- */

TEST(char_class_basic)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "[aeiou]", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "hello", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "gym", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

TEST(char_class_range)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "[0-9]+", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abc123", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abc", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

TEST(char_class_negated)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "[^0-9]", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abc", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "123", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

/* --- Quantifiers --- */

TEST(star_quantifier)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "ab*c", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "ac", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abc", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abbbbc", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

TEST(plus_quantifier)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "ab+c", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "ac", 0, NULL, 0), REG_NOMATCH);
    ASSERT_EQ(amiport_emu_regexec(&re, "abc", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abbbbc", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

TEST(question_quantifier)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "colou?r", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "color", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "colour", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

/* --- Alternation --- */

TEST(alternation)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "cat|dog", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "I have a cat", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "I have a dog", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "I have a bird", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

/* --- Capture groups and match positions --- */

TEST(capture_group_positions)
{
    amiport_emu_regex_t re;
    amiport_emu_regmatch_t matches[3];

    ASSERT_EQ(amiport_emu_regcomp(&re, "(foo)(bar)", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "xfoobar", 3, matches, 0), 0);

    /* Full match */
    ASSERT_EQ(matches[0].rm_so, 1);
    ASSERT_EQ(matches[0].rm_eo, 7);

    /* Group 1: foo */
    ASSERT_EQ(matches[1].rm_so, 1);
    ASSERT_EQ(matches[1].rm_eo, 4);

    /* Group 2: bar */
    ASSERT_EQ(matches[2].rm_so, 4);
    ASSERT_EQ(matches[2].rm_eo, 7);

    amiport_emu_regfree(&re);
}

/* --- Case insensitive --- */

TEST(icase_matching)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "hello", REG_EXTENDED | REG_ICASE), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "HELLO", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "Hello", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

/* --- REG_NOSUB --- */

TEST(nosub_flag)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "test", REG_EXTENDED | REG_NOSUB), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "this is a test", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

/* --- Error handling --- */

TEST(bad_pattern_unmatched_bracket)
{
    amiport_emu_regex_t re;
    int rc = amiport_emu_regcomp(&re, "[abc", REG_EXTENDED);
    ASSERT(rc != 0);
}

TEST(bad_pattern_unmatched_paren)
{
    amiport_emu_regex_t re;
    int rc = amiport_emu_regcomp(&re, "(abc", REG_EXTENDED);
    ASSERT(rc != 0);
}

TEST(regerror_message)
{
    char buf[64];
    size_t len;

    len = amiport_emu_regerror(REG_NOMATCH, NULL, buf, sizeof(buf));
    ASSERT(len > 0);
    ASSERT_STR_EQ(buf, "No match");
}

/* --- Escaped characters --- */

TEST(escaped_chars)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "a\\.b", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "a.b", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "axb", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

/* --- BRE mode --- */

TEST(bre_star)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "ab*c", 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "ac", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abc", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "abbbbc", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

/* --- Empty string edge case --- */

TEST(empty_pattern)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "anything", 0, NULL, 0), 0);
    amiport_emu_regfree(&re);
}

TEST(empty_string)
{
    amiport_emu_regex_t re;
    ASSERT_EQ(amiport_emu_regcomp(&re, "^$", REG_EXTENDED), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "", 0, NULL, 0), 0);
    ASSERT_EQ(amiport_emu_regexec(&re, "notempty", 0, NULL, 0), REG_NOMATCH);
    amiport_emu_regfree(&re);
}

int main(void)
{
    (void)verstag;
    printf("=== Regex Emulation Tests ===\n");

    RUN_TEST(literal_match);
    RUN_TEST(literal_no_match);
    RUN_TEST(dot_matches_any);
    RUN_TEST(anchor_bol);
    RUN_TEST(anchor_eol);
    RUN_TEST(char_class_basic);
    RUN_TEST(char_class_range);
    RUN_TEST(char_class_negated);
    RUN_TEST(star_quantifier);
    RUN_TEST(plus_quantifier);
    RUN_TEST(question_quantifier);
    RUN_TEST(alternation);
    RUN_TEST(capture_group_positions);
    RUN_TEST(icase_matching);
    RUN_TEST(nosub_flag);
    RUN_TEST(bad_pattern_unmatched_bracket);
    RUN_TEST(bad_pattern_unmatched_paren);
    RUN_TEST(regerror_message);
    RUN_TEST(escaped_chars);
    RUN_TEST(bre_star);
    RUN_TEST(empty_pattern);
    RUN_TEST(empty_string);

    return test_summary();
}
