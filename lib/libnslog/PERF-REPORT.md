# lib/libnslog perf-optimizer audit (2026-05-02)

**Verdict:** KEEP -O1 WHOLE-ARCHIVE. No changes required.

**Library:** netsurf-browser/libnslog v0.1.3 @ commit `bedff21`. Built
`-O1 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE
-std=c99` whole-archive. 21 KB total archive (15 KB .text across 4
objects).

## crash-patterns #16 audit (struct-by-value returns >8 bytes)

All four TUs cleared:

| TU | Returns | Notes |
|---|---|---|
| `core.c` | `void`, `const char*`, `nslog_error` (4-byte enum) | None > 8 bytes |
| `filter.c` | `nslog_error`, `nslog_filter_t*`, `bool`, `char*` | `nslog_filter_s` always returned by pointer |
| `filter-lexer.c` | `int` tokens | Scalar |
| `filter-parser.c` | `int` parse result | Scalar |

`nslog_error` enum is 4 bytes, well under the 8-byte threshold. Whole-
archive `-O1` promotion is safe.

## Soft-float pulls

`m68k-amigaos-nm libnslog.a | grep -E '__(div|mul|add|sub|flo|fix)(sf|df)3'`
returns empty. Zero soft-float references. The library is integer-only
(string ops, enum compares, refcount arithmetic). No
`mathieeesingbas.library` crash risk (crash-patterns #2 variant).

## Hot path analysis

Estimated per-call cycles on 68030 @ 25 MHz (rough; varies by filter):

- **Uncorked log (typical)**: ~500-800 cycles (~20-32 us)
  - Function prologue + arg setup
  - Filter dispatch (single-level NSLFK_LEVEL): ~50-100 cycles
  - Callback invocation: ~100 cycles
  - Caller's vsnprintf: ~200-400 cycles (dominant)
- **Corked log**: ~1500-2500 cycles (~60-100 us)
  - vsnprintf probe (1024 byte): ~300-500 cycles
  - calloc: ~400-800 cycles
  - vsnprintf write: ~300-500 cycles
- **Filter walk (5-deep)**: ~300-500 cycles, ~20-deep AND tree adds ~500 cycles

Realistic NetSurf workload: ~100 log calls per pageload. Total logging
overhead per pageload ~50-80 ms on 68030, ~25-40 ms on Vampire 68080
@ 50 MHz. Logging is NOT the bottleneck.

## Stack analysis

- `nslog__log` 1024-byte probe buffer: non-recursive call site, safe
  at 256 KB cookies (well above the AmigaOS 2-4 KB hidden DOS depth)
- `nslog__filter_matches` recursive walk: ~100 bytes per frame; 20-deep
  worst case ~2 KB
- Bison parser: `YYINITDEPTH=200`, ~1.2 KB initial; can grow via
  realloc to `YYMAXDEPTH=10000`

Combined peak stack pressure: ~4.2 KB. Comfortable margin even at
256 KB cookies. The standalone-lib test harness uses 256 KB and passes
all 25 tests including the `stress_deeply_nested_and_tree` test.

## Code size

21 KB archive is at the floor for the feature set. Bison/flex output
dominates (filter-lexer.o: 6.8 KB .text, filter-parser.o: 5.1 KB).
Hand-written code is minimal (filter.o: 2.2 KB, core.o: 1 KB). No
size-reduction opportunities worth pursuing.

## Per-file -O2 promotion considered

`_nslog__filter_matches` is the only non-trivial hot function. -O2
would yield ~10-20% on filter dispatch via inline strncmp/strcmp. But:

1. Logging is < 1% of NetSurf frame time per the openttd-sdl2 perf data.
2. Safety margin from -O1 (avoids any future bebbo-gcc codegen quirks)
   outweighs the marginal -O2 gain.
3. Whole-archive -O1 simplifies the Makefile.

**Decision:** keep whole-archive -O1. Do not promote any file.

## Recommendation

The current Makefile is already optimal:

```
CFLAGS = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -Iinclude -Isrc
CFLAGS += -Wno-unused-function -Wno-unused-variable
```

No changes required.
