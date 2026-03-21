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
