"""Tests for scripts/scrape-adcd.py utility functions."""

import importlib
import sys
import os

# Allow importing from scripts/ directory — the filename uses a hyphen,
# so we need importlib to load it as a module.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))
_mod = importlib.import_module("scrape-adcd")
slugify_title = _mod.slugify_title
convert_entities = _mod.convert_entities
extract_body = _mod.extract_body
extract_links = _mod.extract_links
extract_title = _mod.extract_title
convert_html_to_md = _mod.convert_html_to_md
resolve_links = _mod.resolve_links


SAMPLE_HTML = '''<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<HTML>
<head><title>20 Exec Memory Allocation / Memory Functions</title></head>
<body>
<a href="../Libraries_Manual_guide/node0003.html"><img src="../images/toc.gif" alt="[Contents]" border=0></a>
<a href="../Libraries_Manual_guide/node02A5.html"><img src="../images/prev.gif" alt="[Browse &#060;]" border=0></a>
<a href="../Libraries_Manual_guide/node02A8.html"><img src="../images/next.gif" alt="[Browse &#062;]" border=0></a>
<hr>
<pre>
<!-- AG2HTML: BODY=START -->
Normally, an application uses the <a href="../Includes_and_Autodocs_2._guide/node0332.html">AllocMem()</a> function to ask for memory:

    APTR AllocMem(ULONG byteSize, ULONG attributes);
<a name="line5"></a>
The byteSize argument is the amount of memory the application needs and
attributes is a bit field which specifies any special memory
characteristics (<a href="../Libraries_Manual_guide/node02A8.html">described later</a>).

 <a href="../Libraries_Manual_guide/node02A8.html">Memory Attributes</a>
 <a href="../Libraries_Manual_guide/node02A9.html">Allocating System Memory</a>
<!-- AG2HTML: BODY=END -->
</pre>
</body></html>'''


class TestSlugifyTitle:
    def test_basic(self):
        assert slugify_title("Hello World") == "hello-world"

    def test_special_chars(self):
        assert slugify_title("Exec Library (V39)") == "exec-library-v39"

    def test_truncation(self):
        long_title = "A" * 80
        result = slugify_title(long_title)
        assert len(result) <= 60

    def test_html_entities(self):
        assert slugify_title("Foo &amp; Bar") == "foo-bar"

    def test_collision(self):
        used = {"hello-world"}
        assert slugify_title("Hello World", used) == "hello-world-2"

    def test_collision_multiple(self):
        used = {"hello-world", "hello-world-2", "hello-world-3"}
        assert slugify_title("Hello World", used) == "hello-world-4"


class TestConvertEntities:
    def test_numeric_entity(self):
        assert convert_entities("&#169;") == "\u00a9"

    def test_named_lt_gt(self):
        assert convert_entities("&lt;stdio.h&gt;") == "<stdio.h>"

    def test_ampersand(self):
        assert convert_entities("foo &amp; bar") == "foo & bar"

    def test_no_entities(self):
        assert convert_entities("plain text") == "plain text"


class TestExtractBody:
    def test_with_markers(self):
        body = extract_body(SAMPLE_HTML)
        assert "AllocMem()" in body or "AllocMem" in body
        assert "Normally, an application" in body
        assert "AG2HTML" not in body

    def test_without_markers(self):
        html_no_markers = '''<html><body><pre>
Some content here
with multiple lines
</pre></body></html>'''
        body = extract_body(html_no_markers)
        assert "Some content here" in body

    def test_strips_anchor_names(self):
        body = extract_body(SAMPLE_HTML)
        assert '<a name="line5">' not in body
        assert '<a name=' not in body

    def test_strips_nav_images(self):
        body = extract_body(SAMPLE_HTML)
        assert '<img src="../images/' not in body
        assert "toc.gif" not in body


class TestExtractLinks:
    def test_extracts_relative_links(self):
        links = extract_links(SAMPLE_HTML)
        assert any("node02A8" in link for link in links)
        assert any("node0332" in link for link in links)

    def test_ignores_anchor_names(self):
        links = extract_links(SAMPLE_HTML)
        # anchor-only references like <a name="line5"> should not appear
        for link in links:
            assert not link.startswith("#") or "node" in link


class TestConvertHtmlToMd:
    def test_converts_links(self):
        body = '<a href="../Guide/node0001.html">Click here</a>'
        md = convert_html_to_md(body)
        assert "[Click here](../Guide/node0001.html)" in md

    def test_strips_remaining_html(self):
        body = '<b>bold</b> and <i>italic</i> text'
        md = convert_html_to_md(body)
        assert "<b>" not in md
        assert "<i>" not in md
        assert "bold" in md
        assert "italic" in md

    def test_detects_code_blocks(self):
        body = '''Some text before.

    APTR AllocMem(ULONG byteSize, ULONG attributes);
    if (mem == NULL) {
        return RETURN_FAIL;
    }

Some text after.'''
        md = convert_html_to_md(body)
        assert "```c" in md
        assert "```\n" in md
        assert "AllocMem" in md

    def test_preserves_ascii_art(self):
        body = '''Header text

    +--------+--------+
    | Field  | Value  |
    +--------+--------+

Footer text.'''
        md = convert_html_to_md(body)
        # ASCII art should NOT be wrapped in ```c
        assert "```c" not in md
        assert "+--------+" in md


class TestExtractTitle:
    def test_basic(self):
        title = extract_title(SAMPLE_HTML)
        assert "Memory Functions" in title

    def test_strips_prefix(self):
        html_with_prefix = '<html><head><title>Amiga(R) RKM Libraries: Exec Memory</title></head></html>'
        title = extract_title(html_with_prefix)
        assert title == "Exec Memory"
        assert "Amiga(R)" not in title


class TestResolveLinks:
    def test_known_node(self):
        node_map = {
            "Libraries_Manual_guide/node02A8": {
                "manual": "libraries",
                "path": "libraries/memory-attributes.md",
                "title": "Memory Attributes",
                "slug": "memory-attributes",
            }
        }
        md = "See [described later](../Libraries_Manual_guide/node02A8.html) for details."
        result = resolve_links(md, node_map, "libraries")
        assert "[described later](libraries/memory-attributes.md)" in result

    def test_unknown_node(self):
        node_map = {}
        md = "See [link](../Guide/node9999.html) here."
        result = resolve_links(md, node_map, "libraries")
        assert "[link](../Guide/node9999.html)" in result

    def test_cross_manual(self):
        node_map = {
            "Includes_and_Autodocs_2._guide/node0332": {
                "manual": "autodocs-2.0",
                "path": "autodocs-2.0/allocmem.md",
                "title": "AllocMem()",
                "slug": "allocmem",
            }
        }
        md = "Use [AllocMem()](../Includes_and_Autodocs_2._guide/node0332.html) to allocate."
        result = resolve_links(md, node_map, "libraries")
        assert "[AllocMem()](autodocs-2.0/allocmem.md)" in result
