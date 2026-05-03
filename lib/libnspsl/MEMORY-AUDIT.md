# lib/libnspsl memory-checker audit (2026-05-02)

**Verdict:** APPROVED. No findings.

**Library:** netsurf-browser/libnspsl @ commit `82815c2`. 1 hand-
written TU + 1 generated table file (`psl.inc`, ~13K LOC `static const`).

## Allocation inventory

ZERO dynamic allocations across the entire library. The function
returns a pointer INTO the caller's input string. No malloc / calloc /
realloc / strdup anywhere.

## Findings

**None.** The library is alloc-free, re-entrant, and uses only
`static const` data. Maximally freestanding (only `<stdint.h>` +
`<string.h>`).

## Practical risk

**Consumer integration risk: zero.**

- No cleanup discipline required.
- No reference counting.
- No init / finalise.
- The returned pointer points into the caller's input string -- the
  caller owns the lifetime, has nothing to free.

## Recommendation

`libnspsl.a` is safe to link from `ports/netsurf` and any downstream
consumer. No documentation required beyond the public header.
