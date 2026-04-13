---
name: port_libgit2_perf
description: Performance findings for lib/libgit2 1.8.5 — SHA1DC -O1 promotion, xdiff -O1 promotion, GIT_LEGACY_HASH unaligned-cast fix, reviewed 2026-04-13
type: project
---

# libgit2 1.8.5 Performance Review (Stage 7)

**Date:** 2026-04-13
**Status:** APPLIED

## Verdict
PROMOTE 9 FILES to -O1. Applied to `lib/libgit2/Makefile`.

## Hot Paths
1. `sha1dc/sha1.c` + `sha1dc/ubc_check.c` -- SHA-1 compression, 80-step unrolled, register-bound
2. `xdiff/xdiffi.c` -- Myers O(ND) diff inner loop (xdl_split)
3. `xdiff/xprepare.c` -- per-line hash classification for diff
4. `xdiff/xutils.c` -- xdl_hash_record per-line hashing
5. `xdiff/xhistogram.c`, `xdiff/xmerge.c`, `xdiff/xpatience.c` -- diff algorithm variants
6. `util/wildmatch.c` -- .gitignore / pathspec per-path lookup

## Key Flags Added
- `-DGIT_LEGACY_HASH` added to all CFLAGS -- selects byte-at-a-time fallback in util.c,
  avoiding the unaligned `(const uint32_t *)key` cast in MurmurHash3 that would
  Guru on real 68000 hardware (bus error, unaligned 32-bit read).
- `HOTPATH_CFLAGS` = base CFLAGS with -O0 -> -O1, plus `-fno-strict-aliasing` (required:
  xdiff chastore casts (char*)<->(long*), SHA1DC casts (uint8_t*)<->(uint32_t*)).
- `XDIFF_HOTPATH_CFLAGS` = same but inherits `XDIFF_CFLAGS` (-Isrc/xdiff include path).

## Files NOT Promoted
- `pcre_exec.c` (RISKY): 7173 lines, complex heap-based recursion simulation. Not safe to audit exhaustively.
- `hashsig.c` (LOW-VALUE): int64_t arithmetic; 68000 has no 64-bit ALU, benefit uncertain.
- `delta.c` (LOW-VALUE): integer divides in index-build loop (cold path for amigit use case).
- `util.c` (RISKY): dead MurmurHash3 has unaligned cast; mitigated by -DGIT_LEGACY_HASH.

## Why: -O0 default
Per library-pipeline rule (known-pitfalls: "Default to -O0 for bundled libraries until proven safe").
The -O1 promotion was only applied after full per-TU audit. crash-patterns #16 applies to
struct-by-value returns > 8 bytes -- none found in promoted files.

## split_score = 8 bytes
xdiffi.c declares `struct split_score { int effective_indent; int penalty; }` = exactly 8 bytes.
This is the threshold for crash-patterns #16 (LARGER THAN 8 bytes). GCC returns 8-byte structs
in D0:D1 register pair on 68000 -- safe. The struct is also declared as local + passed by pointer,
not returned by value from any function.
