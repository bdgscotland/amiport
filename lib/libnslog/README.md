# lib/libnslog

NetSurf logging library -- category-based logger with text-defined
filter language, one-shot startup buffering ("cork"), and pluggable
render callback.

Upstream: https://github.com/netsurf-browser/libnslog @ commit
`bedff21` (v0.1.3). Copyright 2014-2017 Vincent Sanders + Daniel
Silverstone, MIT-licensed (see `COPYING`).

## What it is

Libnslog provides a small (4 TUs / ~600 LOC hand-written + ~6 KB
generated flex/bison) hierarchical logger:

- 7 log levels (DEEPDEBUG / DEBUG / VERBOSE / INFO / WARNING / ERROR /
  CRITICAL) with compile-time + runtime threshold filtering
- Hierarchical categories declared via `NSLOG_DEFINE_CATEGORY` /
  `NSLOG_DEFINE_SUBCATEGORY` macros, lazy path normalisation
  (`testcat/testchild/grandchild`)
- One-shot "cork" mechanism that buffers messages until the consumer
  installs a render callback and calls `nslog_uncork()`
- Reference-counted filter trees buildable programmatically (each
  `nslog_filter_*_new()`) or from a textual filter language parsed via
  flex+bison
- Filter language supports level thresholds, category prefixes,
  filename / dirname / funcname substring matches, and AND/OR/XOR/NOT
  composition with grouping parens

It is the eighth library shipped in the NetSurf Vampire Phase 1 dep
stack (Phase D-prime), the first lib in Wave 3. Standalone -- no
NetSurf-internal dependencies.

## Public API

See `include/nslog/nslog.h`. Core entry points:

- `NSLOG(catname, LEVEL, "fmt", args...)` -- the canonical logging
  macro. catname is a bareword, LEVEL is a bareword (DEBUG, INFO,
  WARNING, etc.). Compile-time elided if level < `NSLOG_COMPILED_MIN_LEVEL`.
- `nslog_set_render_callback(cb, ctx)` -- install the consumer's
  message handler. cb signature is `void cb(void*, nslog_entry_context_t*, const char*, va_list)`.
- `nslog_uncork()` -- transition out of corked mode (drains buffered
  messages through the callback). Returns `NSLOG_NO_ERROR` on first
  call, `NSLOG_UNCORKED` thereafter (one-shot).
- `nslog_cleanup()` -- mandatory at exit. Frees category names, drains
  any residual cork chain, releases active filter.
- `nslog_filter_from_text("level: WARNING")` -- parse a textual filter.
  See **Filter language** below.
- `nslog_filter_set_active(filter, &prev_out)` -- install global
  filter; previous filter (if any) returned via `prev_out` (caller
  responsible for ref/unref).
- `nslog_filter_ref` / `nslog_filter_unref` -- refcount management;
  unref(NULL) is safe.

## Build

```bash
make -C lib/libnslog
```

Produces `libnslog.a` (~21 KB).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention as the prior libs.

**Defines:** `-DNDEBUG -D_DEFAULT_SOURCE -std=c99`.

**Optimization:** whole-archive `-O1 -fno-strict-aliasing` per Stage 7
audit (see `PERF-REPORT.md`). All four TUs cleared crash-patterns #16
(no struct returns >8 bytes) and #2 (no soft-float pulls).

**Depends on:** nothing (only libc/libnix). Standalone library.

**Source-analyzer fix applied at `src/core.c:137`:** original
`vsnprintf(NULL, 0, ...)` would crash on libnix per crash-patterns #5
(libnix vsnprintf does NOT support NULL destination, writes to address
zero). Replaced with a 1024-byte stack probe buffer. Documented in
`Makefile` header.

## Pre-generated flex / bison output

The filter parser is implemented via flex (`src/filter-lexer.l`) and
bison (`src/filter-parser.y`). Both `.l` and `.y` are vendored
alongside their pre-generated outputs (`filter-lexer.h`, `.inc`,
`filter-parser.c`, `.h`) so the library-build does NOT depend on host
flex/bison installation.

To regenerate after upstream changes:

```bash
# macOS host: install Homebrew bison (system bison 2.3 is too old)
brew install bison

# Regenerate
/opt/homebrew/opt/bison/bin/bison -d -t \
    --define=api.prefix='{filter_}' --report=all \
    --output=src/filter-parser.c --defines=src/filter-parser.h \
    src/filter-parser.y

flex --outfile=src/filter-lexer.inc \
     --header-file=src/filter-lexer.h \
     src/filter-lexer.l
```

The `src/filter-lexer.c` file is a 3-line wrapper around `filter-lexer.inc`
(per upstream Makefile pattern); it does not need regeneration.

## Filter language

```
level: WARNING                  # threshold filter (UPPERCASE level names)
cat: netsurf/render             # category prefix
file: parser.c                  # filename substring
dir: src/render                 # dirname substring
func: render_node               # function name substring

(level: WARNING) && (cat: netsurf/render)
!(level: DEBUG)
(file: parse) || (file: lex)
```

**LEVEL NAMES ARE UPPERCASE.** Lowercase fails parsing. Recognised
spellings: `DEEPDEBUG` / `DDEBUG` / `DD`, `DEBUG` / `DBG`, `VERBOSE` /
`CHAT`, `INFO`, `WARNING` / `WARN`, `ERROR` / `ERR`, `CRITICAL` / `CRIT`.

## Test

```bash
make -C tests/libnslog run
```

Runs the 25-test suite via `vamos -C 68040 -s 1024 -m 4096 ./test_libnslog`.
Coverage:

- 10 functional (level naming, callback dispatch, cork->uncork,
  uncorked dispatch, filter parsing, refcounting, filter dispatch,
  category hierarchy)
- 5 error path (invalid filter syntax, unbalanced parens, garbage,
  NULL-safe unref, double-uncork returns NSLOG_UNCORKED)
- 4 edge case (empty filter rejected, filter at CRITICAL blocks lower,
  filter at DEEPDEBUG passes all, 1100-char log message)
- 2 Amiga-specific (2 KB log message exercises probe buffer +
  corked path, cleanup+reinit cycle)
- 4 stress (50 cork/uncork cycles, 50 filter create+destroy, 20-deep
  AND tree, 4 KB log message)

The first test (`corked_buffers_messages_until_uncork`) MUST run first
-- libnslog's cork state is process-wide one-shot. After uncork all
subsequent tests run in immediate-dispatch mode.

## CRITICAL design fact: cork is one-shot

`nslog_uncork()` performs a one-time process-wide transition from
buffered to direct-dispatch logging. **`nslog_cleanup()` does NOT
reset cork state.** This is intentional upstream behaviour for
"buffer early-startup logs, drain when callback is installed". After
uncorking, all subsequent `NSLOG()` calls dispatch immediately
through the active callback, no more buffering possible.

Consumer pattern:
```c
nslog_set_render_callback(my_handler, my_ctx);
/* (optionally) ... NSLOG() calls here are buffered ... */
nslog_uncork();
/* ... NSLOG() calls here dispatch directly ... */
/* at program exit: */
nslog_cleanup();
```

## AmigaOS exit cleanup

`nslog_cleanup()` is **mandatory** before exit on AmigaOS `-noixemul`,
which does NOT reclaim process memory at exit. Without cleanup,
~500 bytes to 2 KB leaks permanently per invocation (active filter
tree + category names + cork chain residue).

If using `lib/libwapcaplet/` in the same consumer, also call
`lwc_iterate_strings(NULL, NULL)` AFTER `nslog_cleanup()` -- that
handles libwapcaplet's global string interning context. Order
matters because libnslog's category names (if any are interned by
the consumer) outlive the cleanup transitively.

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs: the test framework's `ASSERT_*`
macros return early from a failing test without running cleanup.
Acceptable for unit-test purposes (vamos host process exit reclaims
memory) but not representative of a real-world consumer leak.

## Memory audit findings

See `lib/libnslog/MEMORY-AUDIT.md` for the Stage 6 report. APPROVED
with no critical findings. Reference-counting is sound, error paths
are guarded, cork chain is properly drained on uncork+cleanup.

## Performance audit findings

See `lib/libnslog/PERF-REPORT.md` for the Stage 7 report. Whole-
archive -O1 is optimal -- per-file -O2 promotion offers ~10-20% on
filter dispatch but logging is < 1% of NetSurf frame time so the
safety margin from -O1 wins.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- developer logging
  for the browser
- (potentially) `lib/libnsutils/`, other Wave 3 NetSurf libs that
  may use libnslog for diagnostic output during development
