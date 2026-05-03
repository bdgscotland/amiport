# lib/libnsutils memory-checker audit (2026-05-02)

**Verdict:** APPROVED. No critical findings.

**Library:** netsurf-browser/libnsutils @ commit `0bd3906`. 3 TUs:

- `src/time.c` (~100 LOC) -- ReadEClock-based monotonic time
- `src/unistd.c` (~70 LOC, +1 amiport ftruncate-fallback removal) --
  pwrite/pread emulation
- `src/base64.c` (~430 LOC) -- RFC4648 base64 encode/decode (8 funcs)

## Allocation inventory

| Component | Allocations | Pattern | Status |
|-----------|-------------|---------|--------|
| time.c | 0 dynamic | Static `prev` (uint64_t) for monotonic clamp | Intentional process-wide |
| unistd.c | 0 dynamic | lseek + read/write only | None |
| base64.c | 2 malloc sites | `_alloc` variants malloc output buffer | Caller owns / caller frees |

`_alloc` ownership semantics are clean: function returns NULL on
malloc failure (no leak), success transfers ownership to caller.
Documented in public header.

## Critical findings

**None.** All findings are limitations / advisories already documented in
the source.

## Practical risk

**Consumer integration risk: minimal.** The `_alloc` variants follow a
consistent ownership pattern (caller frees) used throughout NetSurf.
Time/unistd are alloc-free.

`nsu_pwrite` past EOF returns -1 (or extends in vamos's POSIX-compliant
lseek case) -- documented in source. Consumer must pre-grow files
before pwrite-past-EOF if needed.

## Findings summary

| Check | Result |
|---|---|
| malloc/free balance | All 2 alloc sites have clean ownership transfer to caller |
| Realloc safety | No realloc anywhere |
| Double-free | Not possible (lib never frees -- caller owns) |
| Use-after-free | Not possible (lib doesn't retain pointers) |
| Static globals | `prev` in time.c (intentional, process-wide single-threaded) |
| Soft-float pulls | Zero (verified via `m68k-amigaos-nm`) |
| Struct-by-value returns >8 bytes | Zero |
| Stack safety | Max ~32 bytes per call |
| AmigaOS-specific | ftruncate-fallback removed (libnix has no ftruncate) -- documented |

## Recommendation

`libnsutils.a` is safe to link from `ports/netsurf` and any downstream
NetSurf-Vampire dep stack consumer. Document the `_alloc`-variant
free-on-caller pattern + the `timer.device` requirement for
`nsu_getmonotonic_ms` in consumer PORT.md.
