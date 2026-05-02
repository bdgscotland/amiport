# lib/libdom

NetSurf libdom -- W3C DOM Level 3 implementation in C, plus the libhubbub
HTML5 binding that constructs DOM trees from HTML token streams.

Upstream: https://github.com/netsurf-browser/libdom @ commit `f69781e`
(v0.4.2). Copyright 2007-2020 J-M Bell + contributors, MIT-licensed
(see `COPYING`).

## What it is

LibDOM is the canonical DOM implementation used by the NetSurf browser.
This port supplies:

- Core DOM Level 3 (Node, Element, Document, Attr, Text, Comment,
  CDATASection, EntityReference, ProcessingInstruction, NodeList,
  NamedNodeMap, DocumentFragment, DocumentType, TokenList, Implementation,
  TypeInfo)
- DOM Level 3 Events (Event, EventTarget, EventListener, MouseEvent,
  MutationEvent, KeyboardEvent, TextEvent, UIEvent, CustomEvent,
  MutationNameEvent, MouseWheelEvent, MouseMultiWheelEvent)
- 57 HTML element subclasses (HTMLAnchorElement, HTMLBodyElement,
  HTMLInputElement, HTMLTableElement, HTMLFormElement, ... -- the full
  DOM-HTML binding for HTML 4)
- The libhubbub binding (`src/bindings/hubbub/parser.c`) which consumes
  libhubbub HTML5 tokens and constructs the corresponding libdom DOM tree

It is the third-tier dep beneath `ports/netsurf/` (NetSurf-Vampire Phase 1,
dep stack Phase D-prime). After this, only the small NetSurf-utility libs
(libnsutils, libnslog, libnspsl) and the layout libs (libcss, libsvgtiny)
remain before NetSurf itself.

## Subsystems

| Path | Files | LOC | Purpose |
|------|-------|-----|---------|
| `src/core/` | 18 | ~6K | DOM core (Node, Element, Document, Attr, ...) |
| `src/utils/` | 5 | ~1K | hashtable, namespace, validate, character_valid, walk |
| `src/events/` | 14 | ~2K | DOM Level 3 Events |
| `src/html/` | 57 | ~17K | HTML element subclasses |
| `src/bindings/hubbub/` | 1 | ~1K | libhubbub -> libdom binding (`parser.c`) |

We SKIP the upstream `bindings/xml/` directory (depends on expat or
libxml -- neither shipped in amiport).

## Public API

See `include/dom/`:

- `dom/dom.h` -- aggregator header; pulls in everything below
- `dom/inttypes.h`, `dom/functypes.h` -- baseline typedefs
- `dom/core/string.h` -- `dom_string` create/destroy/length/data/cmp/concat/replace
- `dom/core/document.h` -- `dom_document_*` (create_element/text/comment/cdata/pi,
  get_elements_by_tag_name, get_element_by_id, ...)
- `dom/core/node.h` -- `dom_node_*` (ref/unref, parent/sibling/child traversal,
  append_child/insert_before/remove_child, clone, normalize, ...)
- `dom/core/element.h` -- `dom_element_*` (get_tag_name, get/set/has/remove_attribute,
  get_elements_by_tag_name, ...)
- `dom/core/text.h` + `dom/core/characterdata.h` -- text/CDATA accessors and
  mutation (split_text, get_data, append/insert/delete/replace_data)
- `dom/core/comment.h`, `dom/core/cdatasection.h`, `dom/core/pi.h` --
  type-specific accessors
- `dom/core/nodelist.h`, `dom/core/namednodemap.h` -- collection traversal
- `dom/core/exceptions.h` -- `dom_exception` enum (DOM_NO_ERR + DOM Level 3
  exception codes + amiport-internal codes for DOM_NO_MEM_ERR,
  DOM_ATTR_WRONG_TYPE_ERR)
- `dom/events/events.h` + per-event-type headers -- event API
- `dom/html/*.h` -- 57 HTML element subclass headers
- `src/bindings/hubbub/parser.h` -- the HTML parser bridge entry point.
  NOT under `include/dom/` (it's an implementation detail of the binding)
  but IS the canonical NetSurf usage entry point: `dom_hubbub_parser_create`
  + `dom_hubbub_parser_parse_chunk` + `dom_hubbub_parser_completed` +
  `dom_hubbub_parser_destroy`.

## Build

```bash
make -C lib/libdom
```

Produces `libdom.a` (~272 KB after `-O1` promotion).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention as the prior libs (libwapcaplet, libparserutils, libhubbub).
See `lib/libwapcaplet/Makefile` header for the full rationale; in short:
the entire dep stack matches the `ports/netsurf/` consumer ABI exactly to
avoid mixed-CPU complexity.

**Defines:** `-DNDEBUG -D_DEFAULT_SOURCE -D_BSD_SOURCE -std=c99`. The
upstream Makefile sets `-U__STRICT_ANSI__` only in its AmigaOS branch;
our toolchain doesn't default to `__STRICT_ANSI__` so the `-U` is
redundant and omitted.

**Optimization:** whole-archive `-O1 -fno-strict-aliasing` after audit
2026-05-02. All 95 TUs are -O1-safe per crash-patterns #16: zero
struct-by-value returns >8 bytes (every libdom API returns by pointer
or scalar `dom_exception`), zero soft-float pulls (verified via
`m68k-amigaos-nm`), no large stack arrays, no alignment quirks. Same
whole-archive pattern as the 3 prior dep stack libs. See
`lib/libdom/PERF-REPORT.md` for the full audit.

**Depends on:** `lib/libhubbub` (via the hubbub binding), `lib/libparserutils`
(via libhubbub's transitive deps), `lib/libwapcaplet` (via dom_string's
backing store). Link order on consumer side: `-ldom -lhubbub
-lparserutils -lwapcaplet`.

## Test

```bash
make -C tests/libdom run
```

Runs the 47-test suite via `vamos -C 68040 -s 4096 -m 8192 ./test_libdom`.
Coverage:

- 8 functional (string lifecycle + cmp, document, element create + tag_name,
  text create + get_data, append_child parent relationship, set/get attribute,
  child_nodes_length)
- 4 error path (HIERARCHY_REQUEST_ERR, NOT_FOUND_ERR, INVALID_CHARACTER_ERR
  on empty tag, WRONG_DOCUMENT_ERR)
- 7 edge case (empty doc, single element no sibling, deep tree 50 levels,
  many siblings 50, remove only child, **text_node_split with parent**,
  nodelist length after mutation)
- 5 Amiga-specific (string hash endian-safe, parser create no alignment trap,
  deep tree 30-level safe within 524 KB stack, document destroy cleanly
  frees subtree, namespace_finalise completes)
- 8 stress (50-iter parser create+destroy, 30-iter parse minimal, 1KB parse,
  4KB parse, 50-item nodelist walk, 500-iter dom_string create+destroy,
  500-iter element create, 100-iter text append+remove)
- 15 end-to-end via the hubbub binding (minimal HTML, root tag name, body
  text content, anchor href attribute, chunked parse 3 chunks, byte-by-byte
  parse, pause+resume, get_encoding default, explicit UTF-8 encoding,
  comment node, paragraph under body, input value attribute, table
  structure, named entity decoded, getElementsByTagName)

## Critical: libdom CharacterData mutation needs a parent attached first

**This is an upstream libdom bug**, discovered during Stage 5 here.
Calling `dom_characterdata_delete_data`, `dom_characterdata_replace_data`,
or `dom_text_split_text` on a CharacterData node (text, comment, cdata,
processing_instruction) that has NOT been attached to a parent element
crashes with a NULL deref.

Root cause: the mutation function fires DOMSubtreeModified on the node's
parent via `_dom_dispatch_subtree_modified_event(doc, c->parent, &success)`
(`lib/libdom/src/core/characterdata.c:392`). When `c->parent` is NULL,
the dispatch function dereferences NULL to read its vtable and JSRs through
garbage. On bebbo-gcc 13.3 + vamos this manifests as a Bus Error
(`exc_num=02`) at PC=0x40a with A0=0x408.

The HTML5 hubbub binding never hits this in practice because every text
node it creates is immediately appended to an element. Direct API
consumers (test suites, scripts that build DOM fragments before
insertion) DO hit it.

**Workaround:** always attach character data nodes to a parent BEFORE
mutating their content. This matches the realistic "build tree top-down"
consumer pattern. The test_node_split test demonstrates: create a
container element, append the text to it, THEN call split_text. We've
chosen NOT to patch upstream libdom here -- the upstream upstream behaviour
is preserved so future libdom releases can be merged cleanly. Documented
in amiga-kb pitfall (same title) and cross-referenced from
`.claude/rules/known-pitfalls.md`.

## Critical: vamos resource sizing

Test binaries linking the FULL NetSurf-Vampire dep stack (libdom +
libhubbub + libparserutils + libwapcaplet) need at least **1 MB** for both
`__stack` and `__MEMORY_STEP`. The libhubbub-class threshold of 512 KB is
NOT enough -- libdom's larger code+data footprint pushes libnix's
startup-time allocation past 512 KB.

Symptoms below the threshold: pre-main Bus Error at PC=0x40a with NO
stdout output (libnix's stdio init fails before main).

Apply in test source:
```c
long __stack = 1048576;
unsigned long __MEMORY_STEP = 1048576;
```

And the test Makefile's `VAMOS_STACK = 4096` (4 MB) + `VAMOS_MEM = 8192`
(8 MB) handles vamos's runtime budget.

This extends the existing `feedback_libnix_stack_scales_with_binary` rule:
- libwapcaplet, libparserutils alone: 256 KB cookies fine
- libhubbub: 512 KB cookies needed
- libdom (this lib): 1 MB cookies needed
- ports/netsurf when it lands: probably 2-4 MB cookies

## Pre-generated files

There are NONE. libdom doesn't ship gperf-generated tables or perl-built
header tables (unlike libhubbub's `entities.inc` and `element-type.gperf`).
All the HTML element type lookup is done via switch dispatch in
`src/html/html_document.c:_dom_html_document_get_element_type` against
the static `__element_strings_g[]` table.

## Memory audit findings (Stage 6, 2026-05-02)

Memory-checker APPROVED with one CRITICAL-code / MID-practical finding
plus one mandatory consumer cleanup. See `lib/libdom/MEMORY-AUDIT.md` for
the full report.

**Finding 1 (consumer-side workaround documented):** `_dom_event_targets_expand`
at `src/core/node.c:2431` uses an unsafe realloc pattern that leaks the
old buffer if realloc fails. Practical risk is low (HTML5 trees are
shallow; the targets array rarely grows past its initial capacity), but
the leak is permanent on `-noixemul`. Consumer guidance: under sustained
memory pressure with deeply nested mutation events, prefer event
delegation patterns to avoid forcing realloc growth. Upstream patch is
the right fix; deferred.

**Finding 2 (mandatory consumer cleanup):** libdom maintains a single
process-wide global: `dom_string *dom_namespaces[DOM_NAMESPACE_COUNT]`
(defined in `src/utils/namespace.c`), lazily populated on first DOM
operation that touches namespaces. The documented exit hook
`dom_namespace_finalise()` (declared in `dom/dom.h`) frees the namespace
strings. On AmigaOS with `-noixemul`, **there is no process memory
reclamation on exit** -- so the downstream `ports/netsurf/` consumer MUST
call:

```c
dom_namespace_finalise();        /* ~620 bytes if not called */
lwc_iterate_strings(NULL, NULL); /* libwapcaplet's analogous hook */
```

before program exit to avoid permanent leaks. Each DOM tree must also be
freed by the consumer via `dom_node_unref(doc)` when finished.

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs: the test framework's `ASSERT_*` macros
return early from a failing test without running cleanup. Acceptable for
unit-test purposes (vamos host process exit reclaims memory) but not
representative of a real-world consumer leak.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- `dom_hubbub_parser_*` for
  HTML page rendering; `dom_node_*` and `dom_element_*` for layout,
  style cascade, JavaScript-event-target dispatch
- (future) `lib/libsvgtiny/` -- consumes libdom for SVG element trees
- (future) `lib/libnsutils/` etc. -- the small NetSurf utility libs may
  reference libdom typedefs but typically not the API
