---
name: port_jq_perf
description: Performance findings for ports/jq 1.7.1 — put_char fwrite overhead, MurmurHash3 unaligned read, jvp_string_vfmt heap thrash, builtin startup cost
type: project
---

# jq 1.7.1 — Performance Review

Reviewed: 2026-03-23 (follow-up after optimizations applied)

## Critical constraint
Must compile with -O0 (crash-patterns #16: bebbo-gcc -O2 corrupts 16-byte jv struct returns).
All optimizations are source-level only.

## Applied optimizations (confirmed in code)
1. 4KB static output buffer (amiport_print_buf) in jv_print.c — all put_buf calls go through it
2. Bulk put_indent using static spaces/tabs strings
3. 256-byte stack probe in jv_string_vfmt — avoids malloc for short strings
4. MurmurHash3 uses memcpy(&k1, ..., 4) for alignment-safe 32-bit reads

## flush coverage analysis (follow-up)
- jv_dumpf() flushes at end — good for normal JSON output (jv_dump calls this)
- jv_show() flushes via fflush(stderr) directly — stderr path is unflushed through buffer; this is safe because jv_show writes to stderr and jv_show calls jv_dumpf which flushes at end
- debug_cb() at main.c:327 calls jv_dumpf() to stderr then fprintf(stderr,"\n") — the fprintf bypasses the Amiga buffer (goes straight to libnix/fwrite). The \n lands after the buffer flush, so no interleaving issue.
- GAP: priv_fwrite("\n", 1, stdout, ...) at main.c:276 — the trailing newline after each jv_dump result lands via raw fwrite AFTER amiport_flush_print_buf() has been called by jv_dumpf. So it does hit a separate raw fwrite call. On AmigaOS, a single-byte fwrite costs nearly as much as a bulk fwrite. This is the one remaining per-result raw I/O call.
- priv_fwrite is not patched through the Amiga buffer — it goes direct to fwrite. Calls at lines 261, 272, 276, 278, 295 are all short.

## Remaining opportunities (NEW findings after optimization pass)

### 1. Trailing newline is a naked fwrite (LOW-MEDIUM)
main.c:276: priv_fwrite("\n", 1, stdout, ...) executes once per output value, after jv_dumpf() has already flushed the Amiga buffer. This is a separate 1-byte fwrite call for every output line.
Fix: In the __AMIGA__ path, funnel the trailing newline through put_buf instead of priv_fwrite, or append it directly to the Amiga buffer before flushing. Requires exporting a small amiport-specific function from jv_print.c (e.g. amiport_put_newline_and_flush()).
Impact: Saves one fwrite per output value. For a filter producing 1000 lines this is 1000 unnecessary system-level calls.

### 2. jvp_dump_string: put_char for escape sequences could be put_buf (LOW)
jv_print.c lines 179-221: escape sequences for \b \t \r \n \f \\ " are emitted as two separate put_char calls each ('\' then the letter). Each put_char is now cheap (copies 1 byte into the 4KB buffer, no fwrite). At -O0 without inlining, two put_char calls = two JSR+RTS round-trips vs. one.
Fix: Replace adjacent put_char pairs with put_buf("\\n", 2, ...) etc. Six instances. Minor but trivially correct.
Impact: Saves one JSR/RTS per escaped character in strings. Low unless handling many backslashes.

### 3. put_str uses strlen() — still present for dynamic strings (LOW, already partially mitigated)
jv_print.c:134: put_str() calls strlen(s). This is fine for string literals (compiler may optimize) but is called with dynamic buffers (dtoa output, color escape sequences). The color path calls put_str(color,...) repeatedly — color strings are fixed (e.g. "\033[0;32m", 7 bytes). These won't be constant-folded at -O0.
Fix: Replace put_str calls for known constant strings with put_buf("...", N, ...) directly at call sites in jv_dump_term. About 10 call sites in the color path.
Impact: Eliminates strlen for every color escape emitted. Low unless --color-output is used.

### 4. Builtin startup: still dominates at 87% (confirmed, no new mitigation)
No source-level fix applied. Options remain: (a) strip rarely-used builtins under #ifndef __AMIGA__, (b) no feasible bytecode cache without serialization work.

## What NOT to optimize
- The VM dispatch loop in execute.c — jq_next() is already a tight switch, and without -O2 there's little to gain from source changes there
- The dtoa context (jvp_dtoa) — already maintained per-parser-instance
- The slab stack in exec_stack.h — alignment fix already applied (4-byte minimum)
- MurmurHash3 inner loop — memcpy approach is correct and already applied
