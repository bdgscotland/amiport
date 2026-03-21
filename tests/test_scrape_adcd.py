"""Tests for scripts/scrape-adcd.py utility functions."""

import importlib
import sys
import os
import tempfile

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
detect_functions = _mod.detect_functions
generate_frontmatter = _mod.generate_frontmatter
load_known_functions = _mod.load_known_functions
build_func_to_library_map = _mod.build_func_to_library_map
detect_libraries = _mod.detect_libraries
detect_types = _mod.detect_types
extract_code_examples = _mod.extract_code_examples
KNOWN_AMIGA_TYPES = _mod.KNOWN_AMIGA_TYPES


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


class TestDetectFunctions:
    def test_matches_with_parens(self):
        known = {"AllocMem", "FreeMem", "Open"}
        result = detect_functions("Call AllocMem() to get memory", known)
        assert result == {"AllocMem"}

    def test_ignores_without_parens(self):
        known = {"Open", "Close", "AllocMem"}
        result = detect_functions("Open the window", known)
        assert result == set()

    def test_returns_unique_set(self):
        known = {"AllocMem", "FreeMem"}
        result = detect_functions(
            "Call AllocMem() first, then AllocMem() again", known
        )
        assert result == {"AllocMem"}

    def test_common_names_with_parens(self):
        known = {"Open", "Close", "AllocMem"}
        result = detect_functions("Call Open() then Close()", known)
        assert result == {"Open", "Close"}


class TestGenerateFrontmatter:
    def test_basic(self):
        fm = generate_frontmatter(
            title="Memory Allocation",
            manual="libraries",
            chapter="exec",
            section="memory",
            functions=["AllocMem", "FreeMem"],
            libraries=["exec.library"],
        )
        assert fm.startswith("---\n")
        assert fm.endswith("---\n")
        assert "title: Memory Allocation" in fm
        assert "manual: libraries" in fm
        assert "chapter: exec" in fm
        assert "section: memory" in fm
        assert "AllocMem" in fm
        assert "FreeMem" in fm
        assert "exec.library" in fm

    def test_empty_functions(self):
        fm = generate_frontmatter(
            title="Introduction",
            manual="libraries",
            chapter="intro",
            section="",
            functions=[],
            libraries=[],
        )
        assert fm.startswith("---\n")
        assert fm.endswith("---\n")
        assert "title: Introduction" in fm
        assert "functions: []" in fm
        assert "libraries: []" in fm


class TestLoadKnownFunctions:
    def test_loads_from_autodocs(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create a fake autodoc file
            content = """# exec.library — Autodoc Reference

## Function Index

- **AllocMem** (V36) — allocate memory
- **FreeMem** — free memory
- **AllocVec** (V36) — allocate memory and track size
"""
            with open(os.path.join(tmpdir, "exec.library.md"), "w") as f:
                f.write(content)

            # Create a README.md that should be skipped
            with open(os.path.join(tmpdir, "README.md"), "w") as f:
                f.write("# README\n\nThis is not an autodoc.\n")

            result = load_known_functions(tmpdir)
            assert "AllocMem" in result
            assert "FreeMem" in result
            assert "AllocVec" in result
            assert len(result) == 3


class TestBuildFuncToLibraryMap:
    def test_maps_functions_to_libraries(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            content = """# exec.library — Autodoc Reference

## Function Index

- **AllocMem** (V36) — allocate memory
- **FreeMem** — free memory
"""
            with open(os.path.join(tmpdir, "exec.library.md"), "w") as f:
                f.write(content)

            content2 = """# dos.library — Autodoc Reference

## Function Index

- **Open** — open a file
- **Close** — close a file
"""
            with open(os.path.join(tmpdir, "dos.library.md"), "w") as f:
                f.write(content2)

            result = build_func_to_library_map(tmpdir)
            assert result["AllocMem"] == "exec.library"
            assert result["FreeMem"] == "exec.library"
            assert result["Open"] == "dos.library"
            assert result["Close"] == "dos.library"


class TestDetectLibraries:
    def test_maps_functions_to_libraries(self):
        func_to_lib = {
            "AllocMem": "exec.library",
            "FreeMem": "exec.library",
            "Open": "dos.library",
        }
        result = detect_libraries({"AllocMem", "Open"}, func_to_lib)
        assert result == {"exec.library", "dos.library"}


class TestDetectTypes:
    def test_struct_keyword(self):
        result = detect_types("Pass a struct FileInfoBlock to Examine()")
        assert "FileInfoBlock" in result

    def test_known_type(self):
        result = detect_types("Pass the Window pointer to CloseWindow()")
        assert "Window" in result

    def test_ignores_lowercase(self):
        result = detect_types("the window was open")
        assert "Window" not in result

    def test_multiple_types(self):
        result = detect_types("Open a Screen and get the RastPort from it")
        assert "Screen" in result
        assert "RastPort" in result


class TestExtractCodeExamples:
    def test_detects_code_block(self):
        text = """Some intro text.

    #include <proto/exec.h>
    struct MsgPort *port;
    port = CreateMsgPort();
    if (port != NULL) {
        DeleteMsgPort(port);
    }

More text after."""
        result = extract_code_examples(text, "libraries", "exec-intro", "libraries/exec-intro.md")
        assert len(result) == 1
        assert "#include" in result[0]["code"]
        assert result[0]["library"] == "libraries"
        assert result[0]["section"] == "exec-intro"
        assert result[0]["source"] == "libraries/exec-intro.md"

    def test_skips_short_blocks(self):
        text = """Some text.

    int x = 1;
    int y = 2;
    int z = 3;

More text."""
        result = extract_code_examples(text, "libraries", "short", "libraries/short.md")
        assert len(result) == 0

    def test_skips_non_code(self):
        text = """Some text.

    This is just indented prose that
    happens to be indented with four
    spaces but does not contain any
    actual code markers or function
    calls or anything like that at all

More text."""
        result = extract_code_examples(text, "libraries", "prose", "libraries/prose.md")
        assert len(result) == 0
