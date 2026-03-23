---
name: port_jq_perf
description: Performance findings for ports/jq 1.7.1 — put_char fwrite overhead, MurmurHash3 unaligned read, jvp_string_vfmt heap thrash, builtin startup cost
type: project
---

# jq 1.7.1 — Performance Review

Reviewed: 2026-03-23

## Critical constraint
Must compile with -O0 (crash-patterns #16: bebbo-gcc -O2 corrupts 16-byte jv struct returns).
All optimizations are source-level only.

## Primary bottlenecks

### 1. put_char: fwrite(1 byte) per character in output (HIGH)
jv_print.c put_char() calls put_buf() which calls fwrite(&c, 1, 1, fout).
Every character of JSON output — quotes, braces, commas, spaces — is a separate
fwrite call. fwrite on libnix/AmigaOS has significant per-call overhead.
For a 1KB JSON output with pretty-printing this is hundreds of fwrite calls.
Fix: accumulate output into a static or stack buffer (~4KB), flush on newline or buffer full.

### 2. MurmurHash3 unaligned 32-bit reads on 68k (HIGH)
jv.c jvp_string_hash() line 1184 does:
  const uint32_t* blocks = (const uint32_t*)(data + nblocks*4);
  for (int i = -nblocks; i; i++) { uint32_t k1 = blocks[i]; ... }
The source code has a FIXME comment: "//FIXME: endianness/alignment"
jvp_string data is heap-allocated via malloc — not guaranteed word-aligned on all
AmigaOS allocators. An unaligned 32-bit MOVE.L on 68000 causes an address error
(bus error). On 68020+ unaligned reads are legal but slower (multiple bus cycles).
Fix: use byte-at-a-time reads assembled into uint32_t for the block loop.

### 3. jv_string_vfmt heap-allocates 1024 bytes for every format string (MEDIUM)
jv.c line 1452: always starts with malloc(1024). Every jv_string_fmt() call
(type errors, field access, format strings) allocates + frees 1KB.
On AmigaOS malloc has significant overhead. In error paths this is acceptable,
but jv_string_fmt is called for non-error cases too.
Fix: use a static probe buffer of 256 bytes — try vsnprintf into it first, only
heap-allocate if it overflows. Most format strings are short.

### 4. Builtin startup: 12KB jq source parsed and compiled at launch (MEDIUM)
builtin.c line 1904: builtins_bind() parses the 12KB jq_builtins[] string through
the full jq lexer/parser/compiler every time jq starts. On 68020 @ 14MHz this
is the dominant startup cost (the reported 1-2 second delay).
The compiler processes ~200+ function definitions through the bytecode pipeline.
There is no pre-compiled bytecode or caching mechanism.
Fix options:
  a) Pre-compile builtins to a binary bytecode array at build time (hard, requires
     tooling change)
  b) Reduce the builtin set by commenting out rarely-used functions for the Amiga
     build under #ifndef __AMIGA__ guards (easy, reduces parsing work)
  c) Cache compiled bytecode to T:jq-builtins.cache and load it on second run
     (medium, requires serialization support which jq doesn't have)
Option b is the most practical quick win.

### 5. put_str uses strlen() on every string literal (LOW-MEDIUM)
jv_print.c put_str() calls strlen(s) on every string literal ("null", "true",
"false", "{}", "[]", ",", "[", etc). These are constant-length strings.
Fix: replace put_str("null", ...) with put_buf("null", 4, ...) etc. Eliminates
strlen call for all string literal output.

### 6. put_indent: put_char per space/tab in pretty-print mode (LOW)
jv_print.c put_indent() calls put_char() once per indent space/tab.
For deeply nested structures at indent level 4 with 2-space indent = 8 fwrite calls.
Fix: use put_buf() with a static spaces/tabs buffer.

## What NOT to optimize
- The VM dispatch loop in execute.c — jq_next() is already a tight switch, and
  without -O2 there's little to gain from source changes there
- The dtoa context (jvp_dtoa) — it's already maintained per-parser-instance
- The slab stack in exec_stack.h — alignment fix already applied (4-byte minimum)
