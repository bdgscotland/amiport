# Glob & Argv Wildcard Expansion Shim

**Date:** 2026-03-21
**Status:** Approved (eng review passed)
**Branch:** main

## Problem

AmigaOS shells do not expand wildcards in command-line arguments. When a user types `myprog *.c`, the program receives the literal string `"*.c"` in argv. POSIX programs expect the shell to have already expanded globs before main() runs. This breaks every ported CLI tool that processes filename arguments.

SAS/C, DICE, and Lattice compilers solved this with automatic argv expansion in their startup code, with `int __nowild = 1;` as the opt-out. bebbo-gcc/libnix has no equivalent — this is a gap.

Additionally, the POSIX `glob(3)` API (`glob()`/`globfree()`) is missing from libnix, flagged as a gap in `docs/references/newlib-availability.md`.

## Solution

Two layers, both in `lib/posix-shim/` (Tier 1):

### Layer 1: POSIX glob()/globfree() API

New files: `src/glob.c`, `include/amiport/glob.h`

Built on existing primitives:
- `amiport_opendir()`/`amiport_readdir()`/`amiport_closedir()` for directory iteration
- `amiport_fnmatch()` for pattern matching

Supports both Unix (`*`, `?`, `[...]`) and AmigaOS (`#?`, `~(...)`, `(a|b)`) patterns.
AmigaOS patterns are detected and translated to Unix equivalents before matching:
- `#?` → `*`
- `#x` → `x*` (repeat)
- `(a|b)` → handled via iterative matching
- `~(foo)` → negation (best-effort; falls back to literal if untranslatable)

fnmatch stays pure POSIX — the translation layer lives in glob.c.

**Flags supported:** `GLOB_NOSORT`, `GLOB_APPEND`, `GLOB_NOCHECK`, `GLOB_MARK`, `GLOB_ERR`
**Not supported:** `GLOB_TILDE` (no home dir), `GLOB_BRACE` (bash-specific), recursive `**`

**Returns:** `GLOB_NOMATCH`, `GLOB_NOSPACE`, `GLOB_ABORTED` per POSIX spec.

### Layer 2: Argv Wildcard Expansion

New file: `src/argv_expand.c`

Public API:
```c
/* Expand wildcard arguments in argv. Call at top of main(). */
void amiport_expand_argv(int *argc, char ***argv);

/* Free expanded argv. Call before _exit(). */
void amiport_free_argv(void);
```

Behavior:
1. Check `extern int __nowild` (weak symbol). If set to 1, return immediately.
2. For each `argv[i]` (i > 0, not starting with `-`):
   - Call shared `glob_has_magic()` to detect wildcard chars
   - If wildcards found, call `amiport_glob()` with `GLOB_NOCHECK`
   - Replace argv entry with expanded results
3. Allocate new argv array; original is untouched.
4. `amiport_free_argv()` frees all allocated strings and the array.

### Shared Helper

`glob_has_magic(const char *pattern)` — declared in `amiport/internal.h`, used by both glob.c and argv_expand.c. Detects Unix (`*`, `?`, `[`) and AmigaOS (`#`, `~`, `(`) wildcard characters. Single source of truth for "is this a pattern?"

## Decisions (from eng review)

| # | Decision | Rationale |
|---|---|---|
| 0B | Both Unix + AmigaOS patterns | Full SAS/C parity, minimal extra cost |
| 1A | Explicit call in main() | Explicit > clever; code-transformer adds it mechanically |
| 2A | Individual mallocs + free loop | Matches existing shim pattern; memory-checker friendly |
| 3A | Both in posix-shim | One library, no new link flags |
| 4A | Shared glob_has_magic() helper | DRY — one place to update when adding pattern chars |
| 5B | Translate Amiga→Unix in glob | fnmatch stays pure POSIX; clean separation |
| 6A+B | Unit + integration tests | Function-level + vamos end-to-end |
| 7A | One scan per arg | AmigaOS disk caching handles repeats |

## Files

### New
| File | Purpose |
|------|---------|
| `lib/posix-shim/src/glob.c` | POSIX glob()/globfree() + Amiga pattern translation |
| `lib/posix-shim/include/amiport/glob.h` | Public header: glob_t, GLOB_* constants |
| `lib/posix-shim/src/argv_expand.c` | Startup argv expansion + __nowild + cleanup |
| `tests/shim/test_glob.c` | glob API unit tests |
| `tests/shim/test_argv_expand.c` | argv expansion unit + integration tests |

### Modified
| File | Change |
|------|--------|
| `lib/posix-shim/Makefile` | Add glob.c, argv_expand.c to SRCS |
| `lib/posix-shim/include/amiport/internal.h` | Add glob_has_magic() declaration |
| `.claude/skills/transform-source/references/transformation-rules.md` | Section 10: glob rules + __nowild |
| `.claude/skills/analyze-source/references/common-patterns.md` | New "Wildcard Arguments" pattern |
| `docs/posix-tiers.md` | Move glob/globfree to Tier 1 |
| `docs/references/newlib-availability.md` | Update Glob section |
| `CLAUDE.md` | Add __nowild to Known Pitfalls |
| `TODOS.md` | Mark glob item done; add __nowild retrofit TODO |

## Memory Model

```
Before expansion:          After expansion:
argv[0] = "grep"           new_argv[0] = "grep"        (original, not copied)
argv[1] = "-i"             new_argv[1] = "-i"          (original, not copied)
argv[2] = "*.c"            new_argv[2] = "foo.c"       (malloc'd)
argv[3] = NULL             new_argv[3] = "bar.c"       (malloc'd)
                           new_argv[4] = "baz.c"       (malloc'd)
                           new_argv[5] = NULL

Static globals in argv_expand.c:
  expanded_argv = new_argv   (the array itself, malloc'd)
  expanded_count = 3         (number of malloc'd strings)
  expanded_strings[] = {     (pointers to free)
    "foo.c", "bar.c", "baz.c"
  }

amiport_free_argv():
  free each expanded_strings[i]
  free expanded_argv
  set all to NULL (safe for double-call)
```

## Not In Scope

- `GLOB_TILDE` / `GLOB_BRACE` — no Amiga equivalent, no demand
- `wordexp()` — massively complex, no port needs it
- Recursive glob (`**/*.c`) — not POSIX
- Retrofitting `__nowild` to existing ports — separate follow-up
- Path translation shim (`/tmp` → `T:`) — independent TODOS.md item

## Test Coverage

See test diagram in eng review. Key categories:
- **glob():** Unix patterns, AmigaOS patterns, no matches, GLOB_NOCHECK, GLOB_APPEND, GLOB_ERR, memory exhaustion, path splitting with volume prefixes
- **globfree():** Normal free, double-free safety
- **glob_has_magic():** Unix chars, Amiga chars, literals, escaped chars
- **Amiga→Unix translation:** #?→*, (a|b), ~(foo), passthrough
- **argv expansion:** no wildcards, with matches, no matches (kept literal), __nowild suppression, argv[0] preserved, option args preserved, cleanup
- **Integration:** vamos binary with wildcard args
