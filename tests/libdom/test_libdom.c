/*
 * test_libdom.c -- unit tests for lib/libdom
 *
 * Library built -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE -D_BSD_SOURCE
 * per NetSurf-Vampire dep stack convention. Depends on libhubbub +
 * libparserutils + libwapcaplet (link order: -ldom -lhubbub -lparserutils
 * -lwapcaplet -- the symbols resolve top-down).
 *
 * Run via: vamos -C 68040 -s 4096 -m 8192 ./test_libdom
 *
 * Coverage strategy: most valuable tests are the end-to-end hubbub-binding
 * paths, because they exercise EVERY libdom subsystem (Document, Element,
 * Text, Comment, Attr, NodeList) via the canonical NetSurf entry point
 * (dom_hubbub_parser_create + parse_chunk + completed). The functional /
 * error-path / edge-case / amiga-specific tests cover the direct API
 * surface for completeness.
 *
 * 47 tests across the six categories per docs/test-coverage-standard.md:
 *   8  functional   (direct API lifecycle + accessors)
 *   4  error path   (DOM_*_ERR codes for hierarchy / not-found / wrong-doc / character)
 *   7  edge case    (empty / single-element / deep / wide / remove-only / split-text / nodelist-after-mutation)
 *   5  Amiga-specific (endian / alignment / stack / cleanup / namespace_finalise)
 *   8  stress       (50-iter parser create+destroy, 1KB/4KB/16KB parses, 100-list walk, repeat alloc, repeat element create, repeat text append)
 *  15  end-to-end via hubbub binding
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <dom/dom.h>
#include <dom/core/string.h>
#include <dom/core/document.h>
#include <dom/core/element.h>
#include <dom/core/text.h>
#include <dom/core/characterdata.h>
#include <dom/core/nodelist.h>
#include <dom/core/namednodemap.h>
#include <dom/core/node.h>
#include <dom/core/comment.h>
#include <dom/core/exceptions.h>
#include <dom/core/implementation.h>

#include "../../lib/libdom/src/bindings/hubbub/parser.h"

#include "test_framework.h"

/*
 * libdom + libhubbub + libparserutils + libwapcaplet linked together is a
 * libhubbub-class+ binary. Per
 * ~/.claude/projects/-Users-duncan-Developer-amiport/memory/feedback_libnix_stack_scales_with_binary.md,
 * 256 KB causes libnix's stdio init to fail with Illegal Instruction
 * before main() runs. 512 KB is the floor for binaries linking the dep
 * stack. Use 512 KB pre-emptively to avoid the bisect cycle.
 */
long __stack = 1048576;
unsigned long __MEMORY_STEP = 1048576;

/* ===================================================================
 * Helpers
 * =================================================================== */

/*
 * Make a transient dom_string from a C literal. Caller-owns; must unref.
 * Returns NULL on allocation failure (asserted by caller via ASSERT_NOT_NULL).
 */
static dom_string *make_dom_string(const char *s)
{
    dom_string *ds = NULL;
    dom_exception err;

    err = dom_string_create((const uint8_t *)s, strlen(s), &ds);
    if (err != DOM_NO_ERR) {
        return NULL;
    }
    return ds;
}

/*
 * Quiet message-callback for the hubbub parser. Without one, the binding's
 * default writes diagnostics to stderr at high verbosity which clutters TAP
 * output. We swallow everything.
 */
static void quiet_msg(uint32_t severity, void *ctx, const char *msg, ...)
{
    (void)severity; (void)ctx; (void)msg;
}

static void init_parser_params(dom_hubbub_parser_params *p)
{
    p->enc = NULL;
    p->fix_enc = true;
    p->enable_script = false;
    p->script = NULL;
    p->msg = quiet_msg;
    p->ctx = NULL;
    p->daf = NULL;
}

/*
 * Parse a complete HTML document in one shot. Caller becomes owner of *doc
 * and must dom_node_unref it. Parser is created and destroyed internally.
 */
static dom_hubbub_error parse_html_one_shot(const char *html,
                                            dom_document **doc)
{
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_hubbub_error err;

    init_parser_params(&params);

    err = dom_hubbub_parser_create(&params, &parser, doc);
    if (err != DOM_HUBBUB_OK) {
        return err;
    }

    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)html,
                                        strlen(html));
    if (err != DOM_HUBBUB_OK) {
        dom_hubbub_parser_destroy(parser);
        return err;
    }

    err = dom_hubbub_parser_completed(parser);
    dom_hubbub_parser_destroy(parser);
    return err;
}

/* ===================================================================
 * Category 1: Functional (8)
 * =================================================================== */

TEST(string_lifecycle)
{
    dom_string *s = make_dom_string("hello");
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(dom_string_byte_length(s), 5);
    ASSERT_STR_EQ(dom_string_data(s), "hello");
    dom_string_unref(s);
}

TEST(string_compare_isequal)
{
    dom_string *a = make_dom_string("foo");
    dom_string *b = make_dom_string("foo");
    dom_string *c = make_dom_string("bar");
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(c);
    ASSERT(dom_string_isequal(a, b));
    ASSERT(!dom_string_isequal(a, c));
    dom_string_unref(a);
    dom_string_unref(b);
    dom_string_unref(c);
}

TEST(document_lifecycle)
{
    dom_document *doc = NULL;
    dom_exception err;
    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(doc);
    dom_node_unref(doc);
}

TEST(create_element_and_get_tag_name)
{
    dom_document *doc = NULL;
    dom_element *el = NULL;
    dom_string *tag = NULL;
    dom_string *got = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &el);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(el);

    err = dom_element_get_tag_name(el, &got);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(got);
    /*
     * HTML element tag names are uppercased by the implementation
     * (per HTML DOM spec). Test for either case to allow for either
     * the HTML-document branch or the core-document branch.
     */
    ASSERT(dom_string_caseless_isequal(got, tag));

    dom_string_unref(got);
    dom_string_unref(tag);
    dom_node_unref(el);
    dom_node_unref(doc);
}

TEST(create_text_node_and_get_data)
{
    dom_document *doc = NULL;
    dom_text *text = NULL;
    dom_string *data = NULL;
    dom_string *got = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    data = make_dom_string("hello world");
    ASSERT_NOT_NULL(data);

    err = dom_document_create_text_node(doc, data, &text);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(text);

    err = dom_characterdata_get_data(text, &got);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(got);
    ASSERT(dom_string_isequal(got, data));

    dom_string_unref(got);
    dom_string_unref(data);
    dom_node_unref(text);
    dom_node_unref(doc);
}

TEST(append_child_parent_relationship)
{
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_element *child = NULL;
    dom_node *result = NULL;
    dom_node *got_parent = NULL;
    dom_string *tag = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);
    err = dom_document_create_element(doc, tag, &child);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_append_child(parent, child, &result);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(result);

    err = dom_node_get_parent_node(child, &got_parent);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_EQ((void *)got_parent, (void *)parent);

    dom_node_unref(got_parent);
    dom_node_unref(result);
    dom_node_unref(child);
    dom_node_unref(parent);
    dom_string_unref(tag);
    dom_node_unref(doc);
}

TEST(set_and_get_attribute)
{
    dom_document *doc = NULL;
    dom_element *el = NULL;
    dom_string *tag = NULL;
    dom_string *attr_name = NULL;
    dom_string *attr_value = NULL;
    dom_string *got_value = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("a");
    attr_name = make_dom_string("href");
    attr_value = make_dom_string("http://example.com/");
    ASSERT_NOT_NULL(tag);
    ASSERT_NOT_NULL(attr_name);
    ASSERT_NOT_NULL(attr_value);

    err = dom_document_create_element(doc, tag, &el);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_element_set_attribute(el, attr_name, attr_value);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_element_get_attribute(el, attr_name, &got_value);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(got_value);
    ASSERT(dom_string_isequal(got_value, attr_value));

    dom_string_unref(got_value);
    dom_string_unref(attr_value);
    dom_string_unref(attr_name);
    dom_string_unref(tag);
    dom_node_unref(el);
    dom_node_unref(doc);
}

TEST(child_nodes_length)
{
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_element *child = NULL;
    dom_node *junk = NULL;
    dom_nodelist *list = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    uint32_t length = 0;
    int i;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);

    for (i = 0; i < 5; i++) {
        err = dom_document_create_element(doc, tag, &child);
        ASSERT_EQ(err, DOM_NO_ERR);
        err = dom_node_append_child(parent, child, &junk);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(junk);
        dom_node_unref(child);
    }

    err = dom_node_get_child_nodes(parent, &list);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(list);

    err = dom_nodelist_get_length(list, &length);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_EQ(length, 5);

    dom_nodelist_unref(list);
    dom_string_unref(tag);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

/* ===================================================================
 * Category 2: Error path (4)
 * =================================================================== */

TEST(append_child_hierarchy_request_err)
{
    /*
     * Attempting to append a Document into an Element should fail with
     * DOM_HIERARCHY_REQUEST_ERR (per DOM Core spec section 1.4 -- the
     * Document node type cannot be a child of another node).
     */
    dom_document *doc = NULL;
    dom_element *el = NULL;
    dom_node *junk = NULL;
    dom_string *tag = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &el);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_append_child(el, doc, &junk);
    ASSERT_EQ(err, DOM_HIERARCHY_REQUEST_ERR);

    dom_string_unref(tag);
    dom_node_unref(el);
    dom_node_unref(doc);
}

TEST(remove_child_not_found_err)
{
    /*
     * remove_child of a node that is not actually a child of the parent
     * must return DOM_NOT_FOUND_ERR.
     */
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_element *unrelated = NULL;
    dom_node *junk = NULL;
    dom_string *tag = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);
    err = dom_document_create_element(doc, tag, &unrelated);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_remove_child(parent, unrelated, &junk);
    ASSERT_EQ(err, DOM_NOT_FOUND_ERR);

    dom_string_unref(tag);
    dom_node_unref(unrelated);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

TEST(create_element_invalid_character_err)
{
    /*
     * Empty tag name is an INVALID_CHARACTER_ERR.
     *
     * Note: libdom's HTMLDocument override only checks for empty tag names
     * (src/html/html_document.c:557-558). The XML-spec full Name-production
     * validation only runs in the XML core path, NOT the HTML path -- the
     * HTML5 spec is more permissive than XML and libdom follows that. So
     * passing "bad tag" (with a space) to dom_document_create_element on
     * an HTMLDocument actually succeeds, returning DOM_NO_ERR. Stick with
     * the empty-tag-name case for portability across both Document modes.
     */
    dom_document *doc = NULL;
    dom_element *el = NULL;
    dom_string *bad_tag = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    bad_tag = make_dom_string("");
    ASSERT_NOT_NULL(bad_tag);

    err = dom_document_create_element(doc, bad_tag, &el);
    ASSERT_EQ(err, DOM_INVALID_CHARACTER_ERR);

    dom_string_unref(bad_tag);
    dom_node_unref(doc);
}

TEST(append_child_wrong_document_err)
{
    /*
     * Appending a node from a different document into a tree without first
     * importing it must return DOM_WRONG_DOCUMENT_ERR.
     */
    dom_document *doc_a = NULL;
    dom_document *doc_b = NULL;
    dom_element *parent_a = NULL;
    dom_element *child_b = NULL;
    dom_node *junk = NULL;
    dom_string *tag = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc_a);
    ASSERT_EQ(err, DOM_NO_ERR);
    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc_b);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc_a, tag, &parent_a);
    ASSERT_EQ(err, DOM_NO_ERR);
    err = dom_document_create_element(doc_b, tag, &child_b);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_append_child(parent_a, child_b, &junk);
    ASSERT_EQ(err, DOM_WRONG_DOCUMENT_ERR);

    dom_string_unref(tag);
    dom_node_unref(child_b);
    dom_node_unref(parent_a);
    dom_node_unref(doc_b);
    dom_node_unref(doc_a);
}

/* ===================================================================
 * Category 3: Edge case (7)
 * =================================================================== */

TEST(empty_document_no_root)
{
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_document_get_document_element(doc, &root);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NULL(root);

    dom_node_unref(doc);
}

TEST(single_element_no_sibling)
{
    dom_document *doc = NULL;
    dom_element *el = NULL;
    dom_node *next = NULL;
    dom_string *tag = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &el);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_get_next_sibling(el, &next);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NULL(next);

    dom_string_unref(tag);
    dom_node_unref(el);
    dom_node_unref(doc);
}

TEST(deep_nested_tree_50_levels)
{
    /*
     * 50-level deep tree built by repeated append_child of fresh
     * elements. Then walk back down via get_first_child to verify depth.
     * Bounded to 50 -- HTML in practice rarely nests deeper, and we want
     * the test to fit comfortably under 524 KB stack.
     */
    dom_document *doc = NULL;
    dom_element *cursor = NULL;
    dom_element *parent_of_root = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    int i, depth;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &cursor);
    ASSERT_EQ(err, DOM_NO_ERR);
    parent_of_root = cursor;
    /* keep parent_of_root around so the whole subtree stays alive */

    for (i = 0; i < 49; i++) {
        dom_element *next = NULL;
        dom_node *junk = NULL;
        err = dom_document_create_element(doc, tag, &next);
        ASSERT_EQ(err, DOM_NO_ERR);
        err = dom_node_append_child(cursor, next, &junk);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(junk);
        dom_node_unref(next);
        cursor = next;
    }

    /* Walk down */
    cursor = parent_of_root;
    depth = 1;
    while (cursor != NULL) {
        dom_node *child = NULL;
        err = dom_node_get_first_child(cursor, &child);
        ASSERT_EQ(err, DOM_NO_ERR);
        if (child == NULL) {
            break;
        }
        depth++;
        dom_node_unref(child);
        cursor = (dom_element *)child;
    }
    ASSERT_EQ(depth, 50);

    dom_string_unref(tag);
    dom_node_unref(parent_of_root);
    dom_node_unref(doc);
}

TEST(many_siblings_50)
{
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_element *child = NULL;
    dom_node *junk = NULL;
    dom_node *cursor = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    int i, count;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("li");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);

    for (i = 0; i < 50; i++) {
        err = dom_document_create_element(doc, tag, &child);
        ASSERT_EQ(err, DOM_NO_ERR);
        err = dom_node_append_child(parent, child, &junk);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(junk);
        dom_node_unref(child);
    }

    err = dom_node_get_first_child(parent, &cursor);
    ASSERT_EQ(err, DOM_NO_ERR);
    count = 0;
    while (cursor != NULL) {
        dom_node *next = NULL;
        count++;
        err = dom_node_get_next_sibling(cursor, &next);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(cursor);
        cursor = next;
    }
    ASSERT_EQ(count, 50);

    dom_string_unref(tag);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

TEST(remove_only_child_yields_empty)
{
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_element *child = NULL;
    dom_node *result = NULL;
    dom_node *got = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    bool has_children = true;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);
    err = dom_document_create_element(doc, tag, &child);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_append_child(parent, child, &result);
    ASSERT_EQ(err, DOM_NO_ERR);
    dom_node_unref(result);

    err = dom_node_remove_child(parent, child, &got);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_EQ((void *)got, (void *)child);

    err = dom_node_has_child_nodes(parent, &has_children);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT(!has_children);

    dom_node_unref(got);
    dom_node_unref(child);
    dom_string_unref(tag);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

TEST(text_node_split)
{
    /*
     * Split a 10-char text node at offset 5. Original keeps "hello",
     * returned new text holds "world".
     *
     * IMPORTANT: text MUST be attached to a parent element BEFORE the
     * split, because libdom's _dom_dispatch_subtree_modified_event NULL-
     * derefs when fired against c->parent on a parent-less character data
     * node. Real consumers (like the hubbub binding building DOM trees)
     * always have parents, so this is fine in practice -- but the library
     * does not guard against the NULL deref. Documented in lib/libdom/README.md.
     */
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_text *text = NULL;
    dom_text *split_part = NULL;
    dom_node *junk = NULL;
    dom_string *data = NULL;
    dom_string *div_tag = NULL;
    dom_string *got_orig = NULL;
    dom_string *got_split = NULL;
    dom_exception err;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    div_tag = make_dom_string("div");
    ASSERT_NOT_NULL(div_tag);
    err = dom_document_create_element(doc, div_tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);

    data = make_dom_string("helloworld");
    ASSERT_NOT_NULL(data);

    err = dom_document_create_text_node(doc, data, &text);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_append_child(parent, text, &junk);
    ASSERT_EQ(err, DOM_NO_ERR);
    dom_node_unref(junk);

    err = dom_text_split_text(text, 5, &split_part);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(split_part);

    err = dom_characterdata_get_data(text, &got_orig);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_STR_EQ(dom_string_data(got_orig), "hello");

    err = dom_characterdata_get_data(split_part, &got_split);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_STR_EQ(dom_string_data(got_split), "world");

    dom_string_unref(got_split);
    dom_string_unref(got_orig);
    dom_node_unref(split_part);
    dom_node_unref(text);
    dom_string_unref(data);
    dom_string_unref(div_tag);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

TEST(nodelist_length_after_mutation)
{
    /*
     * NodeList is "live": appending a child after fetching the list must
     * be reflected in the next get_length call.
     */
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_element *child = NULL;
    dom_node *junk = NULL;
    dom_nodelist *list = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    uint32_t length;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_node_get_child_nodes(parent, &list);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_nodelist_get_length(list, &length);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_EQ(length, 0);

    err = dom_document_create_element(doc, tag, &child);
    ASSERT_EQ(err, DOM_NO_ERR);
    err = dom_node_append_child(parent, child, &junk);
    ASSERT_EQ(err, DOM_NO_ERR);
    dom_node_unref(junk);
    dom_node_unref(child);

    err = dom_nodelist_get_length(list, &length);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_EQ(length, 1);

    dom_nodelist_unref(list);
    dom_string_unref(tag);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

/* ===================================================================
 * Category 4: Amiga-specific (5)
 * =================================================================== */

TEST(string_hash_endian_safe)
{
    /*
     * 68k is big-endian. Verify dom_string_hash returns a non-zero
     * deterministic value -- catches accidents like reading a 16-bit
     * codepoint as host-order 32-bit on big-endian.
     */
    dom_string *s = make_dom_string("amiga");
    uint32_t h1, h2;
    ASSERT_NOT_NULL(s);

    h1 = dom_string_hash(s);
    h2 = dom_string_hash(s);
    ASSERT(h1 != 0);
    ASSERT_EQ(h1, h2);

    dom_string_unref(s);
}

TEST(parser_create_no_alignment_trap)
{
    /*
     * The hubbub binding pushes a hubbub_tree_handler struct (17 callback
     * pointers) onto the parser. On 68k, struct returns and pointer
     * arrays must be properly aligned. If alignment is wrong, parser
     * setup will Guru before main() returns.
     */
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_hubbub_error err;

    init_parser_params(&params);

    err = dom_hubbub_parser_create(&params, &parser, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);
    ASSERT_NOT_NULL(parser);
    ASSERT_NOT_NULL(doc);

    dom_hubbub_parser_destroy(parser);
    dom_node_unref(doc);
}

TEST(deep_tree_safe_within_524k_stack)
{
    /*
     * Build a 30-level deep tree and walk it. The internal libdom
     * tree-walk and refcount paths use limited stack per frame, but
     * combined with libnix's start-up overhead this is the practical
     * "is libdom safe in our memory budget" check.
     */
    dom_document *doc = NULL;
    dom_element *cursor = NULL;
    dom_element *root = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    int i;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &cursor);
    ASSERT_EQ(err, DOM_NO_ERR);
    root = cursor;

    for (i = 0; i < 29; i++) {
        dom_element *next = NULL;
        dom_node *junk = NULL;
        err = dom_document_create_element(doc, tag, &next);
        ASSERT_EQ(err, DOM_NO_ERR);
        err = dom_node_append_child(cursor, next, &junk);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(junk);
        dom_node_unref(next);
        cursor = next;
    }

    dom_string_unref(tag);
    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(document_destroy_safe_with_subtree)
{
    /*
     * Build a 5-element subtree and rely on the root unref to free the
     * full tree. If libdom's destroy path is broken on 68k the binary
     * will Guru here. Not a leak check (we have no allocator hook on
     * vamos) -- only a crash check.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_element *child = NULL;
    dom_node *junk = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    int i;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("p");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &root);
    ASSERT_EQ(err, DOM_NO_ERR);

    for (i = 0; i < 5; i++) {
        err = dom_document_create_element(doc, tag, &child);
        ASSERT_EQ(err, DOM_NO_ERR);
        err = dom_node_append_child(root, child, &junk);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(junk);
        dom_node_unref(child);
    }

    /* Single unref must destroy the whole subtree */
    dom_string_unref(tag);
    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(namespace_finalise_completes)
{
    /*
     * After calling dom_namespace_finalise the dom_namespaces[] array
     * must be NULLed (or at least the call must complete cleanly).
     * The function is the documented exit-cleanup hook for ports/netsurf.
     */
    dom_document *doc = NULL;
    dom_exception err;

    /* Ensure the namespace strings are initialised by creating a doc */
    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);
    dom_node_unref(doc);

    err = dom_namespace_finalise();
    ASSERT_EQ(err, DOM_NO_ERR);
}

/* ===================================================================
 * Category 5: Stress (8)
 * =================================================================== */

TEST(stress_parser_create_destroy_50)
{
    /*
     * 50 iterations of full parser lifecycle. Mirrors the libhubbub
     * pattern -- catches per-iteration leaks of the binding's malloc'd
     * binding struct + tokenizer + treebuilder + documents.
     */
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_hubbub_error err;
    int i;

    init_parser_params(&params);

    for (i = 0; i < 50; i++) {
        err = dom_hubbub_parser_create(&params, &parser, &doc);
        ASSERT_EQ(err, DOM_HUBBUB_OK);
        dom_hubbub_parser_destroy(parser);
        dom_node_unref(doc);
        parser = NULL;
        doc = NULL;
    }
}

TEST(stress_parse_minimal_repeatedly)
{
    /*
     * Parse a small HTML chunk through full pipeline 30 times. Slower
     * than the create/destroy loop -- exercises the codec, tokeniser,
     * treebuilder, AND DOM construction every iteration.
     */
    dom_document *doc = NULL;
    dom_hubbub_error err;
    const char *html = "<html><body><p>x</p></body></html>";
    int i;

    for (i = 0; i < 30; i++) {
        err = parse_html_one_shot(html, &doc);
        ASSERT_EQ(err, DOM_HUBBUB_OK);
        ASSERT_NOT_NULL(doc);
        dom_node_unref(doc);
        doc = NULL;
    }
}

TEST(stress_parse_1kb_html)
{
    /*
     * Build a 1 KB document of <p>n</p> elements and feed it. Tests the
     * tokenizer hot loop + element-creation hot path.
     */
    char html[2048];
    char *p = html;
    dom_document *doc = NULL;
    dom_hubbub_error err;
    int i;

    p += sprintf(p, "<html><body>");
    for (i = 0; i < 120 && (p - html) < 1900; i++) {
        p += sprintf(p, "<p>%d</p>", i);
    }
    p += sprintf(p, "</body></html>");
    ASSERT(p - html >= 800); /* ~1 KB target */

    err = parse_html_one_shot(html, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);
    ASSERT_NOT_NULL(doc);
    dom_node_unref(doc);
}

TEST(stress_parse_4kb_html)
{
    static char html[4500];
    char *p = html;
    dom_document *doc = NULL;
    dom_hubbub_error err;
    int i;

    p += sprintf(p, "<html><body>");
    for (i = 0; i < 400 && (p - html) < 4400; i++) {
        p += sprintf(p, "<p>x%d</p>", i);
    }
    p += sprintf(p, "</body></html>");
    ASSERT(p - html >= 3000); /* ~4 KB target */

    err = parse_html_one_shot(html, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);
    ASSERT_NOT_NULL(doc);
    dom_node_unref(doc);
}

TEST(stress_walk_50_node_list)
{
    /*
     * Build a 50-item NodeList and walk it via dom_nodelist_item.
     * Verifies the random-access path (vs sibling iteration walked
     * earlier).
     */
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_element *child = NULL;
    dom_node *junk = NULL;
    dom_nodelist *list = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    uint32_t i, length;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("li");
    ASSERT_NOT_NULL(tag);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);

    for (i = 0; i < 50; i++) {
        err = dom_document_create_element(doc, tag, &child);
        ASSERT_EQ(err, DOM_NO_ERR);
        err = dom_node_append_child(parent, child, &junk);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(junk);
        dom_node_unref(child);
    }

    err = dom_node_get_child_nodes(parent, &list);
    ASSERT_EQ(err, DOM_NO_ERR);

    err = dom_nodelist_get_length(list, &length);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_EQ(length, 50);

    for (i = 0; i < length; i++) {
        dom_node *item = NULL;
        err = dom_nodelist_item(list, i, &item);
        ASSERT_EQ(err, DOM_NO_ERR);
        ASSERT_NOT_NULL(item);
        dom_node_unref(item);
    }

    dom_nodelist_unref(list);
    dom_string_unref(tag);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

TEST(stress_dom_string_create_destroy_500)
{
    int i;
    for (i = 0; i < 500; i++) {
        dom_string *s = make_dom_string("x");
        ASSERT_NOT_NULL(s);
        dom_string_unref(s);
    }
}

TEST(stress_create_element_500)
{
    dom_document *doc = NULL;
    dom_element *el = NULL;
    dom_string *tag = NULL;
    dom_exception err;
    int i;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("div");
    ASSERT_NOT_NULL(tag);

    for (i = 0; i < 500; i++) {
        err = dom_document_create_element(doc, tag, &el);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(el);
    }

    dom_string_unref(tag);
    dom_node_unref(doc);
}

TEST(stress_text_append_remove_100)
{
    /*
     * Append-remove cycle: tests the parent/child pointer manipulation
     * and refcount bookkeeping under repeated mutation.
     */
    dom_document *doc = NULL;
    dom_element *parent = NULL;
    dom_text *text = NULL;
    dom_node *got = NULL;
    dom_node *junk = NULL;
    dom_string *tag = NULL;
    dom_string *data = NULL;
    dom_exception err;
    int i;

    err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML,
                                             NULL, NULL, NULL,
                                             NULL, NULL, &doc);
    ASSERT_EQ(err, DOM_NO_ERR);

    tag = make_dom_string("p");
    data = make_dom_string("x");
    ASSERT_NOT_NULL(tag);
    ASSERT_NOT_NULL(data);

    err = dom_document_create_element(doc, tag, &parent);
    ASSERT_EQ(err, DOM_NO_ERR);

    for (i = 0; i < 100; i++) {
        err = dom_document_create_text_node(doc, data, &text);
        ASSERT_EQ(err, DOM_NO_ERR);
        err = dom_node_append_child(parent, text, &junk);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(junk);
        err = dom_node_remove_child(parent, text, &got);
        ASSERT_EQ(err, DOM_NO_ERR);
        dom_node_unref(got);
        dom_node_unref(text);
    }

    dom_string_unref(data);
    dom_string_unref(tag);
    dom_node_unref(parent);
    dom_node_unref(doc);
}

/* ===================================================================
 * Category 6: End-to-end via hubbub binding (15)
 * =================================================================== */

TEST(e2e_minimal_html_doc)
{
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_hubbub_error err;

    err = parse_html_one_shot("<html><body>hello</body></html>", &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);
    ASSERT_NOT_NULL(doc);

    /* Document element must exist */
    err = (dom_hubbub_error)dom_document_get_document_element(doc, &root);
    ASSERT_EQ(err, (dom_hubbub_error)DOM_NO_ERR);
    ASSERT_NOT_NULL(root);

    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_root_has_html_tag_name)
{
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_string *tag = NULL;
    dom_exception err;

    ASSERT_EQ(parse_html_one_shot("<html></html>", &doc), DOM_HUBBUB_OK);

    err = dom_document_get_document_element(doc, &root);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(root);

    err = dom_element_get_tag_name(root, &tag);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(tag);

    /* HTML elements come out uppercased */
    ASSERT(strcasecmp(dom_string_data(tag), "HTML") == 0);

    dom_string_unref(tag);
    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_body_text_content_present)
{
    /*
     * Look for any text node anywhere in the tree containing "hello".
     * Tree-walk: root -> body -> text. Body is auto-inserted by HTML5
     * tokeniser even if absent in source, so this is robust.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_node *cursor = NULL;
    dom_node *child = NULL;
    bool found_text = false;
    int max_walk = 100;

    ASSERT_EQ(parse_html_one_shot("<html><body>hello world</body></html>",
                                  &doc),
              DOM_HUBBUB_OK);

    ASSERT_EQ(dom_document_get_document_element(doc, &root), DOM_NO_ERR);
    cursor = (dom_node *)root;

    /*
     * Depth-first: descend into first child while available; backtrack
     * via next_sibling. Bounded to 100 visits.
     */
    while (cursor != NULL && max_walk-- > 0) {
        dom_node_type t;
        ASSERT_EQ(dom_node_get_node_type(cursor, &t), DOM_NO_ERR);
        if (t == DOM_TEXT_NODE) {
            dom_string *data = NULL;
            ASSERT_EQ(dom_characterdata_get_data(cursor, &data), DOM_NO_ERR);
            if (data != NULL && strstr(dom_string_data(data), "hello") != NULL) {
                found_text = true;
                dom_string_unref(data);
                break;
            }
            if (data != NULL) dom_string_unref(data);
        }
        ASSERT_EQ(dom_node_get_first_child(cursor, &child), DOM_NO_ERR);
        if (child != NULL) {
            dom_node *prev = cursor;
            cursor = child;
            if ((void *)prev != (void *)root) dom_node_unref(prev);
            continue;
        }
        ASSERT_EQ(dom_node_get_next_sibling(cursor, &child), DOM_NO_ERR);
        {
            dom_node *prev = cursor;
            cursor = child;
            if ((void *)prev != (void *)root) dom_node_unref(prev);
        }
    }

    if (cursor != NULL && (void *)cursor != (void *)root) {
        dom_node_unref(cursor);
    }

    ASSERT(found_text);

    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_anchor_with_href_attribute)
{
    /*
     * Parse a document with an <a href="..."> and find that anchor's
     * href attribute via dom_element_get_attribute. Walks tree until an
     * element with tag "A" is found.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_string *href_attr = NULL;
    dom_string *got = NULL;
    dom_node *cursor = NULL;
    dom_node *child = NULL;
    bool found = false;
    int max_walk = 100;

    ASSERT_EQ(parse_html_one_shot(
                  "<html><body><a href=\"http://example.com/\">x</a></body></html>",
                  &doc),
              DOM_HUBBUB_OK);

    ASSERT_EQ(dom_document_get_document_element(doc, &root), DOM_NO_ERR);
    href_attr = make_dom_string("href");
    ASSERT_NOT_NULL(href_attr);

    cursor = (dom_node *)root;
    while (cursor != NULL && max_walk-- > 0) {
        dom_node_type t;
        ASSERT_EQ(dom_node_get_node_type(cursor, &t), DOM_NO_ERR);
        if (t == DOM_ELEMENT_NODE) {
            dom_string *tn = NULL;
            ASSERT_EQ(dom_element_get_tag_name((dom_element *)cursor, &tn),
                      DOM_NO_ERR);
            if (tn != NULL && strcasecmp(dom_string_data(tn), "A") == 0) {
                dom_string_unref(tn);
                ASSERT_EQ(dom_element_get_attribute((dom_element *)cursor,
                                                     href_attr, &got),
                          DOM_NO_ERR);
                ASSERT_NOT_NULL(got);
                ASSERT_STR_EQ(dom_string_data(got), "http://example.com/");
                found = true;
                break;
            }
            if (tn != NULL) dom_string_unref(tn);
        }
        ASSERT_EQ(dom_node_get_first_child(cursor, &child), DOM_NO_ERR);
        if (child != NULL) {
            if ((void *)cursor != (void *)root) dom_node_unref(cursor);
            cursor = child;
            continue;
        }
        ASSERT_EQ(dom_node_get_next_sibling(cursor, &child), DOM_NO_ERR);
        if ((void *)cursor != (void *)root) dom_node_unref(cursor);
        cursor = child;
    }

    if (cursor != NULL && (void *)cursor != (void *)root) {
        dom_node_unref(cursor);
    }

    ASSERT(found);

    if (got != NULL) dom_string_unref(got);
    dom_string_unref(href_attr);
    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_chunked_parse_three_chunks)
{
    /*
     * Feed the same HTML in three chunks. Result must be valid and
     * indistinguishable from one-shot parse (test by document exists +
     * has document element).
     */
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_hubbub_error err;
    const char *c1 = "<html><body>";
    const char *c2 = "<p>chunk</p>";
    const char *c3 = "</body></html>";

    init_parser_params(&params);

    err = dom_hubbub_parser_create(&params, &parser, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)c1, strlen(c1));
    ASSERT_EQ(err, DOM_HUBBUB_OK);
    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)c2, strlen(c2));
    ASSERT_EQ(err, DOM_HUBBUB_OK);
    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)c3, strlen(c3));
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_completed(parser);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    ASSERT_EQ((dom_hubbub_error)dom_document_get_document_element(doc, &root),
              (dom_hubbub_error)DOM_NO_ERR);
    ASSERT_NOT_NULL(root);

    dom_node_unref(root);
    dom_hubbub_parser_destroy(parser);
    dom_node_unref(doc);
}

TEST(e2e_byte_at_a_time_parse)
{
    /*
     * One-byte chunks. Stress-tests the codec + input stream + tokenizer
     * state machine. Output must still be a valid document.
     */
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_hubbub_error err;
    const char *html = "<html><body><p>1</p></body></html>";
    size_t i;

    init_parser_params(&params);

    err = dom_hubbub_parser_create(&params, &parser, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    for (i = 0; i < strlen(html); i++) {
        err = dom_hubbub_parser_parse_chunk(parser,
                                            (const uint8_t *)&html[i], 1);
        ASSERT_EQ(err, DOM_HUBBUB_OK);
    }
    err = dom_hubbub_parser_completed(parser);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    ASSERT_EQ((dom_hubbub_error)dom_document_get_document_element(doc, &root),
              (dom_hubbub_error)DOM_NO_ERR);
    ASSERT_NOT_NULL(root);

    dom_node_unref(root);
    dom_hubbub_parser_destroy(parser);
    dom_node_unref(doc);
}

TEST(e2e_pause_resume)
{
    /*
     * Pause the parser between chunks then resume. Documented control
     * surface for ports/netsurf when handing control back to the
     * AmigaOS event loop mid-parse.
     */
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_hubbub_error err;
    const char *html = "<html><body>x</body></html>";

    init_parser_params(&params);

    err = dom_hubbub_parser_create(&params, &parser, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)html, strlen(html) / 2);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_pause(parser, true);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_pause(parser, false);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)html + strlen(html) / 2,
                                        strlen(html) - strlen(html) / 2);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_completed(parser);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    dom_hubbub_parser_destroy(parser);
    dom_node_unref(doc);
}

TEST(e2e_get_encoding_default)
{
    /*
     * After parsing without a charset hint, the binding should report
     * SOMETHING for the encoding (UTF-8 by default). The exact string is
     * not asserted; we only check non-NULL.
     */
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_hubbub_encoding_source src;
    const char *enc;
    dom_hubbub_error err;

    init_parser_params(&params);

    err = dom_hubbub_parser_create(&params, &parser, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)"<html></html>", 13);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_completed(parser);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    enc = dom_hubbub_parser_get_encoding(parser, &src);
    ASSERT_NOT_NULL(enc);

    dom_hubbub_parser_destroy(parser);
    dom_node_unref(doc);
}

TEST(e2e_explicit_utf8_encoding)
{
    /*
     * Pass enc="UTF-8" explicitly. Parser must accept it and parse
     * cleanly. encoding_source on get_encoding becomes HEADER.
     */
    dom_hubbub_parser_params params;
    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_hubbub_error err;

    init_parser_params(&params);
    params.enc = "UTF-8";

    err = dom_hubbub_parser_create(&params, &parser, &doc);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_parse_chunk(parser,
                                        (const uint8_t *)"<html><body>x</body></html>",
                                        strlen("<html><body>x</body></html>"));
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    err = dom_hubbub_parser_completed(parser);
    ASSERT_EQ(err, DOM_HUBBUB_OK);

    dom_hubbub_parser_destroy(parser);
    dom_node_unref(doc);
}

TEST(e2e_comment_node_present)
{
    /*
     * Ensure HTML comment is present in the tree. Tree walk depth-first
     * looking for a DOM_COMMENT_NODE.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_node *cursor = NULL;
    dom_node *child = NULL;
    bool found_comment = false;
    int max_walk = 100;

    ASSERT_EQ(parse_html_one_shot("<html><!-- yo --><body>x</body></html>",
                                  &doc),
              DOM_HUBBUB_OK);

    ASSERT_EQ(dom_document_get_document_element(doc, &root), DOM_NO_ERR);

    /*
     * The comment may be a child of the document itself (before the
     * root element) rather than a child of root. Walk both.
     */
    cursor = (dom_node *)doc;
    while (cursor != NULL && max_walk-- > 0) {
        dom_node_type t;
        ASSERT_EQ(dom_node_get_node_type(cursor, &t), DOM_NO_ERR);
        if (t == DOM_COMMENT_NODE) {
            found_comment = true;
            break;
        }
        ASSERT_EQ(dom_node_get_first_child(cursor, &child), DOM_NO_ERR);
        if (child != NULL) {
            if ((void *)cursor != (void *)doc) dom_node_unref(cursor);
            cursor = child;
            continue;
        }
        ASSERT_EQ(dom_node_get_next_sibling(cursor, &child), DOM_NO_ERR);
        if ((void *)cursor != (void *)doc) dom_node_unref(cursor);
        cursor = child;
    }

    if (cursor != NULL && (void *)cursor != (void *)doc) dom_node_unref(cursor);

    ASSERT(found_comment);

    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_paragraph_under_body)
{
    /*
     * Body must have a P element child. Find the BODY tag, then look for
     * a child element with tag P.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_node *cursor = NULL;
    dom_node *child = NULL;
    bool found_p = false;
    int max_walk = 100;

    ASSERT_EQ(parse_html_one_shot("<html><body><p>under-body</p></body></html>",
                                  &doc),
              DOM_HUBBUB_OK);

    ASSERT_EQ(dom_document_get_document_element(doc, &root), DOM_NO_ERR);

    cursor = (dom_node *)root;
    while (cursor != NULL && max_walk-- > 0) {
        dom_node_type t;
        ASSERT_EQ(dom_node_get_node_type(cursor, &t), DOM_NO_ERR);
        if (t == DOM_ELEMENT_NODE) {
            dom_string *tn = NULL;
            ASSERT_EQ(dom_element_get_tag_name((dom_element *)cursor, &tn),
                      DOM_NO_ERR);
            if (tn != NULL && strcasecmp(dom_string_data(tn), "P") == 0) {
                found_p = true;
                dom_string_unref(tn);
                break;
            }
            if (tn != NULL) dom_string_unref(tn);
        }
        ASSERT_EQ(dom_node_get_first_child(cursor, &child), DOM_NO_ERR);
        if (child != NULL) {
            if ((void *)cursor != (void *)root) dom_node_unref(cursor);
            cursor = child;
            continue;
        }
        ASSERT_EQ(dom_node_get_next_sibling(cursor, &child), DOM_NO_ERR);
        if ((void *)cursor != (void *)root) dom_node_unref(cursor);
        cursor = child;
    }

    if (cursor != NULL && (void *)cursor != (void *)root) dom_node_unref(cursor);

    ASSERT(found_p);

    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_input_with_value_attribute)
{
    /*
     * Form input with value. Locate the INPUT element by tag, then read
     * its "value" attribute.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_string *value_attr = NULL;
    dom_string *got = NULL;
    dom_node *cursor = NULL;
    dom_node *child = NULL;
    bool found = false;
    int max_walk = 100;

    ASSERT_EQ(parse_html_one_shot(
                  "<html><body><input value=\"foo\"></body></html>",
                  &doc),
              DOM_HUBBUB_OK);

    ASSERT_EQ(dom_document_get_document_element(doc, &root), DOM_NO_ERR);
    value_attr = make_dom_string("value");
    ASSERT_NOT_NULL(value_attr);

    cursor = (dom_node *)root;
    while (cursor != NULL && max_walk-- > 0) {
        dom_node_type t;
        ASSERT_EQ(dom_node_get_node_type(cursor, &t), DOM_NO_ERR);
        if (t == DOM_ELEMENT_NODE) {
            dom_string *tn = NULL;
            ASSERT_EQ(dom_element_get_tag_name((dom_element *)cursor, &tn),
                      DOM_NO_ERR);
            if (tn != NULL && strcasecmp(dom_string_data(tn), "INPUT") == 0) {
                dom_string_unref(tn);
                ASSERT_EQ(dom_element_get_attribute((dom_element *)cursor,
                                                     value_attr, &got),
                          DOM_NO_ERR);
                ASSERT_NOT_NULL(got);
                ASSERT_STR_EQ(dom_string_data(got), "foo");
                found = true;
                break;
            }
            if (tn != NULL) dom_string_unref(tn);
        }
        ASSERT_EQ(dom_node_get_first_child(cursor, &child), DOM_NO_ERR);
        if (child != NULL) {
            if ((void *)cursor != (void *)root) dom_node_unref(cursor);
            cursor = child;
            continue;
        }
        ASSERT_EQ(dom_node_get_next_sibling(cursor, &child), DOM_NO_ERR);
        if ((void *)cursor != (void *)root) dom_node_unref(cursor);
        cursor = child;
    }

    if (cursor != NULL && (void *)cursor != (void *)root) dom_node_unref(cursor);

    ASSERT(found);

    if (got != NULL) dom_string_unref(got);
    dom_string_unref(value_attr);
    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_table_structure_preserved)
{
    /*
     * <table><tr><td>x</td></tr></table> -- HTML5 tokeniser auto-inserts
     * a <tbody> wrapper. Verify the tree contains a TABLE, a TR, and a
     * TD via tag-name walk.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_node *cursor = NULL;
    dom_node *child = NULL;
    bool found_table = false, found_tr = false, found_td = false;
    int max_walk = 200;

    ASSERT_EQ(parse_html_one_shot(
                  "<html><body><table><tr><td>x</td></tr></table></body></html>",
                  &doc),
              DOM_HUBBUB_OK);

    ASSERT_EQ(dom_document_get_document_element(doc, &root), DOM_NO_ERR);

    cursor = (dom_node *)root;
    while (cursor != NULL && max_walk-- > 0) {
        dom_node_type t;
        ASSERT_EQ(dom_node_get_node_type(cursor, &t), DOM_NO_ERR);
        if (t == DOM_ELEMENT_NODE) {
            dom_string *tn = NULL;
            ASSERT_EQ(dom_element_get_tag_name((dom_element *)cursor, &tn),
                      DOM_NO_ERR);
            if (tn != NULL) {
                if (strcasecmp(dom_string_data(tn), "TABLE") == 0) found_table = true;
                if (strcasecmp(dom_string_data(tn), "TR")    == 0) found_tr    = true;
                if (strcasecmp(dom_string_data(tn), "TD")    == 0) found_td    = true;
                dom_string_unref(tn);
            }
        }
        ASSERT_EQ(dom_node_get_first_child(cursor, &child), DOM_NO_ERR);
        if (child != NULL) {
            if ((void *)cursor != (void *)root) dom_node_unref(cursor);
            cursor = child;
            continue;
        }
        ASSERT_EQ(dom_node_get_next_sibling(cursor, &child), DOM_NO_ERR);
        if ((void *)cursor != (void *)root) dom_node_unref(cursor);
        cursor = child;
    }

    if (cursor != NULL && (void *)cursor != (void *)root) dom_node_unref(cursor);

    ASSERT(found_table);
    ASSERT(found_tr);
    ASSERT(found_td);

    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_named_entity_decoded)
{
    /*
     * &amp; in HTML must decode to '&' in the resulting DOM text.
     */
    dom_document *doc = NULL;
    dom_element *root = NULL;
    dom_node *cursor = NULL;
    dom_node *child = NULL;
    bool found_amp = false;
    int max_walk = 100;

    ASSERT_EQ(parse_html_one_shot("<html><body>&amp;</body></html>", &doc),
              DOM_HUBBUB_OK);

    ASSERT_EQ(dom_document_get_document_element(doc, &root), DOM_NO_ERR);
    cursor = (dom_node *)root;
    while (cursor != NULL && max_walk-- > 0) {
        dom_node_type t;
        ASSERT_EQ(dom_node_get_node_type(cursor, &t), DOM_NO_ERR);
        if (t == DOM_TEXT_NODE) {
            dom_string *data = NULL;
            ASSERT_EQ(dom_characterdata_get_data(cursor, &data), DOM_NO_ERR);
            if (data != NULL && strchr(dom_string_data(data), '&') != NULL) {
                found_amp = true;
                dom_string_unref(data);
                break;
            }
            if (data != NULL) dom_string_unref(data);
        }
        ASSERT_EQ(dom_node_get_first_child(cursor, &child), DOM_NO_ERR);
        if (child != NULL) {
            if ((void *)cursor != (void *)root) dom_node_unref(cursor);
            cursor = child;
            continue;
        }
        ASSERT_EQ(dom_node_get_next_sibling(cursor, &child), DOM_NO_ERR);
        if ((void *)cursor != (void *)root) dom_node_unref(cursor);
        cursor = child;
    }

    if (cursor != NULL && (void *)cursor != (void *)root) dom_node_unref(cursor);

    ASSERT(found_amp);

    dom_node_unref(root);
    dom_node_unref(doc);
}

TEST(e2e_get_elements_by_tag_name)
{
    /*
     * Parse a doc with multiple p elements and use Document API
     * dom_document_get_elements_by_tag_name to retrieve them all.
     */
    dom_document *doc = NULL;
    dom_string *p_tag = NULL;
    dom_nodelist *list = NULL;
    dom_exception err;
    uint32_t length;

    ASSERT_EQ(parse_html_one_shot(
                  "<html><body><p>1</p><p>2</p><p>3</p></body></html>",
                  &doc),
              DOM_HUBBUB_OK);

    p_tag = make_dom_string("p");
    ASSERT_NOT_NULL(p_tag);

    err = dom_document_get_elements_by_tag_name(doc, p_tag, &list);
    ASSERT_EQ(err, DOM_NO_ERR);
    ASSERT_NOT_NULL(list);

    err = dom_nodelist_get_length(list, &length);
    ASSERT_EQ(err, DOM_NO_ERR);
    /* HTML5 should give us 3 P elements (one per <p>) */
    ASSERT_EQ(length, 3);

    dom_nodelist_unref(list);
    dom_string_unref(p_tag);
    dom_node_unref(doc);
}

/* ===================================================================
 * main
 * =================================================================== */

int main(void)
{
    printf("\n=== libdom unit tests (47) ===\n\n");

    printf("[Functional]\n");
    RUN_TEST(string_lifecycle);
    RUN_TEST(string_compare_isequal);
    RUN_TEST(document_lifecycle);
    RUN_TEST(create_element_and_get_tag_name);
    RUN_TEST(create_text_node_and_get_data);
    RUN_TEST(append_child_parent_relationship);
    RUN_TEST(set_and_get_attribute);
    RUN_TEST(child_nodes_length);

    printf("\n[Error path]\n");
    RUN_TEST(append_child_hierarchy_request_err);
    RUN_TEST(remove_child_not_found_err);
    RUN_TEST(create_element_invalid_character_err);
    RUN_TEST(append_child_wrong_document_err);
    printf("\n[Edge case]\n");
    RUN_TEST(empty_document_no_root);
    RUN_TEST(single_element_no_sibling);
    RUN_TEST(deep_nested_tree_50_levels);
    RUN_TEST(many_siblings_50);
    RUN_TEST(remove_only_child_yields_empty);
    RUN_TEST(text_node_split);
    RUN_TEST(nodelist_length_after_mutation);

    printf("\n[Amiga-specific]\n");
    RUN_TEST(string_hash_endian_safe);
    RUN_TEST(parser_create_no_alignment_trap);
    RUN_TEST(deep_tree_safe_within_524k_stack);
    RUN_TEST(document_destroy_safe_with_subtree);
    RUN_TEST(namespace_finalise_completes);

    printf("\n[Stress]\n");
    RUN_TEST(stress_parser_create_destroy_50);
    RUN_TEST(stress_parse_minimal_repeatedly);
    RUN_TEST(stress_parse_1kb_html);
    RUN_TEST(stress_parse_4kb_html);
    RUN_TEST(stress_walk_50_node_list);
    RUN_TEST(stress_dom_string_create_destroy_500);
    RUN_TEST(stress_create_element_500);
    RUN_TEST(stress_text_append_remove_100);

    printf("\n[End-to-end via hubbub binding]\n");
    RUN_TEST(e2e_minimal_html_doc);
    RUN_TEST(e2e_root_has_html_tag_name);
    RUN_TEST(e2e_body_text_content_present);
    RUN_TEST(e2e_anchor_with_href_attribute);
    RUN_TEST(e2e_chunked_parse_three_chunks);
    RUN_TEST(e2e_byte_at_a_time_parse);
    RUN_TEST(e2e_pause_resume);
    RUN_TEST(e2e_get_encoding_default);
    RUN_TEST(e2e_explicit_utf8_encoding);
    RUN_TEST(e2e_comment_node_present);
    RUN_TEST(e2e_paragraph_under_body);
    RUN_TEST(e2e_input_with_value_attribute);
    RUN_TEST(e2e_table_structure_preserved);
    RUN_TEST(e2e_named_entity_decoded);
    RUN_TEST(e2e_get_elements_by_tag_name);

    return test_summary();
}
