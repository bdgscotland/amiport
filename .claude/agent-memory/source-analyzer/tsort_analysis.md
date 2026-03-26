---
name: tsort_analysis
description: OpenBSD tsort v1.38 portability analysis: MODERATE verdict, ohash library vendor required (biggest blocker), fgetln->fgets transformation, __dead macro, pledge stub, exit code fixes, offsetof in ohash_info struct (alignment safe)
type: project
---

# tsort v1.38 Portability Analysis Summary

**Verdict:** MODERATE
**Category:** 1 (CLI tool)
**Key blocker:** ohash library — BSD-specific open hash implementation not in libnix or any shim. Must vendor ohash.c + ohash.h from OpenBSD source.

## Issues

### Tier 3 — Vendor Required (build blocker)
- `#include <ohash.h>` — Not in libnix, not in amiport shims. Must vendor ohash.c+ohash.h from OpenBSD src/usr.bin/tsort or libc.
- Functions used: ohash_init, ohash_create_entry, ohash_qlookupi, ohash_find, ohash_insert, ohash_first, ohash_next, ohash_entries, ohash_delete

### Tier 1 — Shim/Stub
- pledge() x2 — stub as macro
- fgetln() x2 (read_pairs, read_hints) — replace with fgets() + sentinel logic rewrite
- err/errx/warnx — use amiport/err.h
- getopt() — use amiport_getopt() (short options only, which is all tsort uses)
- __dead macro — strip (GCC noreturn attribute)
- __progname — extern decl present, amiport_expand_argv() provides it

### Exit codes
- exit(1) in usage() -> exit(10)
- errx(1, ...) -> errx(10, ...)
- err(1, ...) -> err(10, ...)
- tsort() return value: 0 or broken_cycles count (used for -w flag) -- this is semantically meaningful, not an error code, so leave the return value logic alone. The caller in main() returns it directly. Amiga shells use IF WARN (>=5), so broken_cycles of 1+ will trigger IF WARN correctly.

### offsetof in ohash_info (line 155)
- offsetof(struct node, k) -- used to tell ohash where the key field is within the node struct
- struct node layout: refs(4) + order(4) + *arcs(4) + *from(4) + *traverse(4) + mark(4) + k[1]
- offsetof on 68k returns 2-byte aligned, so k is at offset 24 (word-aligned). This is a data field offset, NOT used for allocator block alignment.
- This is safe: ohash uses this value to find the key string within a node, not to compute block alignment boundaries. The crash-patterns #15 concern applies to custom allocators that use offsetof to pack adjacent memory blocks -- not to finding a field within a single struct.

### fgetln transformation (non-trivial)
fgetln returns a non-NUL-terminated buffer of exactly `size` bytes. The parse loops use str/sentinel pointer arithmetic within that buffer. Replacement with fgets requires:
1. Static buffer of adequate size (MAXNAMLEN or similar)
2. fgets NUL-terminates, so sentinel = str + strlen(str) instead of str + size
3. fgets stops at \n but includes it; need to strip trailing \n
4. The inner parsing loop is already written to handle multiple tokens per line, so the change is only in the outer while() condition
- Simplest fix: static char linebuf[4096]; fgets(linebuf, sizeof(linebuf), f); str=linebuf; size=strlen(str);

### C99
- stdint.h included but only used for UINT_MAX (from limits.h) -- actually no uint*_t types used, stdint.h can be dropped
- size_t, ssize_t used in fgetln signature (size_t size) -- no %zu format strings visible, clean

### Stack safety
- All graph traversal is iterative (explicit stack via node->from/traverse pointers), NOT recursive
- traverse_node(), find_cycle_from() both use iterative loops
- Local variables in all functions are small (a few pointers + unsigned ints)
- Stack: 16384 bytes should be sufficient

### Memory cleanup
- Graph nodes intentionally not freed (comment in source: "We can't free nodes")
- heap->t and remaining->t arrays allocated but not freed before exit
- For a CLI tool that runs once and exits, libnix reclaims memory on exit -- acceptable for Amiga
- However: add atexit cleanup for the two array->t allocations to be clean

## Files needed from OpenBSD
- ohash.c and ohash.h from OpenBSD src/lib/libc/hash/ (or usr.bin/tsort/ if bundled there)
- Check: https://github.com/openbsd/src/blob/master/lib/libc/hash/ohash.c

## Recommended approach
1. Vendor ohash.c + ohash.h into ports/tsort/ported/
2. Replace fgetln with fgets in read_pairs() and read_hints()
3. Stub pledge as macro
4. Fix exit codes
5. Strip __dead -> __attribute__((noreturn)) or just remove
6. Add amiport_expand_argv, __stack cookie, $VER string
