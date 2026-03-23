---
name: port_uniq_perf
description: Performance findings for ports/uniq 1.33 — hotpath summary and recommendations reviewed 2026-03-22
type: project
---

Key hot paths in ports/uniq (OpenBSD uniq 1.33):

1. Main loop (lines 179-204) — getline() + skip() + strcmp/strcasecmp per line. The getline()
   shim already uses fgets() internally (fast path confirmed in stdio_ext.c). Primary cost is
   the function-call chain per line. No fgetc() in the hot path. I/O is well-handled.

2. `long long numchars, numfields` and `unsigned long long repeats` (line 74-75) — 64-bit globals.
   On 68000/68EC020, each 64-bit increment (++repeats) compiles to addq.l + addx.l (2 insns).
   Each 64-bit compare (nfields && ..., nchars-- && ...) costs 2 compares instead of 1. These
   are in the inner loops of skip(). When -f or -s flags are in use, this matters. Range is
   bounded by line count (never exceeds LONG_MAX in practice) — safe to use long/int.

3. skip() character loop (lines 253-273) — iterates str++ with len=1 at each step. The `str += len`
   pattern with len always being 1 compiles to addq.l #1,An — which is fine. The isblank() call
   inside the field-skip loop IS a library call (JSR overhead ~20 cycles per call on 68000).
   When -f is used on files with long blank-prefixed fields, this accumulates. Replace with
   inline test: `(uc == ' ' || uc == '\t')`.

4. `(iflag ? strcasecmp : strcmp)(p, t)` (line 190) — conditional function pointer call. On
   68000, this is a JSR (Ra) indirect call — 16 cycles vs a direct JSR (8 cycles + displacement
   fetch). For most files the branch predictor doesn't exist on 68000 — this is a minor but
   real cost per line. Pre-computing a function pointer outside the loop eliminates the ternary
   overhead per iteration.

5. numchars/numfields zero-check (line 172, 184) — OR of two 64-bit values to check "either
   nonzero" compiles to 4 instructions on 68000 (move.l + move.l + or.l + or.l). A boolean
   flag set once at startup is a single tst.b — saves ~12 cycles per line, ~1% on fast input.

6. show() function (line 222-230) — called every time a run ends (i.e., once per unique run).
   Uses printf() for both -c and non-c paths. The non-c path (printf("%s\n", str)) is fine
   for output-bound work. The -c path (printf("%4llu ...", repeats + 1, str)) uses %llu which
   on libnix requires 64-bit argument handling. Minor — show() is not the hot path.

**Primary bottleneck:** For the default case (no -f/-s), this is almost entirely I/O bound.
getline() → fgets() → libnix fread → AmigaDOS Read() with a 512-byte sector buffer. On a
typical file, each line costs: fgets buffered read (cheap) + strlen (cheap) + strcmp (O(n)
per line). No major CPU waste in the default path.

**When -f or -s are used:** skip() becomes the hot path. isblank() as a library call per
character is the same trap as isspace() in wc — ~20 cycles overhead per character per call.
On a file with 10-field lines of average 60 chars, that's ~600 isblank() calls per line.

**Recommendations summary:**
- HIGH: Downgrade numchars/numfields/repeats from 64-bit to long/ULONG — saves 1 instruction
  per 64-bit op in skip() inner loop and in the main loop repeats increment.
- MEDIUM: Inline isblank() in skip() as `(uc == ' ' || uc == '\t')` — eliminates JSR per char
  when -f is active.
- MEDIUM: Hoist `iflag ? strcasecmp : strcmp` to a function pointer set once before the loop.
- LOW: Replace `numfields || numchars` (64-bit OR) with a single boolean flag `use_skip`.
- LOW: `repeats` counter only needs to distinguish 0 from nonzero for -d/-u; for -c it's
  bounded by line count (max ~2^31 lines in practice). long is safe.
- NOT WORTH IT: getline I/O path is already optimised (fgets fast path in shim). Do not change.
- NOT WORTH IT: show() — output bound, not CPU bound.

**Why:** The 64-bit downgrade and isblank() inline are the only changes with measurable impact
on real 7MHz 68000 hardware. The rest are micro-optimizations with <1% impact.

**How to apply:** When advising on uniq performance, lead with the 64-bit→32-bit type downgrade
for numchars/numfields/repeats. The isblank() inline is a secondary win when -f is active.
