/*
 * test_libhubbub.c -- unit tests for lib/libhubbub
 *
 * Library built -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE per
 * NetSurf-Vampire dep stack convention. Depends on libparserutils.
 * Run via: vamos -C 68040 -s 1024 -m 4096 ./test_libhubbub
 *
 * Coverage focus: tokeniser + parser entry points, charset detection,
 * entity bsearch. Treebuilder requires a 17-callback fake tree handler
 * which is too heavy for unit tests -- coverage of that subsystem comes
 * from upstream's JSON-based test corpus and from real NetSurf parsing.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <hubbub/hubbub.h>
#include <hubbub/parser.h>
#include <hubbub/types.h>
#include <hubbub/tree.h>

#include "test_framework.h"

/* libhubbub test binary needs 512 KB minimum for stack and __MEMORY_STEP.
 * 256 KB caused libnix startup to fail with Illegal Instruction (exc_num=04
 * at PC=0x01e16c, inside libnix internal stdio init) when hubbub_parser_create
 * is referenced -- forcing the linker to pull in the full hubbub + parserutils
 * + codec subsystem. The bigger binary + more globals push libnix's startup
 * allocation requirement past 256 KB. */
long __stack = 524288;
unsigned long __MEMORY_STEP = 524288;

/* ===================================================================
 * Token-handler scaffold for parser tests
 * =================================================================== */

typedef struct {
    int total;
    int doctypes;
    int start_tags;
    int end_tags;
    int comments;
    int characters;
    int eofs;
    /* Last-seen content captured for spot checks */
    char last_tag[32];
    char last_comment[64];
} token_counts;

static hubbub_error counting_token_handler(const hubbub_token *token, void *pw)
{
    token_counts *c = (token_counts *)pw;
    c->total++;
    switch (token->type) {
    case HUBBUB_TOKEN_DOCTYPE:
        c->doctypes++;
        break;
    case HUBBUB_TOKEN_START_TAG: {
        c->start_tags++;
        size_t n = token->data.tag.name.len;
        if (n >= sizeof(c->last_tag)) n = sizeof(c->last_tag) - 1;
        memcpy(c->last_tag, token->data.tag.name.ptr, n);
        c->last_tag[n] = '\0';
        break;
    }
    case HUBBUB_TOKEN_END_TAG: {
        c->end_tags++;
        size_t n = token->data.tag.name.len;
        if (n >= sizeof(c->last_tag)) n = sizeof(c->last_tag) - 1;
        memcpy(c->last_tag, token->data.tag.name.ptr, n);
        c->last_tag[n] = '\0';
        break;
    }
    case HUBBUB_TOKEN_COMMENT: {
        c->comments++;
        size_t n = token->data.comment.len;
        if (n >= sizeof(c->last_comment)) n = sizeof(c->last_comment) - 1;
        memcpy(c->last_comment, token->data.comment.ptr, n);
        c->last_comment[n] = '\0';
        break;
    }
    case HUBBUB_TOKEN_CHARACTER:
        c->characters++;
        break;
    case HUBBUB_TOKEN_EOF:
        c->eofs++;
        break;
    }
    return HUBBUB_OK;
}

static void reset_counts(token_counts *c)
{
    memset(c, 0, sizeof(*c));
}

static hubbub_error feed_html(const char *html, token_counts *c)
{
    hubbub_parser *parser = NULL;
    hubbub_error rc = hubbub_parser_create("UTF-8", false, &parser);
    if (rc != HUBBUB_OK) return rc;

    hubbub_parser_optparams params;
    params.token_handler.handler = counting_token_handler;
    params.token_handler.pw = c;
    rc = hubbub_parser_setopt(parser, HUBBUB_PARSER_TOKEN_HANDLER, &params);
    if (rc != HUBBUB_OK) { hubbub_parser_destroy(parser); return rc; }

    rc = hubbub_parser_parse_chunk(parser, (const uint8_t *)html, strlen(html));
    if (rc != HUBBUB_OK) { hubbub_parser_destroy(parser); return rc; }

    rc = hubbub_parser_completed(parser);
    hubbub_parser_destroy(parser);
    return rc;
}

/* ===================================================================
 * Category 1: Parser lifecycle
 * =================================================================== */

TEST(parser_create_destroy) {
    hubbub_parser *parser = NULL;
    ASSERT_EQ(hubbub_parser_create("UTF-8", false, &parser), HUBBUB_OK);
    ASSERT_NOT_NULL(parser);
    ASSERT_EQ(hubbub_parser_destroy(parser), HUBBUB_OK);
}

TEST(parser_create_unknown_charset_fails) {
    hubbub_parser *parser = NULL;
    hubbub_error rc = hubbub_parser_create("INVALID-XYZ", false, &parser);
    ASSERT(rc != HUBBUB_OK);
    /* parser may or may not be set; if non-NULL the caller would leak */
}

TEST(parser_create_null_charset_uses_default) {
    /* NULL charset should be acceptable -- parser uses charset detection */
    hubbub_parser *parser = NULL;
    hubbub_error rc = hubbub_parser_create(NULL, false, &parser);
    ASSERT_EQ(rc, HUBBUB_OK);
    ASSERT_NOT_NULL(parser);
    hubbub_parser_destroy(parser);
}

TEST(parser_create_null_out_fails) {
    /* Note: hubbub_parser_create may have varying validation. Test that
     * passing NULL doesn't crash. */
    hubbub_error rc = hubbub_parser_create("UTF-8", false, NULL);
    ASSERT(rc != HUBBUB_OK);
}

TEST(parser_setopt_token_handler) {
    hubbub_parser *parser = NULL;
    ASSERT_EQ(hubbub_parser_create("UTF-8", false, &parser), HUBBUB_OK);
    token_counts c;
    reset_counts(&c);
    hubbub_parser_optparams params;
    params.token_handler.handler = counting_token_handler;
    params.token_handler.pw = &c;
    ASSERT_EQ(hubbub_parser_setopt(parser, HUBBUB_PARSER_TOKEN_HANDLER,
              &params), HUBBUB_OK);
    hubbub_parser_destroy(parser);
}

TEST(parser_setopt_pause_unpause) {
    hubbub_parser *parser = NULL;
    ASSERT_EQ(hubbub_parser_create("UTF-8", false, &parser), HUBBUB_OK);
    hubbub_parser_optparams params;
    params.pause_parse = true;
    ASSERT_EQ(hubbub_parser_setopt(parser, HUBBUB_PARSER_PAUSE, &params),
              HUBBUB_OK);
    params.pause_parse = false;
    ASSERT_EQ(hubbub_parser_setopt(parser, HUBBUB_PARSER_PAUSE, &params),
              HUBBUB_OK);
    hubbub_parser_destroy(parser);
}

/* ===================================================================
 * Category 2: HTML tokenisation roundtrips
 * =================================================================== */

TEST(parse_simple_text) {
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("hello", &c), HUBBUB_OK);
    /* "hello" -> 5 character tokens (or fewer if coalesced) + 1 EOF.
     * The exact character-token count depends on the tokeniser's coalescing
     * behaviour; verify at least 1 character token + 1 EOF. */
    ASSERT(c.characters >= 1);
    ASSERT_EQ(c.eofs, 1);
    ASSERT_EQ(c.start_tags, 0);
    ASSERT_EQ(c.end_tags, 0);
}

TEST(parse_simple_tag) {
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("<p>hello</p>", &c), HUBBUB_OK);
    ASSERT_EQ(c.start_tags, 1);
    ASSERT_EQ(c.end_tags, 1);
    ASSERT_STR_EQ(c.last_tag, "p");
    ASSERT(c.characters >= 1);
    ASSERT_EQ(c.eofs, 1);
}

TEST(parse_doctype) {
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("<!DOCTYPE html><html></html>", &c), HUBBUB_OK);
    ASSERT_EQ(c.doctypes, 1);
    ASSERT_EQ(c.start_tags, 1);
    ASSERT_EQ(c.end_tags, 1);
    ASSERT_STR_EQ(c.last_tag, "html");
    ASSERT_EQ(c.eofs, 1);
}

TEST(parse_comment) {
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("<!--hello world-->", &c), HUBBUB_OK);
    ASSERT_EQ(c.comments, 1);
    ASSERT_STR_EQ(c.last_comment, "hello world");
    ASSERT_EQ(c.eofs, 1);
}

TEST(parse_self_closing_tag) {
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("<br/>", &c), HUBBUB_OK);
    ASSERT_EQ(c.start_tags, 1);
    ASSERT_STR_EQ(c.last_tag, "br");
    ASSERT_EQ(c.eofs, 1);
}

TEST(parse_attribute) {
    token_counts c;
    reset_counts(&c);
    /* This test verifies that attributed tags don't crash and produce
     * a start tag. Detailed attribute inspection would require a custom
     * handler that examines the token's attribute array. */
    ASSERT_EQ(feed_html("<a href=\"http://example.com/\">link</a>", &c),
              HUBBUB_OK);
    ASSERT_EQ(c.start_tags, 1);
    ASSERT_EQ(c.end_tags, 1);
    ASSERT_STR_EQ(c.last_tag, "a");
}

TEST(parse_nested_tags) {
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("<div><p><span>x</span></p></div>", &c), HUBBUB_OK);
    ASSERT_EQ(c.start_tags, 3);
    ASSERT_EQ(c.end_tags, 3);
    ASSERT(c.characters >= 1);
}

TEST(parse_named_entity) {
    /* &amp; -> 1 character token containing '&' */
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("&amp;", &c), HUBBUB_OK);
    ASSERT(c.characters >= 1);
    ASSERT_EQ(c.eofs, 1);
}

TEST(parse_numeric_entity) {
    /* &#65; -> 'A' */
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("&#65;", &c), HUBBUB_OK);
    ASSERT(c.characters >= 1);
    ASSERT_EQ(c.eofs, 1);
}

TEST(parse_chunked_input) {
    /* Feed input in two separate chunks; verify tokeniser stitches across */
    hubbub_parser *parser = NULL;
    ASSERT_EQ(hubbub_parser_create("UTF-8", false, &parser), HUBBUB_OK);
    token_counts c;
    reset_counts(&c);
    hubbub_parser_optparams params;
    params.token_handler.handler = counting_token_handler;
    params.token_handler.pw = &c;
    ASSERT_EQ(hubbub_parser_setopt(parser, HUBBUB_PARSER_TOKEN_HANDLER,
              &params), HUBBUB_OK);
    /* Split tag boundary across chunks */
    ASSERT_EQ(hubbub_parser_parse_chunk(parser, (const uint8_t *)"<di", 3),
              HUBBUB_OK);
    ASSERT_EQ(hubbub_parser_parse_chunk(parser, (const uint8_t *)"v>x</div>", 9),
              HUBBUB_OK);
    ASSERT_EQ(hubbub_parser_completed(parser), HUBBUB_OK);
    ASSERT_EQ(c.start_tags, 1);
    ASSERT_EQ(c.end_tags, 1);
    ASSERT_STR_EQ(c.last_tag, "div");
    hubbub_parser_destroy(parser);
}

/* ===================================================================
 * Category 3: Tokeniser direct API
 * =================================================================== */

TEST(tokeniser_via_parser_run) {
    /* hubbub_tokeniser_create requires a parserutils_inputstream argument;
     * the parser API wraps that. Just verify the parser-driven path works
     * (already covered by feed_html-based tests above). */
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html("<x>", &c), HUBBUB_OK);
    ASSERT_EQ(c.start_tags, 1);
    ASSERT_STR_EQ(c.last_tag, "x");
}

/* Charset detection (hubbub_charset_extract) and named-entity bsearch
 * (hubbub_entities_search_step) are exercised INDIRECTLY by the parser
 * tests above (parse_named_entity / parse_numeric_entity / the realistic
 * page in stress tests) plus the parser_create("UTF-8") path which
 * exercises charset alias lookup. They're declared in INTERNAL headers
 * (src/charset/detect.h, src/tokeniser/entities.h) so we don't add
 * direct unit tests for them -- doing so would couple the test suite
 * to private API surface that may change. End-to-end coverage via the
 * public hubbub_parser_* API is sufficient. */

/* ===================================================================
 * Category 6: Stress / real-world
 * =================================================================== */

TEST(stress_parse_realistic_page) {
    /* A small but realistic HTML page */
    const char *html =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<title>Test Page</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Hello, World!</h1>\n"
        "<p>This is a <strong>test</strong> paragraph with "
        "<a href=\"http://example.com/\">a link</a>.</p>\n"
        "<ul>\n"
        "  <li>Item 1</li>\n"
        "  <li>Item 2</li>\n"
        "  <li>Item 3</li>\n"
        "</ul>\n"
        "<!-- a comment -->\n"
        "</body>\n"
        "</html>\n";
    token_counts c;
    reset_counts(&c);
    ASSERT_EQ(feed_html(html, &c), HUBBUB_OK);
    ASSERT_EQ(c.doctypes, 1);
    ASSERT(c.start_tags >= 10);  /* html, head, meta, title, body, h1, p, strong, a, ul, li(x3) ... */
    ASSERT(c.end_tags >= 7);     /* end tags for non-self-closing */
    ASSERT_EQ(c.comments, 1);
    ASSERT_EQ(c.eofs, 1);
}

TEST(stress_many_chunks) {
    /* Feed a moderately large input one byte at a time -- exercises
     * tokeniser state machine across chunk boundaries. */
    const char *html = "<html><head><title>x</title></head>"
                       "<body><p>hello</p></body></html>";
    hubbub_parser *parser = NULL;
    ASSERT_EQ(hubbub_parser_create("UTF-8", false, &parser), HUBBUB_OK);
    token_counts c;
    reset_counts(&c);
    hubbub_parser_optparams params;
    params.token_handler.handler = counting_token_handler;
    params.token_handler.pw = &c;
    ASSERT_EQ(hubbub_parser_setopt(parser, HUBBUB_PARSER_TOKEN_HANDLER,
              &params), HUBBUB_OK);
    int i;
    int n = (int)strlen(html);
    for (i = 0; i < n; i++) {
        ASSERT_EQ(hubbub_parser_parse_chunk(parser,
                  (const uint8_t *)html + i, 1), HUBBUB_OK);
    }
    ASSERT_EQ(hubbub_parser_completed(parser), HUBBUB_OK);
    /* HTML: <html><head><title>x</title></head><body><p>hello</p></body></html>
     * Start tags: html, head, title, body, p = 5
     * End tags: title, head, p, body, html = 5 */
    ASSERT_EQ(c.start_tags, 5);
    ASSERT_EQ(c.end_tags, 5);
    ASSERT_EQ(c.eofs, 1);
    hubbub_parser_destroy(parser);
}

/* ===================================================================
 * Category 7: Amiga-specific
 * =================================================================== */

TEST(amiga_parser_lifecycle_no_leak) {
    /* Create + destroy 50 parsers in a loop. If allocator hooks were
     * misbehaving (returning bogus pointers, leaking on destroy), we'd
     * exhaust memory or crash by iteration ~30. */
    int i;
    for (i = 0; i < 50; i++) {
        hubbub_parser *parser = NULL;
        ASSERT_EQ(hubbub_parser_create("UTF-8", false, &parser), HUBBUB_OK);
        ASSERT_NOT_NULL(parser);
        ASSERT_EQ(hubbub_parser_destroy(parser), HUBBUB_OK);
    }
}

/* ===================================================================
 * Test runner
 * =================================================================== */

int main(void) {
    printf("=== lib/libhubbub unit tests ===\n");

    /* Category 1: Parser lifecycle */
    RUN_TEST(parser_create_destroy);
    RUN_TEST(parser_create_unknown_charset_fails);
    RUN_TEST(parser_create_null_charset_uses_default);
    RUN_TEST(parser_create_null_out_fails);
    RUN_TEST(parser_setopt_token_handler);
    RUN_TEST(parser_setopt_pause_unpause);

    /* Category 2: HTML tokenisation */
    RUN_TEST(parse_simple_text);
    RUN_TEST(parse_simple_tag);
    RUN_TEST(parse_doctype);
    RUN_TEST(parse_comment);
    RUN_TEST(parse_self_closing_tag);
    RUN_TEST(parse_attribute);
    RUN_TEST(parse_nested_tags);
    RUN_TEST(parse_named_entity);
    RUN_TEST(parse_numeric_entity);
    RUN_TEST(parse_chunked_input);

    /* Category 3: Tokeniser direct (covered indirectly) */
    RUN_TEST(tokeniser_via_parser_run);

    /* Charset detection + entity bsearch covered indirectly by Category 2 */

    /* Category 6: Stress */
    RUN_TEST(stress_parse_realistic_page);
    RUN_TEST(stress_many_chunks);

    /* Category 7: Amiga-specific */
    RUN_TEST(amiga_parser_lifecycle_no_leak);

    return test_summary();
}
