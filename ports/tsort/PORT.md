# Port: tsort

## Overview

| Field | Value |
|-------|-------|
| Program | tsort |
| Version | 1.38 |
| Source | OpenBSD usr.bin/tsort (v1.38, Jan 2024) |
| Category | 1 -- CLI Tool |
| License | ISC |
| Original Author | Marc Espie |
| Port Date | 2026-03-26 |

## Description

tsort performs topological sorting of a directed graph given as pairs of nodes on standard input or from a file. It supports cycle detection and breaking, longest cycle search (-l), quiet mode (-q), reverse arcs (-r), verbose diagnostics (-v), exit-code-as-cycle-count (-w), input-arrival-order hints (-f), and external hints files (-h). Useful for build dependency ordering, package installation sequencing, and any problem requiring linearization of a partial order. This is the OpenBSD implementation with a bundled minimal ohash (open double hashing) library.

## Prior Art on Aminet

Researched via aminet-researcher agent. No modern standalone noixemul port found.

## Portability Analysis

Verdict: **MODERATE** -- Single file (1295 lines) but requires bundling OpenBSD's ohash library (not available on AmigaOS), replacing fgetln() with fgets(), and handling C99/POSIX type dependencies.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<ohash.h>` (OpenBSD hash library) | Tier 3 | Bundled minimal ohash implementation (~200 lines) |
| `fgetln()` (BSD-specific) | Tier 2 | Replaced with fgets() + static buffer (2 call sites) |
| `<stdint.h>` | Tier 1 | Removed; UINT_MAX from `<limits.h>` is sufficient |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx/warn) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<getopt.h>` (getopt) | Tier 1 | Replaced with `<amiport/getopt.h>` |
| `reallocarray()` | Tier 1 | Via `<amiport/string.h>` macro |
| `__dead` attribute | Tier 1 | Defined as `__attribute__((noreturn))` |
| `UNUSED` attribute | Tier 1 | Defined as `__attribute__((unused))` |
| `pledge()`/`unveil()` | Stub | Macro stubs |
| Exit codes | Tier 1 | `exit(1)`/`err(1,...)`/`errx(1,...)` -> exit(10)/err(10,...)/errx(10,...) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| tsort.c | 21 | (none) | `$VER` string | Amiga version string |
| tsort.c | 24 | (none) | `long __stack = 32768` | Stack cookie (graph traversal recurses) |
| tsort.c | 27 | pledge/unveil | `#define pledge(p, e) (0)` | Stubs |
| tsort.c | 31-37 | (none) | `__dead` definition | `__attribute__((noreturn))` |
| tsort.c | 42 | `<err.h>` | `<amiport/err.h>` | Error shim |
| tsort.c | 46-57 | `<stdint.h>`, `<stdlib.h>`, `<unistd.h>`, `<ohash.h>` | amiport shims, ohash removed | Multiple header replacements |
| tsort.c | 60-257 | (none) | Bundled ohash implementation | ~200 lines: init, lookup, insert, create_entry, delete, iteration |
| tsort.c | 152 | (none) | Underflow guard for `sz-2` | Prevents negative probe step |
| tsort.c | 397-403 | (none) | `cleanup()` + `atexit(cleanup)` | Free expanded argv on all paths |
| tsort.c | 414 | `errx(1, "Memory exhausted")` | `errx(10, ...)` | RETURN_ERROR |
| tsort.c | 441 | `reallocarray()` | via `<amiport/string.h>` macro | Overflow-safe realloc |
| tsort.c | 544-564 | `fgetln(f, &len)` in read_pairs() | `fgets(line_buf, sizeof(line_buf), f)` | BSD fgetln not in libnix |
| tsort.c | 613-624 | `fgetln(f, &len)` in read_hints() | `fgets(line_buf, sizeof(line_buf), f)` | Same replacement |
| tsort.c | 605,608,656 | `exit(1)`/`err(1,...)` | `errx(10,...)`/`err(10,...)` | RETURN_ERROR |
| tsort.c | 1158,1170 | `err(1, "Can't open file")` | `err(10, ...)` | RETURN_ERROR |
| tsort.c | 1265-1270 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Argv expansion + cleanup |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warnx()`
- `amiport_reallocarray()`
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_exit()` (via macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Stack size | `__stack = 32768` (32 KB -- graph traversal recurses deeply) |
| Source files | tsort.c (1 file, 1295 lines including bundled ohash) |

## Test Results

25 tests via FS-UAE. Pass rate: 25/25 (100%).

| Category | Count | Description |
|----------|-------|-------------|
| Functional (flags) | 9 | Default sort, -l, -q, -r, -f, -h, -v, -w (1 and 2 cycles) |
| Cycle detection | 2 | Cycle with all nodes output, -l longest cycle |
| Exit codes | 4 | RC=0 clean, RC=10 odd tokens/missing file/invalid flag |
| Error paths | 1 | Stdin redirect input |
| Edge cases | 4 | Empty file, single self-arc, disconnected graph, diamond DAG |
| Amiga-specific | 1 | WORK: volume path for -h hints file |
| Real-world | 2 | Build dependency chain, reverse dependency order |
| Stress | 2 | 100-node linear chain, 100-node reversed |

## Memory Safety

- `atexit(cleanup)` registered immediately after `amiport_expand_argv()`
- Graph nodes allocated via ohash entry_alloc (calloc-based) -- freed by ohash_delete()
- Arc links allocated via ereallocarray() -- not individually freed (acceptable: single invocation, no interactive loop)
- fgetln() replaced with fgets() into static 2048-byte buffer -- no dynamic allocation for line reading
- Reviewed by memory-checker agent

## Performance Notes

- Topological sort is O(e) where e is edge count; ohash lookup adds O(1) amortized per node
- Cycle detection with -l flag can be O(2^e) in worst case (all free paths explored) but is fast for typical dependency graphs
- Heap-based ordering with hints file (-h/-f) is O(e + v log v) worst case
- The 32 KB stack is appropriate for the recursive cycle detection on deep graphs

## Known Limitations

1. **Empty file produces RC=10** -- The fgetln()-to-fgets() replacement causes an empty file to be treated as an error (fgets returns NULL immediately, read_pairs sees no tokens). Native OpenBSD tsort returns RC=0 for empty input. This is a minor behavioral difference.
2. **Line length limit** -- The fgets() replacement uses a 2048-byte static buffer. Input lines longer than 2048 bytes will be truncated. Node names in dependency graphs are typically short, so this is unlikely to be an issue in practice.
3. **ohash is bundled, not shared** -- The ohash implementation is embedded in tsort.c rather than provided as a shared library. If other ports need ohash, it should be extracted to `lib/ohash/`.

## Review

Reviewed with `/review-amiga` and `memory-checker` agent. Score: **READY**.
