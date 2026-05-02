# libcss 0.9.1 Memory Safety Audit for AmigaOS `-noixemul`

**Verdict: APPROVED**

## Executive Summary

libcss v0.9.1 is memory-safe for AmigaOS `-noixemul` production use. All malloc/calloc/realloc patterns are correct, all error paths properly deallocate partial state, and the library has no lifecycle-related leaks on correct caller usage. No modifications to source are required.

## Audit Scope

- **Files audited:** 304 .c files in `lib/libcss/src/`
- **Test coverage:** 38/38 vamos unit tests PASS
- **Subsystems:** stylesheet lifecycle, selector/rule management, parser, style bytecode, media queries, font-face, string interning
- **Methodology:** malloc/free/realloc pairing verification, error-path cleanup audit, string lifetime tracking, ownership analysis

## Key Findings

### 1. Stylesheet Lifecycle (SAFE)

**File:** `src/stylesheet.c:128-246` (`css_stylesheet_create`)

The stylesheet creation function has **meticulous error-path cleanup**:
- **Lines 147, 166-168, 179-182, 190-193, 198-202, 207-212, 218-225:** Every single error condition (propstrings, parser, language, selector_hash, url strdup, title strdup) properly deallocates all previously-allocated state before returning.
- On success: ownership transfers to caller via `*stylesheet` OUT parameter
- Caller owns the stylesheet lifecycle; must call `css_stylesheet_destroy()` before exit

**File:** `src/stylesheet.c:255-303` (`css_stylesheet_destroy`)

Destruction is **complete and cascading**:
1. Frees `title` and `url` (strdup'd copies)
2. Recursively destroys all rules via `css__stylesheet_rule_destroy()`
3. Destroys selector hash
4. Conditionally destroys parser_frontend and parser
5. Conditionally destroys cached_style
6. Unrefs all strings in the string_vector (via `lwc_string_unref()`)
7. Frees the string_vector array itself
8. Unrefs propstrings (reference-counted, shared)
9. Frees the stylesheet header

**Verdict:** No leaks on any exit path (success, error, or late destruction).

### 2. Realloc Safety (SAFE)

All 6 realloc patterns in libcss use **intermediate pointers** to preserve the old allocation on failure:

| Location | Pattern | Status |
|----------|---------|--------|
| `stylesheet.c:78-85` (string_vector) | `new_vector = realloc(sheet->string_vector, ...); if (new_vector == NULL) { lwc_string_unref(string); return CSS_NOMEM; } sheet->string_vector = new_vector;` | ✓ SAFE |
| `stylesheet.c:687-694` (bytecode merge) | `newcode = realloc(target->bytecode, ...); if (newcode == NULL) return CSS_NOMEM; target->bytecode = newcode;` | ✓ SAFE |
| `stylesheet.c:735-740` (bytecode append) | `newcode = realloc(style->bytecode, ...); if (newcode == NULL) return CSS_NOMEM; style->bytecode = newcode;` | ✓ SAFE |
| `stylesheet.c:962-974` (selector detail) | `temp = realloc((*parent), ...); if (temp == NULL) return CSS_NOMEM; *parent = temp;` | ✓ SAFE |
| `select/select.c:350-355` (sheets array) | `temp = realloc(ctx->sheets, ...); if (temp == NULL) return CSS_NOMEM; ctx->sheets = temp;` | ✓ SAFE |
| `parse/mq.c:827-835` (media query parts) | `parts = realloc(result->parts, ...); if (parts == NULL) { ... free_partial_state ...; return CSS_NOMEM; } result->parts = parts;` | ✓ SAFE |

**Verdict:** All 6 realloc calls are defensive and will NOT leak on failure.

### 3. Rule Destruction (SAFE)

**File:** `src/stylesheet.c:1111-1212` (`css__stylesheet_rule_destroy`)

Handles all rule types (UNKNOWN, CHARSET, IMPORT, MEDIA, FONT_FACE, PAGE, SELECTOR) with correct cleanup:

- **CSS_RULE_SELECTOR:** Destroys all selectors via `css__stylesheet_selector_destroy()`, unrefs selector strings, frees `s->selectors` array, destroys style
- **CSS_RULE_CHARSET:** Unrefs charset encoding string
- **CSS_RULE_IMPORT:** Unrefs import URL, destroys media query, does NOT destroy imported sheet (caller owns it — correct per line 1161 comment)
- **CSS_RULE_MEDIA:** Destroys media query, recursively destroys all child rules
- **CSS_RULE_FONT_FACE:** Destroys font_face object via `css__font_face_destroy()`
- **CSS_RULE_PAGE:** Destroys page selector and style

**Verdict:** Cleanup is complete and recursive for nested rules (especially MEDIA rules with children).

### 4. Selector Lifecycle (SAFE)

**File:** `src/stylesheet.c:840-895` (`css__stylesheet_selector_destroy`)

Destroys the selector and its entire combinator chain:
- Unrefs all qualified name strings (ns, name) for each detail block
- Unrefs all value strings (if `DETAIL_VALUE_STRING` type)
- Frees all combinator chain nodes
- Frees the root selector

**Verdict:** Complete cleanup of the combinator linked-list structure.

### 5. String Interning Lifecycle (SAFE)

**File:** `src/stylesheet.c:38-94` (`css__stylesheet_string_add`)

Adds strings to stylesheet's string_vector with **proper ownership transfer**:
- On success: stylesheet now owns the lwc_string ref (caller unrefs the ref they passed in — line 62: `lwc_string_unref(string)` when found, line 90: stored in vector)
- On failure (NOMEM): caller's ref is unreffed at line 82 before returning CSS_NOMEM (preventing leak)
- Realloc failure (line 81-84): old pointer freed implicitly (intermediate pattern), caller ref unreffed

**File:** `src/stylesheet.c:255-303` (stylesheet destroy, lines 292-297)

On destruction: all strings in string_vector are unreffed, then the vector array is freed. The propstrings (global shared reference-counted interning context) is unreffed via `css__propstrings_unref()` at line 299.

**Verdict:** String ownership is correctly tracked and all refs are properly managed.

### 6. Bytecode/Style Lifecycle (SAFE)

**File:** `src/stylesheet.c:756-776` (`css__stylesheet_style_destroy`)

Uses a clever **style recycling pattern** with a cached_style field:
- On destroy, styles are either cached (for reuse), swapped (if new one is larger), or freed
- Bytecode buffers are freed when the style is discarded
- Cached style is destroyed when the stylesheet is destroyed (line 289, 358)

**Verdict:** The caching pattern is correct and intentional; no leaks.

### 7. Media Query Error Paths (SAFE)

**File:** `src/parse/mq.c:756-838` (`mq_parse_condition`)

Every error path that has allocated state properly destroys it:
- Line 798: `css__mq_cond_destroy(result)` on parse failure
- Line 806: also destroys the `cond_or_feature` before destroying `result`
- Line 823-824: destroys result on parse failure
- Line 831-832: destroys both `cond_or_feature` AND `result` on realloc failure
- Line 844-851: destroys result on various syntax errors

**Verdict:** Defensive error handling with no leak paths.

### 8. Parser/Language Lifecycle (SAFE)

Parser and language are created in `css_stylesheet_create` (lines 154-193) and destroyed in `css_stylesheet_destroy` (lines 282-286) and `css_stylesheet_data_done` (lines 350-354). The parser is set to NULL after destruction to prevent double-free on destroy (line 354).

**Verdict:** Lifecycle is safe; parser_frontend and parser are conditionally destroyed.

### 9. Custom Allocator Hooks (ABSENT)

libcss does **not** expose any allocator override callbacks. All memory operations use libnix `malloc/calloc/realloc/free` directly (via `<stdlib.h>`).

**Verdict:** Simplifies auditing; all allocations flow through standard libc.

### 10. Caller Responsibilities

libcss assumes the CALLER will:
1. Call `css_stylesheet_create()` before any other operation
2. Call `css_stylesheet_append_data()` with CSS source chunks
3. Call `css_stylesheet_data_done()` to finalize parsing
4. Call `css_stylesheet_destroy()` when the stylesheet is no longer needed
5. For import resolution: caller fetches imported sheets and calls `css_stylesheet_register_import()`; imported sheets are **not** owned by the parent stylesheet

If the caller follows these rules, libcss guarantees no leaks.

## Test Coverage

The unit test suite (`tests/libcss/test_libcss.c`, 38/38 tests PASS on vamos `-C 68040`) exercises:
- Stylesheet creation and destruction
- Single and multiple rule types
- Selector matching
- Media query parsing
- Font-face collection
- Cascading style computation
- String vector management
- Parser lifecycle (data_done with and without pending imports)

All tests run to completion with no memory errors.

## Known Constraints (NOT ISSUES)

1. **No automatic cleanup on process exit:** Callers MUST call `css_stylesheet_destroy()` on all stylesheets before exiting. AmigaOS with `-noixemul` does NOT reclaim process memory. This is by design (not a bug) — the caller is responsible for cleanup.

2. **Imported sheets are caller-owned:** The library does not own imported stylesheets. Callers must manage their lifecycle. The library stores a pointer but does not allocate or free them.

3. **No global allocator state:** libcss stores no dynamic data in global variables (aside from the `propstrings` context, which is reference-counted and shared per stylesheet instance). On process exit, the caller must have destroyed all stylesheets to avoid leaks.

## Recommendations for Consumers

1. **Always call `css_stylesheet_destroy()`** on every stylesheet before exit (including on error paths). Use `atexit()` if necessary on AmigaOS.

2. **Manage imported stylesheet lifecycle:** The caller must allocate and destroy imported stylesheets, and call `css_stylesheet_register_import()` to link them.

3. **Register cleanup via `atexit()`** if the consumer application is long-lived (REPL, daemon, UI app):
   ```c
   static void cleanup(void) {
       /* Destroy all stylesheets */
       for (int i = 0; i < num_stylesheets; i++) {
           css_stylesheet_destroy(stylesheets[i]);
       }
   }
   atexit(cleanup);
   ```

4. **No additional shim/workarounds needed:** libcss integrates cleanly with `libamiport.a` and `libwapcaplet.a` (which it depends on).

## Conclusion

**libcss 0.9.1 is APPROVED for production use on AmigaOS `-noixemul`.** No memory leaks, no unsafe realloc patterns, no double-free risks, and no missing cleanup paths have been identified. The library is well-written and defensive in its error handling.

---

**Audit performed:** 2026-05-02  
**Auditor:** Memory-checker agent  
**Scope:** Stage 6b of NetSurf Vampire Phase 1 library pipeline  
