# lib/libwapcaplet

NetSurf string interning library, ported to AmigaOS 3.x for the
NetSurf Vampire Phase 1 dep stack.

Upstream: https://github.com/netsurf-browser/libwapcaplet @ commit
`c7c128d` (v0.4.3). Copyright 2009 The NetSurf Browser Project,
Daniel Silverstone, MIT-licensed (see `COPYING`).

## What it is

LibWapcaplet provides a reference-counted string internment system
optimised for storing many small strings (CSS selectors, HTML element
and attribute names, URLs, etc.) and comparing them rapidly via pointer
equality. Caseless comparison is supported via lazy-allocated
"insensitive" twins.

It is the foundational dep for NetSurf's parser stack: libcss, libdom,
libhubbub, libsvgtiny, libnsutils, libnspsl all link against it.

## Public API

See `include/libwapcaplet/libwapcaplet.h`. The core surface:

- `lwc_intern_string(s, slen, &out)` -- intern a string, returns refcnt'd ptr
- `lwc_intern_substring(parent, off, len, &out)` -- intern a substring slice
- `lwc_string_tolower(str, &out)` -- get a lowercase variant (refcnt'd)
- `lwc_string_destroy(str)` -- forced free (normally not called directly)
- `lwc_iterate_strings(cb, pw)` -- visit all interned strings; clean up
  global context if no strings remain

Plus header-inline macros for `ref` / `unref` / `isequal` /
`caseless_isequal` / `data` / `length` / `hash_value` /
`caseless_hash_value`.

## Build

```bash
make -C lib/libwapcaplet
```

Produces `libwapcaplet.a` (~3 KB).

**CPU target:** `-m68040 -m68881`. This deviates from the project's
standard `-m68000` library convention (`.claude/rules/known-pitfalls.md`
"Libraries MUST Use -m68000") on purpose: this lib is part of the
NetSurf Vampire Phase 1 dep stack, all of which is built to match the
final `ports/netsurf/` consumer ABI exactly. See the Makefile header
for the full rationale and the `2026-05-02-netsurf-vampire-phase-d-prime-dep-stack.md`
plan addendum for the project decision.

**Optimization:** `-O1` (audited 2026-05-02 against
crash-patterns #16 -- single TU, no struct-by-value returns >8 bytes,
no float math, no alignment traps; clean for -O1 with bebbo-gcc 13.3).

## Test

```bash
make -C tests/libwapcaplet run
```

Runs the 36-test suite via `vamos -C 68040`. Coverage:

- 13 functional tests (all 6 extern entry points + 8 macros)
- 2 error-path tests (range error variants; OOM is not vamos-testable)
- 6 edge-case tests (empty string, single char, full-length substring, etc.)
- 5 reference-counting lifecycle tests (incl. self-insensitive sentinel)
- 4 caseless-behaviour tests
- 2 Amiga-specific (hash collision + 8 KB string allocation)
- 3 stress / real-world (1000 unique strings + iterate cleanup + substring chain)
- 1 memory-checker-audit verification (self-insensitive destroy path)

OOM path is not exercisable on vamos without a custom allocator hook
(libwapcaplet doesn't provide one); the upstream Check-based suite
covers it.

## AmigaOS exit cleanup discipline -- IMPORTANT

LibWapcaplet maintains a process-wide `static lwc_context *ctx` global
that holds the 4091-bucket hash table and all interned strings. The
context is freed automatically by `lwc_iterate_strings()` when called
with zero strings remaining (libwapcaplet.c:286-291).

On AmigaOS with `-noixemul`, **there is no process memory reclamation
on exit**. Consumers MUST follow this lifecycle:

1. Unref every interned `lwc_string *` they hold (or accept the leak
   and rely on iterate-with-callback to chase them down).
2. Call `lwc_iterate_strings(NULL, NULL)` (or any no-op callback) to
   trigger the global cleanup branch when no strings remain.
3. After the call, `ctx` is `NULL` again -- subsequent intern calls
   re-initialise transparently.

A NetSurf-MUI session that interns ~5000 strings and exits without
this cleanup permanently leaks ~50-500 KB of fast RAM until reboot.
The downstream `ports/netsurf/` Makefile consumer is responsible for
calling the cleanup at shutdown -- this is documented in PORT.md once
that port stage lands.

## Self-insensitive destroy path -- audited safe

The Stage 6 memory-checker audit raised a concern that the
`lwc_string_destroy()` recursive-unref path could double-free a string
whose `insensitive` pointer points to itself (the "already-lowercase"
sentinel). Source trace and `TEST(self_insensitive_destroy_path)`
empirically confirm the existing `if (str->insensitive != NULL && str->refcnt == 0)`
gate at libwapcaplet.c:196-197 short-circuits correctly when entered
via the macro's "refcnt==1 && insensitive==self" branch (because at
that point refcnt has been decremented to 1, not 0). No upstream patch
required.

## Test ASSERT-failure leak caveat

The test suite uses `tests/shim/test_framework.h`'s `ASSERT_*` macros
which `return` from the enclosing test function on failure. Allocations
made before a failing assertion are not subsequently unref'd in that
test. This is acceptable for unit-test behaviour (vamos host process
exit reclaims the memory) but readers should not interpret the lack of
a final `lwc_string_unref()` on a particular code path as a real-world
leak.

## Consumers

- `ports/netsurf/` (Phase 1) -- the primary consumer
- `lib/libcss/`, `lib/libdom/`, `lib/libhubbub/`, `lib/libsvgtiny/`,
  `lib/libnsutils/`, `lib/libnspsl/` -- sibling NetSurf dep libs in
  Phase D-prime queue (not yet shipped as of 2026-05-02)
