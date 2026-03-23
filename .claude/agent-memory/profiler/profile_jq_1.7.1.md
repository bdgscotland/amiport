---
name: jq 1.7.1 runtime profile
description: Empirical vamos timing profile for jq 1.7.1 AmigaOS port — startup cost, hotspots, binary size analysis
type: project
---

# jq 1.7.1 Runtime Profile (vamos, 2026-03-23)

## Timing Data (wall seconds, vamos emulated ~10MHz)

| Workload | Input size | Time | Notes |
|----------|-----------|------|-------|
| `--version` (no startup init) | — | 0.12s | vamos+binary load only |
| `--null-input 'null'` | — | 1.26s | pure startup + builtin compile, zero filter work |
| `.name` | 17 bytes | 1.26s | simple field access |
| `.[] \| . * 2` | 22 bytes | 1.29s | array iteration, 10 elements |
| `keys` | 32 bytes | 1.26s | object keys, 5 keys |
| `.[].id` | 64KB (1000 obj) | 2.15s | 1000-element extraction |
| `select(.active) \| .name` | 64KB (1000 obj) | 2.34s | filter+select, 1000 elements |
| `.[].id` | 335KB (5000 obj) | 6.15s | 5000-element extraction |

## Key Finding: Startup Dominates Small Inputs

- `--version` exits before builtin compilation: **0.12s**
- `--null-input 'null'` (builtins compiled, no input): **1.26s**
- This means **builtin compilation costs ~1.14s** per invocation
- Simple field access on 17-byte input: **1.26s** — indistinguishable from null-input
- Startup (builtin compile) is **~87% of total runtime** for tiny inputs

### Startup breakdown
- vamos binary load: ~0.12s
- jq_init + builtins_bind (parse+compile 112 jq defs from 12KB builtin.inc): ~1.14s
- Actual filter execution on small input: <0.02s (unmeasurable)

## Binary Size Analysis (340,632 bytes text segment)

### Top hotspot functions by code size:

| Size (bytes) | % text | Function | Notes |
|-------------|--------|----------|-------|
| 41,724 | 12.2% | `_binop_greatereq` | ANOMALY — see below |
| 18,828 | 5.5% | `_yylex` | flex scanner for jq filter language |
| 17,818 | 5.2% | `_yyparse` | bison parser for jq filter language |
| 15,678 | 4.6% | `_jv_load_file` | JSON file loader (fread loop) |
| 12,506 | 3.7% | `_jq_next` | CORE: bytecode evaluator main loop |
| 10,502 | 3.1% | `_jq_yyfree` | flex memory free (generated) |
| 10,232 | 3.0% | `_main` | argument parsing |
| 9,112 | 2.7% | `_binop_plus` | arithmetic dispatch |
| 8,760 | 2.6% | `_jq_yylex` | jq module lexer |
| 8,026 | 2.4% | `_jq_testsuite` | test suite (dead code in production) |
| 7,342 | 2.2% | `_gen_cbinding` | startup-only: registers C builtins |
| 6,900 | 2.0% | `_jvp_dtoa_context_free` | dtoa cleanup |
| 6,772 | 2.0% | `___vfprintf_total_size` | libnix printf (pulled in) |
| 6,714 | 2.0% | `_jvp_strtod` | string-to-double conversion |

### The `_binop_greatereq` Anomaly

`binop_greatereq` is a 2-line source function that calls `order_cmp(a, b, CMP_OP_GREATEREQ)`.
At -O0, it generates **41,724 bytes** of code — 12.2% of the entire text segment.

This is NOT inlining (O0 forbids it). The gap between symbol `_binop_greatereq` (0x8414)
and the next exported symbol `_builtins_bind` (0x12710) contains ~96 static functions
from builtin.c, including:
- All math wrapper stubs from libm.h (59 LIBM_DD/DDD/DDDD instantiations)
- All jq builtin C implementations (f_contains, f_length, f_sort, etc.)
- order_cmp and its friends

nm only shows the FIRST exported symbol in that range. The "41KB for binop_greatereq"
is a nm reporting artifact — it's actually all the static functions in builtin.c
that follow the last exported symbol before the gap.

**Optimization target:** These 96 static functions total ~41KB of code. At -O0 each
function call has full stack frame overhead. On 7MHz 68000 this adds up during
the builtins_bind() startup walk.

## Optimization Priorities for A1200 (68020, 14MHz)

### Priority 1: Startup / Builtin Compilation (~87% cost for small inputs)

The `builtins_bind()` call parses and compiles 12KB of jq-language builtin definitions
(112 function defs) on EVERY invocation. On 7MHz 68000, this is the entire runtime budget
for small filters.

**Recommendation:** This is an architectural problem — jq has no precompiled bytecode cache.
Short of caching compiled builtins to disk, the only relief is `-O1` if/when the GCC
codegen bug (struct return corruption) is fixed, or targeting `-mcpu=68020` explicitly.

**Do NOT try to instrument builtins_bind** with AMIPORT_PROFILE — it calls jq_parse_library
which calls the full yyparse/yylex chain. Profile the outer call in main() instead.

### Priority 2: `_jq_next` (bytecode evaluator, 12,506 bytes)

This is the inner loop for filter execution. Called once per output value.
For 1000-element arrays it runs 1000+ times. The 0.90s delta from null-input
to 1k-element extraction is where this function lives.

**Recommendation for perf-optimizer:** Look at the opcode dispatch loop in execute.c:342.
At -O0, every opcode fetch is `uint16_t opcode = *pc` with full register save/restore.
The switch statement will compile as a jump table on 68k but each case body has
redundant stack pushes. Consider `register` hints for `pc` and `jq` pointers.

### Priority 3: `_jv_load_file` (file reader, 15,678 bytes)

Called for every input file. The 4096-byte fread buffer is already static (crash-patterns #10).
The fread loop is the only I/O path — no character-at-a-time reads.

**Already optimized:** Static buffer avoids stack allocation. fread-based not fgetc-based.
No further gains here.

### Priority 4: `_jvp_strtod` + `_jvp_dtoa_context_free` (6,714 + 6,900 bytes)

These handle JSON number parsing/printing. Called for every numeric value.
The dtoa implementation (from David Gay) is already well-optimized — no gains expected.

### Priority 5: Dead Code — `_jq_testsuite` (8,026 bytes)

This function exists in the binary and is never called in production.
It implements jq's self-test suite. It wastes 8KB of instruction cache.

**Recommendation:** `#ifdef AMIPORT_STRIP_TESTSUITE` guard around `jq_testsuite()` in
jq_test.c. Since jq_test.c is already excluded from SOURCES, this dead code is
coming in via the linker finding it in another object. Check if it can be excluded
from compile entirely, or use `--gc-sections` if supported by bebbo-ld.

## Scaling Behavior

| Multiplier | Input elements | Time | Marginal cost/1000 elem |
|-----------|---------------|------|-------------------------|
| baseline | 0 (null-input) | 1.26s | — |
| 1000 | 1,000 | 2.15s | 0.89s/1000 = 0.89ms each |
| 5000 | 5,000 | 6.15s | ~1.0s/1000 = 1.0ms each |

The scaling is slightly superlinear (1.0ms/1000 at 5k vs 0.89ms/1000 at 1k).
This is consistent with jv's reference-counted heap growing with array accumulation —
larger arrays need deeper `jv_copy`/`jv_free` reference count walks.

## Vamos Notes

- vamos reports emulated clock — timing ratios are valid, absolute values are not
- Scale factor for real A1200 (14MHz 68020): approximately 6-10x slower than vamos
- At 14MHz: small filter ~7-12s, 1000-element ~12-21s, 5000-element ~37-60s
- jq on real A1200 is practical for config file processing but not stream processing

**Why:** vamos is an interpreter running at ~10MHz equivalent. A1200 is 14MHz 68020
with CHIP RAM penalty and no cache (OCS chip bus contention). The -O0 binary with
full frame overhead hits this especially hard on the recursive jv_cmp path.
