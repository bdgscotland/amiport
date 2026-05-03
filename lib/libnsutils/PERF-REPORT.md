# lib/libnsutils perf-optimizer audit (2026-05-02)

**Verdict:** KEEP -O1 WHOLE-ARCHIVE. No changes required.

**Library:** netsurf-browser/libnsutils @ commit `0bd3906`. Built `-O1
-fno-strict-aliasing -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE
-std=c99` whole-archive. 3 KB total archive.

## crash-patterns #16 audit (struct-by-value returns >8 bytes)

All three TUs cleared:

| TU | Returns | Notes |
|---|---|---|
| `time.c` | `nsuerror` (4-byte enum) | None > 8 bytes |
| `unistd.c` | `ssize_t` (4 bytes on 68k) | None > 8 bytes |
| `base64.c` | `nsuerror` (8 functions, all 4-byte) | None > 8 bytes |

## Soft-float pulls

Zero. `m68k-amigaos-nm libnsutils.a | grep -E '__(div|mul|add|sub|flo|fix)(sf|df)3'`
returns empty. The library is integer-only:

- `time.c` uses 64-bit integer arithmetic (`uint64_t * 1000`,
  `eclock / freq` via `___udivdi3` libgcc helper -- INTEGER, not float)
- `unistd.c` is just lseek + read/write
- `base64.c` is bit-shifts + 256-byte table lookups

No mathieeesingbas.library crash risk (crash-patterns #2 variant).

## Hot path / per-call cost

- **`nsu_getmonotonic_ms`**: ReadEClock + 64-bit divide + clamp ~150-200
  cycles. Called once per render-tick in NetSurf -- < 0.001% of frame
  budget.
- **`nsu_pwrite/pread`**: I/O-bound thin wrappers. ~100 cycles + I/O latency.
- **`base64.c`** core loop: ~10-12 cycles/byte for both encode and
  decode. 4 KB encode = ~41K cycles = ~1.6 ms on 68040 @ 25 MHz.
  NetSurf's typical use: < 1 KB payloads (HTTP Basic Auth, data URIs).

## Stack analysis

Max per-call footprint ~32 bytes across all functions. Combined with
AmigaOS's 2-4 KB hidden DOS depth, safe at 256 KB cookies. All 22
tests pass on the standalone-lib `-s 1024` (1 MB) vamos invocation.

## Code size

Archive 3 KB total: base64.o ~1.7 KB (4× 256-byte lookup tables +
8 small functions), time.o 344 bytes, unistd.o 296 bytes. No bloat.

## Per-file -O2 promotion considered

base64.c is the only non-trivial hot path. -O2 would yield ~8-10% on
encode/decode (table lookup hoisting, unroll). At NetSurf's typical
< 1 KB workload: ~0.03 ms saved. Not worth the safety risk.

## Recommendation

The current Makefile is already optimal:

```
CFLAGS = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -Iinclude -Isrc
```

No changes required.
