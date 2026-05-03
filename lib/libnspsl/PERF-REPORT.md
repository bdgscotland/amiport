# lib/libnspsl perf-optimizer audit (2026-05-02)

**Verdict:** KEEP -O1 WHOLE-ARCHIVE. No changes required.

**Library:** netsurf-browser/libnspsl @ commit `82815c2`. Built `-O1
-fno-strict-aliasing -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE
-std=c99` whole-archive. 67 KB total archive (~64 KB PSL data,
~3 KB code).

## crash-patterns #16 audit (struct-by-value returns >8 bytes)

Single TU, all return types scalar:

- `nspsl_getpublicsuffix`: `const char *` (4 bytes)
- `matchlabel`: `int` (4 bytes)
- `huffcasecmp`: `int` (4 bytes)
- inline helpers: `int` / `char`

Zero struct-by-value returns. -O1 codegen safe.

## Soft-float pulls

`m68k-amigaos-nm libnspsl.a | grep -E '__(div|mul|add|sub|flo|fix)(sf|df)3'`
returns empty. Zero soft-float references. The library is integer-only:
bit-shifts, table lookups, integer compares, pointer arithmetic.

## Hot path / per-call cost

Per `nspsl_getpublicsuffix` call (typical 4-label hostname):

| Path | Cycles | Notes |
|---|---|---|
| Hostname backward scan | ~250 | Pointer arith + char branch |
| `matchlabel` (4 labels, ~50 iter avg) | ~2500 | Linear scan of children at each tree level |
| `huffcasecmp` (4 labels) | ~640 | Bit-level Huffman decode + char compare |
| **Total** | **~3400 cycles** | |

Wall time:
- 68000 @ 7 MHz: ~0.49 ms per call
- 68060 @ 50 MHz: ~0.03 ms per call

NetSurf usage: ~10-100 calls per pageload. Negligible.

## Stack analysis

Local variables sum to ~54 bytes across the call chain
(nspsl_getpublicsuffix → matchlabel → huffcasecmp). No recursion. No
large local arrays. Safe at any cookie size used in the dep stack.

## Per-file -O2 promotion considered

The hot path is `matchlabel`'s linear scan (worst case 1500 root-level
TLD entries). -O2 would inline the array indexing into pointer
increments and yield ~10-15% on the worst-case match. For NetSurf's
typical pageload (~50 ms total PSL lookup overhead on 68000), -O2
would save ~5-6 ms -- invisible.

Not worth the safety risk vs the marginal gain. Keep -O1.

## Recommendation

The current Makefile is already optimal:

```
CFLAGS = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -Iinclude -Isrc
```

No changes required.
