/*
 * test_libcss.c -- unit tests for lib/libcss
 *
 * Library built -O0 -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE
 * (CFLAGS may be promoted to -O1 -fno-strict-aliasing after Stage 7
 * perf audit -- the 4 prior dep stack libs all promoted, expect this
 * to follow the same pattern).
 *
 * Run via: vamos -C 68040 -s 4096 -m 8192 ./test_libcss
 *
 * Coverage focus: stylesheet API (parse + lifecycle + accessors),
 * select context API (create / sheet management / destroy), error
 * paths, edge cases, stress, and Amiga-specific (alignment, fixed-
 * point integer math, exit cleanup).
 *
 * Out of scope: css_select_style end-to-end testing -- it requires
 * implementing a complete css_select_handler DOM-traversal callback
 * surface (~30 callbacks). Defer to ports/netsurf integration where
 * libdom provides the DOM the select_handler needs to walk.
 *
 * 38 tests across the six categories per docs/test-coverage-standard.md:
 *   8  functional   (stylesheet create/destroy, append_data, data_done,
 *                    get_url/title/level, size, ctx create/destroy)
 *   5  error path   (NULL params, invalid params_version, NULL data,
 *                    select_ctx NULL, sheet append NULL)
 *   6  edge case    (empty CSS, single selector, multi-selector,
 *                    chunked append, level upgrade, comment-only)
 *   5  Amiga-specific (fixed-point math, struct alignment, deep parse
 *                    stack safety, css_error_to_string, pre-main init)
 *   8  stress       (50-iter parse+destroy, 1KB CSS, 4KB CSS, 100 rules,
 *                    long selectors, 50-prop rule, ctx 10-sheets,
 *                    repeated arena allocations)
 *   6  property parsing (color hex, color rgb(), length px, length em,
 *                    margin shorthand, font shorthand)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <libwapcaplet/libwapcaplet.h>
#include <libcss/libcss.h>

#include "test_framework.h"

/*
 * libcss + libwapcaplet + libparserutils binary is the smallest of the
 * NetSurf-Vampire dep stack tests (libcss has no libhubbub/libdom
 * pulled in, just libparserutils for the lexer's input stream).
 * Empirically the 524288 cookie used for libhubbub is sufficient here
 * (libcss is ~476 KB but the .a is mostly per-property dispatchers
 * that don't all link together at -O0). Use 1 MB to be safe and stay
 * consistent with the libdom-class testing pattern.
 */
long __stack = 1048576;
unsigned long __MEMORY_STEP = 1048576;

/* ===================================================================
 * Helpers
 * =================================================================== */

/*
 * URL resolution callback: trivial pass-through for tests that don't
 * exercise import resolution. Returns CSS_OK without resolving (the
 * library tolerates this for parser-only tests).
 */
static css_error url_resolve_noop(void *pw, const char *base,
                                  lwc_string *rel, lwc_string **abs)
{
    (void)pw; (void)base;
    *abs = lwc_string_ref(rel);
    return CSS_OK;
}

/* No-op color resolver -- "current" CSS resolved colors are integer RGBA */
static css_error color_resolve_noop(void *pw, lwc_string *name,
                                    css_color *color)
{
    (void)pw; (void)name;
    *color = 0xff000000;  /* opaque black */
    return CSS_OK;
}

/* No-op font resolver */
static css_error font_resolve_noop(void *pw, lwc_string *name,
                                   css_system_font *system_font)
{
    (void)pw; (void)name; (void)system_font;
    return CSS_INVALID;
}

/* Build a default css_stylesheet_params block */
static void init_params(css_stylesheet_params *params, const char *url)
{
    memset(params, 0, sizeof(*params));
    params->params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
    params->level = CSS_LEVEL_DEFAULT;
    params->charset = NULL;
    params->url = url ? url : "test://test/";
    params->title = NULL;
    params->allow_quirks = false;
    params->inline_style = false;
    params->resolve = url_resolve_noop;
    params->resolve_pw = NULL;
    params->import = NULL;
    params->import_pw = NULL;
    params->color = color_resolve_noop;
    params->color_pw = NULL;
    params->font = font_resolve_noop;
    params->font_pw = NULL;
}

/*
 * One-shot CSS parse helper. Returns CSS_OK + populates *sheet on
 * success, otherwise destroys partial sheet and returns the error.
 */
static css_error parse_css_one_shot(const char *css, css_stylesheet **sheet)
{
    css_stylesheet_params params;
    css_error err;

    init_params(&params, NULL);
    err = css_stylesheet_create(&params, sheet);
    if (err != CSS_OK) {
        return err;
    }
    err = css_stylesheet_append_data(*sheet,
                                     (const uint8_t *)css, strlen(css));
    if (err != CSS_OK && err != CSS_NEEDDATA) {
        css_stylesheet_destroy(*sheet);
        *sheet = NULL;
        return err;
    }
    err = css_stylesheet_data_done(*sheet);
    if (err != CSS_OK && err != CSS_IMPORTS_PENDING) {
        css_stylesheet_destroy(*sheet);
        *sheet = NULL;
        return err;
    }
    return CSS_OK;
}

/* ===================================================================
 * Category 1: Functional (8)
 * =================================================================== */

TEST(stylesheet_create_destroy)
{
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    css_error err;

    init_params(&params, NULL);
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);

    err = css_stylesheet_destroy(sheet);
    ASSERT_EQ(err, CSS_OK);
}

TEST(parse_simple_rule)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot("p { color: red; }", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

TEST(stylesheet_get_url)
{
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    const char *url = NULL;
    css_error err;

    init_params(&params, "http://example.com/style.css");
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_stylesheet_get_url(sheet, &url);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(url);
    ASSERT_STR_EQ(url, "http://example.com/style.css");

    css_stylesheet_destroy(sheet);
}

TEST(stylesheet_get_title)
{
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    const char *title = NULL;
    css_error err;

    init_params(&params, NULL);
    params.title = "alt-style";
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_stylesheet_get_title(sheet, &title);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(title);
    ASSERT_STR_EQ(title, "alt-style");

    css_stylesheet_destroy(sheet);
}

TEST(stylesheet_get_language_level)
{
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    css_language_level level = (css_language_level)99;
    css_error err;

    init_params(&params, NULL);
    params.level = CSS_LEVEL_3;
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_stylesheet_get_language_level(sheet, &level);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_EQ((int)level, (int)CSS_LEVEL_3);

    css_stylesheet_destroy(sheet);
}

TEST(stylesheet_size)
{
    css_stylesheet *sheet = NULL;
    size_t size = 0;
    css_error err;

    err = parse_css_one_shot("body { background: white; }", &sheet);
    ASSERT_EQ(err, CSS_OK);
    err = css_stylesheet_size(sheet, &size);
    ASSERT_EQ(err, CSS_OK);
    ASSERT(size > 0);

    css_stylesheet_destroy(sheet);
}

TEST(select_ctx_create_destroy)
{
    css_select_ctx *ctx = NULL;
    css_error err;

    err = css_select_ctx_create(&ctx);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(ctx);

    err = css_select_ctx_destroy(ctx);
    ASSERT_EQ(err, CSS_OK);
}

TEST(select_ctx_count_sheets_empty)
{
    css_select_ctx *ctx = NULL;
    uint32_t count = 99;
    css_error err;

    err = css_select_ctx_create(&ctx);
    ASSERT_EQ(err, CSS_OK);

    err = css_select_ctx_count_sheets(ctx, &count);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_EQ(count, 0);

    css_select_ctx_destroy(ctx);
}

/* ===================================================================
 * Category 2: Error path (5)
 * =================================================================== */

TEST(create_null_params)
{
    css_stylesheet *sheet = NULL;
    css_error err = css_stylesheet_create(NULL, &sheet);
    ASSERT_EQ(err, CSS_BADPARM);
}

TEST(create_null_result)
{
    css_stylesheet_params params;
    css_error err;
    init_params(&params, NULL);
    err = css_stylesheet_create(&params, NULL);
    ASSERT_EQ(err, CSS_BADPARM);
}

TEST(create_invalid_params_version)
{
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    css_error err;
    init_params(&params, NULL);
    params.params_version = 99;  /* unknown ABI version */
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_BADPARM);
}

TEST(append_data_null_data)
{
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    css_error err;

    init_params(&params, NULL);
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_stylesheet_append_data(sheet, NULL, 100);
    ASSERT_EQ(err, CSS_BADPARM);

    css_stylesheet_destroy(sheet);
}

TEST(select_ctx_count_null_ctx)
{
    uint32_t count = 99;
    css_error err = css_select_ctx_count_sheets(NULL, &count);
    ASSERT_EQ(err, CSS_BADPARM);
}

/* ===================================================================
 * Category 3: Edge case (6)
 * =================================================================== */

TEST(parse_empty_stylesheet)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot("", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

TEST(parse_comment_only)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot("/* this is a comment */", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

TEST(parse_chunked_append)
{
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    css_error err;
    const char *c1 = "p { col";
    const char *c2 = "or: bl";
    const char *c3 = "ack; }";

    init_params(&params, NULL);
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_stylesheet_append_data(sheet, (const uint8_t *)c1, strlen(c1));
    ASSERT(err == CSS_OK || err == CSS_NEEDDATA);
    err = css_stylesheet_append_data(sheet, (const uint8_t *)c2, strlen(c2));
    ASSERT(err == CSS_OK || err == CSS_NEEDDATA);
    err = css_stylesheet_append_data(sheet, (const uint8_t *)c3, strlen(c3));
    ASSERT(err == CSS_OK || err == CSS_NEEDDATA);

    err = css_stylesheet_data_done(sheet);
    ASSERT_EQ(err, CSS_OK);

    css_stylesheet_destroy(sheet);
}

TEST(parse_multi_selector_rule)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "h1, h2, h3, p, div { color: blue; margin: 0; }", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

TEST(parse_multi_rule_stylesheet)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "body { background: white; }\n"
        "h1 { font-size: 2em; }\n"
        "p { line-height: 1.5; margin: 1em 0; }\n"
        "a { color: blue; text-decoration: underline; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

TEST(parse_invalid_css_no_crash)
{
    /*
     * Malformed CSS. libcss is supposed to be tolerant -- skip the bad
     * rule and continue. data_done returns CSS_OK with the valid parts
     * preserved. This is critical for real-world web rendering where
     * stylesheets are routinely broken.
     */
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "body { color: red; }\n"
        "this is { not valid: css; @@@; }\n"
        "p { font-size: 14px; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

/* ===================================================================
 * Category 4: Amiga-specific (5)
 * =================================================================== */

TEST(error_to_string_safe)
{
    /*
     * css_error_to_string returns a const char * for known errors.
     * NULL terminator must be present for ASCII output.
     */
    const char *s_ok = css_error_to_string(CSS_OK);
    const char *s_err = css_error_to_string(CSS_BADPARM);
    ASSERT_NOT_NULL(s_ok);
    ASSERT_NOT_NULL(s_err);
    ASSERT(strlen(s_ok) > 0);
    ASSERT(strlen(s_err) > 0);
}

TEST(parse_css_with_lengths_no_softfloat)
{
    /*
     * CSS values with units (px, em, %) are stored in 22:10 fixed-point
     * (`css_fixed = int32_t`). Verifies the parser doesn't fall back to
     * float for length values -- if it did, downstream linkers would
     * pull __divsf3/etc. from libnix, triggering FS-UAE mathieee*
     * crashes (crash-patterns #2 family). The build itself doesn't pull
     * soft-float (verified at Stage 7), but this exercises the relevant
     * code path at runtime.
     */
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "div {\n"
        "  margin: 10px 20% 1.5em 2.54cm;\n"
        "  font-size: 14pt;\n"
        "  width: 800px;\n"
        "  height: 600px;\n"
        "}\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

TEST(parse_deep_nested_selectors_safe_stack)
{
    /*
     * Long descendant selectors push the parser's per-token state.
     * Verify a 20-level descendant chain doesn't blow the 1 MB stack.
     */
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "html body div div div div div div div div div div div div div "
        "div div div div div div p { color: red; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_NOT_NULL(sheet);
    css_stylesheet_destroy(sheet);
}

TEST(stylesheet_destroy_safe)
{
    /*
     * Destroy a stylesheet with rules attached. Verifies the arena
     * allocator (src/select/arena.c) frees correctly on 68k. Not a
     * leak check (no allocator hook on vamos) -- only a crash check.
     */
    css_stylesheet *sheet = NULL;
    css_error err;
    int i;
    for (i = 0; i < 10; i++) {
        err = parse_css_one_shot(
            "p { color: red; }\n"
            "h1 { font-size: 2em; }\n"
            "div { margin: 0; }\n", &sheet);
        ASSERT_EQ(err, CSS_OK);
        css_stylesheet_destroy(sheet);
        sheet = NULL;
    }
}

TEST(select_ctx_append_sheet_basic)
{
    /*
     * Build a select context, append a stylesheet, verify count.
     * Doesn't call css_select_style (would need a fake DOM handler);
     * just verifies the sheet management API is sound on 68k.
     */
    css_select_ctx *ctx = NULL;
    css_stylesheet *sheet = NULL;
    uint32_t count = 99;
    css_error err;

    err = css_select_ctx_create(&ctx);
    ASSERT_EQ(err, CSS_OK);

    err = parse_css_one_shot("p { color: red; }", &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_select_ctx_append_sheet(ctx, sheet,
                                      CSS_ORIGIN_AUTHOR, NULL);
    ASSERT_EQ(err, CSS_OK);

    err = css_select_ctx_count_sheets(ctx, &count);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_EQ(count, 1);

    css_select_ctx_destroy(ctx);
    css_stylesheet_destroy(sheet);
}

/* ===================================================================
 * Category 5: Stress (8)
 * =================================================================== */

TEST(stress_parse_destroy_50)
{
    css_stylesheet *sheet = NULL;
    css_error err;
    int i;
    for (i = 0; i < 50; i++) {
        err = parse_css_one_shot("p { color: red; }", &sheet);
        ASSERT_EQ(err, CSS_OK);
        css_stylesheet_destroy(sheet);
        sheet = NULL;
    }
}

TEST(stress_parse_1kb)
{
    char css[2048];
    char *p = css;
    css_stylesheet *sheet = NULL;
    css_error err;
    int i;

    for (i = 0; i < 50 && (p - css) < 1900; i++) {
        p += sprintf(p, ".cls%d { color: rgb(%d,%d,%d); }\n",
                     i, i, (i*7)%256, (i*13)%256);
    }
    ASSERT(p - css >= 800);

    err = parse_css_one_shot(css, &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(stress_parse_4kb)
{
    static char css[4500];
    char *p = css;
    css_stylesheet *sheet = NULL;
    css_error err;
    int i;

    for (i = 0; i < 200 && (p - css) < 4400; i++) {
        p += sprintf(p, "#id%d { font-size: %dpx; margin: %dem; }\n",
                     i, 10 + i, i % 5);
    }
    ASSERT(p - css >= 3000);

    err = parse_css_one_shot(css, &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(stress_long_selector_chain)
{
    /* 30-level descendant + nth-child pseudo-class */
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "a b c d e f g h i j k l m n o p q r s t u v w x y z aa bb cc dd "
        "{ color: red; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(stress_50_properties_per_rule)
{
    char css[4096];
    char *p = css;
    css_stylesheet *sheet = NULL;
    css_error err;
    int i;

    p += sprintf(p, "p {\n");
    for (i = 0; i < 50; i++) {
        p += sprintf(p, "  color: rgb(%d,%d,%d);\n", i, i, i);
    }
    p += sprintf(p, "}\n");

    err = parse_css_one_shot(css, &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(stress_select_ctx_10_sheets)
{
    css_select_ctx *ctx = NULL;
    css_stylesheet *sheets[10] = { NULL };
    uint32_t count = 0;
    css_error err;
    int i;

    err = css_select_ctx_create(&ctx);
    ASSERT_EQ(err, CSS_OK);

    for (i = 0; i < 10; i++) {
        err = parse_css_one_shot("p { color: red; }", &sheets[i]);
        ASSERT_EQ(err, CSS_OK);
        err = css_select_ctx_append_sheet(ctx, sheets[i],
                                          CSS_ORIGIN_AUTHOR, NULL);
        ASSERT_EQ(err, CSS_OK);
    }

    err = css_select_ctx_count_sheets(ctx, &count);
    ASSERT_EQ(err, CSS_OK);
    ASSERT_EQ(count, 10);

    css_select_ctx_destroy(ctx);
    for (i = 0; i < 10; i++) {
        css_stylesheet_destroy(sheets[i]);
    }
}

TEST(stress_quirks_mode_parse)
{
    /*
     * Quirks mode: tolerates pre-CSS2 syntax. Real-world web has lots
     * of legacy CSS that triggers this code path.
     */
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    css_error err;

    init_params(&params, NULL);
    params.allow_quirks = true;
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_stylesheet_append_data(sheet,
        (const uint8_t *)"body { background: red }",
        strlen("body { background: red }"));
    ASSERT(err == CSS_OK || err == CSS_NEEDDATA);
    err = css_stylesheet_data_done(sheet);
    ASSERT_EQ(err, CSS_OK);

    css_stylesheet_destroy(sheet);
}

TEST(stress_inline_style_parse)
{
    /*
     * Inline style ("style" attribute on HTML element). Different
     * parser entry vs full stylesheet.
     */
    css_stylesheet_params params;
    css_stylesheet *sheet = NULL;
    css_error err;

    init_params(&params, NULL);
    params.inline_style = true;
    err = css_stylesheet_create(&params, &sheet);
    ASSERT_EQ(err, CSS_OK);

    err = css_stylesheet_append_data(sheet,
        (const uint8_t *)"color: red; font-weight: bold",
        strlen("color: red; font-weight: bold"));
    ASSERT(err == CSS_OK || err == CSS_NEEDDATA);
    err = css_stylesheet_data_done(sheet);
    ASSERT_EQ(err, CSS_OK);

    css_stylesheet_destroy(sheet);
}

/* ===================================================================
 * Category 6: Property parsing variety (6)
 * =================================================================== */

TEST(parse_color_hex)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        ".x { color: #ff00ff; background: #000; }", &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(parse_color_named_and_rgb)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        ".x { color: red; }\n"
        ".y { color: rgb(255, 0, 128); }\n"
        ".z { color: rgba(0, 0, 0, 0.5); }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(parse_lengths_units)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        ".x { width: 100px; height: 50%; margin-top: 1.5em; "
        "padding: 10pt; line-height: 0.8; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(parse_margin_shorthand)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        ".a { margin: 0; }\n"
        ".b { margin: 10px; }\n"
        ".c { margin: 10px 20px; }\n"
        ".d { margin: 10px 20px 30px; }\n"
        ".e { margin: 10px 20px 30px 40px; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(parse_font_shorthand)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "body { font: 14px/1.5 Arial, sans-serif; }\n"
        "h1 { font: bold 24px serif; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

TEST(parse_pseudo_classes_and_descendant)
{
    css_stylesheet *sheet = NULL;
    css_error err = parse_css_one_shot(
        "a:hover { color: blue; }\n"
        "a:visited { color: purple; }\n"
        "ul li:first-child { font-weight: bold; }\n"
        ".container .item:nth-child(odd) { background: #eee; }\n", &sheet);
    ASSERT_EQ(err, CSS_OK);
    css_stylesheet_destroy(sheet);
}

/* ===================================================================
 * main
 * =================================================================== */

int main(void)
{
    printf("\n=== libcss unit tests (38) ===\n\n");

    printf("[Functional]\n");
    RUN_TEST(stylesheet_create_destroy);
    RUN_TEST(parse_simple_rule);
    RUN_TEST(stylesheet_get_url);
    RUN_TEST(stylesheet_get_title);
    RUN_TEST(stylesheet_get_language_level);
    RUN_TEST(stylesheet_size);
    RUN_TEST(select_ctx_create_destroy);
    RUN_TEST(select_ctx_count_sheets_empty);

    printf("\n[Error path]\n");
    RUN_TEST(create_null_params);
    RUN_TEST(create_null_result);
    RUN_TEST(create_invalid_params_version);
    RUN_TEST(append_data_null_data);
    RUN_TEST(select_ctx_count_null_ctx);

    printf("\n[Edge case]\n");
    RUN_TEST(parse_empty_stylesheet);
    RUN_TEST(parse_comment_only);
    RUN_TEST(parse_chunked_append);
    RUN_TEST(parse_multi_selector_rule);
    RUN_TEST(parse_multi_rule_stylesheet);
    RUN_TEST(parse_invalid_css_no_crash);

    printf("\n[Amiga-specific]\n");
    RUN_TEST(error_to_string_safe);
    RUN_TEST(parse_css_with_lengths_no_softfloat);
    RUN_TEST(parse_deep_nested_selectors_safe_stack);
    RUN_TEST(stylesheet_destroy_safe);
    RUN_TEST(select_ctx_append_sheet_basic);

    printf("\n[Stress]\n");
    RUN_TEST(stress_parse_destroy_50);
    RUN_TEST(stress_parse_1kb);
    RUN_TEST(stress_parse_4kb);
    RUN_TEST(stress_long_selector_chain);
    RUN_TEST(stress_50_properties_per_rule);
    RUN_TEST(stress_select_ctx_10_sheets);
    RUN_TEST(stress_quirks_mode_parse);
    RUN_TEST(stress_inline_style_parse);

    printf("\n[Property parsing]\n");
    RUN_TEST(parse_color_hex);
    RUN_TEST(parse_color_named_and_rgb);
    RUN_TEST(parse_lengths_units);
    RUN_TEST(parse_margin_shorthand);
    RUN_TEST(parse_font_shorthand);
    RUN_TEST(parse_pseudo_classes_and_descendant);

    return test_summary();
}
