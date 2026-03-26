---
name: port_jq_perf
description: Performance findings for ports/jq 1.7.1 — put_char fwrite overhead, MurmurHash3 unaligned read, jvp_string_vfmt heap thrash, builtin startup cost, Oniguruma regex caching
type: project
---

# jq 1.7.1 — Performance Review

Reviewed: 2026-03-23 (initial), 2026-03-25 (Oniguruma regex support added)

## Critical constraint
Must compile with -O0 (crash-patterns #16: bebbo-gcc -O2 corrupts 16-byte jv struct returns).
All optimizations are source-level only.
lib/oniguruma/ also uses -O0 (Makefile comment: "-O2 causes regex failures on real AmigaOS").

## Applied optimizations (confirmed in code)
1. 4KB static output buffer (amiport_print_buf) in jv_print.c — all put_buf calls go through it
2. Bulk put_indent using static spaces/tabs strings
3. 256-byte stack probe in jv_string_vfmt — avoids malloc for short strings
4. MurmurHash3 uses memcpy(&k1, ..., 4) for alignment-safe 32-bit reads

## Oniguruma analysis (2026-03-25)

### match_at is NOT recursive
Oniguruma's match_at() is an iterative NFA interpreter, not a recursive backtracker.
It uses an explicit heap-allocated backtrack stack (STACK_INIT macro), starting at 160
entries (INIT_MATCH_STACK_SIZE=160, each StackType ~24 bytes = ~3.8KB initial heap alloc).
Stack grows via xmalloc/xmemcpy if needed. Zero risk of C stack exhaustion from backtracking.

### __stack=131072 is sufficient
match_at() has a large local variable frame (~20 pointer-sized locals, plus the opcode jump
table at ~200 void* entries = ~800 bytes on 68k). With INIT_MATCH_STACK_SIZE on the heap,
the C stack cost of one onig_search() call is approximately 1-2KB. 131072 bytes (128KB) is
ample. No increase needed for Oniguruma.

### onig_new() called on every f_match invocation — HIGH impact
builtin.c:928: onig_new() compiles a fresh regex_t on every call to f_match/test/match/capture.
onig_free() is called at line 1051 at end of function. No caching.

For filters like `.[] | test("pattern")` on an array, onig_new() runs once per array element.
onig_new() does NFA compilation (regcomp.c), NFA optimization, and heap allocation. On 68000
at 7MHz with -O0, this is expensive: hundreds of cycles for even a simple literal pattern.

Fix: Add a static regex cache keyed on (pattern, encoding, options). Simplest implementation:
a single cached entry -- most jq usage applies one pattern repeatedly. Check if
(pattern string, options) matches previous call; if so, reuse the compiled regex_t.

Cache lifetime: the jq_state lifecycle. Free on jq_teardown or at program exit via atexit.
This is safe because jq is single-threaded on AmigaOS (no pthreads).

Single-entry cache estimate: eliminates ~95% of onig_new() calls for common usage patterns.

### jv_copy(input) inside inner loop — MEDIUM
builtin.c:929: jv_string_length_bytes(jv_copy(regex)) -- jv_copy increments a refcount, and
jv_string_length_bytes then reads a field. The jv_copy is unnecessary here: regex is not
consumed by jv_string_length_bytes. Can use jv_string_length_bytes(regex) with a direct
local cache of the length value. Same at line 943 for input length.

### onig_region_new() heap allocation per search -- LOW-MEDIUM
builtin.c:945: onig_region_new() mallocs a new region per f_match call, freed at line 1049.
For global=0 (non-'g' mode, the common case), only one region is needed per call.
Fix: use onig_region_init() on a stack-allocated region for non-global matches. Avoids
one malloc/free pair per regex operation.
Note: only valid when num_regs is small (<=ONIG_NREGION). For simple patterns this is safe.

### Oniguruma -O0 subset analysis
Files that can safely use -O2 (no struct-by-value returns > 8 bytes):
- regversion.c: trivial version string accessors -- safe
- regerror.c: error string formatting -- safe
- regsyntax.c: syntax table initialization -- safe (called once)
- regtrav.c: node tree traversal utilities -- safe
- ascii.c: encoding table lookups -- safe, pure table ops
- unicode_stubs.c: stub functions returning int -- safe

Files that must stay at -O0 (uncertain/complex struct returns or matching known -O2 failure):
- regexec.c: match_at() -- this is the known -O2 failure. MUST stay -O0.
- regcomp.c: NFA compilation -- complex, uses MinMaxLen/MinMaxCharLen (by pointer, not value)
  but safe in principle. However, the original -O2 failure was not isolated to regexec.c,
  so keep at -O0 until isolated.
- regparse.c: 3 compound literal returns found (return {expr}) -- MUST stay -O0.
- st.c: hash table -- generally safe but not a hotpath, leave at -O0.
- regenc.c: encoding dispatch -- safe, trivial functions.
- utf8.c: UTF-8 decode tables -- safe, pure table ops.

Practical recommendation: the hotpath files are regexec.c and regcomp.c. Both are uncertain
or known-bad. The safe files (version, error, syntax, trav, ascii, stubs) are not hotpaths.
The -O2 gain from those files is negligible. Not worth the per-file build complexity.
Conclusion: keep whole library at -O0.

## flush coverage analysis (from 2026-03-23 review)
- jv_dumpf() flushes at end -- good for normal JSON output
- GAP: priv_fwrite("\n", 1, stdout, ...) at main.c:276 -- one raw fwrite per output value

## Remaining opportunities

### 1. Regex compile cache in f_match (HIGH)
builtin.c:928: onig_new() on every invocation. Add a single-entry static cache.
Est: eliminates regex compilation cost for repeated pattern matching (the common case).
This is the dominant new cost added by Oniguruma support.

### 2. jv_copy() overhead in f_match (MEDIUM)
builtin.c:929,943: unnecessary refcount bumps for jv_string_length_bytes calls.
Cache the string lengths in locals before the match loop.

### 3. onig_region stack allocation for non-global matches (LOW-MEDIUM)
builtin.c:945: heap malloc per search for non-'g' mode matches.
Use stack-allocated region + onig_region_init() for the common case.

### 4. Trailing newline is a naked fwrite (LOW-MEDIUM)
main.c:276: priv_fwrite("\n", 1, stdout, ...) executes once per output value.
Fix: funnel through the Amiga output buffer (amiport_put_newline_and_flush()).

### 5. Builtin startup: still dominates at 87% (no new mitigation)
No source-level fix applied. Options: strip rarely-used builtins under #ifndef __AMIGA__.

## What NOT to optimize
- The VM dispatch loop in execute.c -- jq_next() is already a tight switch
- The dtoa context (jvp_dtoa) -- already maintained per-parser-instance
- The slab stack in exec_stack.h -- alignment fix already applied
- MurmurHash3 inner loop -- memcpy approach is correct and already applied
- Oniguruma -O2 upgrade -- not worth the build complexity for cold-path files
- match_at() stack depth -- not recursive; uses heap stack; no issue
