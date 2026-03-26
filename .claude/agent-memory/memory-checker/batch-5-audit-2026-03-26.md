---
name: Memory Safety Audit Batch 5 (2026-03-26)
description: Comprehensive memory safety review of true, false, colrm, factor, getopt ports
type: project
---

## Audit Summary

Batch 5 audit of 5 ports on 2026-03-26. All ports are trivial or well-implemented.

### Ports Audited
1. **ports/true** — trivial, zero allocations, CLEAN
2. **ports/false** — trivial, zero allocations, CLEAN
3. **ports/colrm** — complex, getline() buffer tracking, perfectly balanced, CLEAN
4. **ports/factor** — large tables (static), atexit cleanup, CLEAN
5. **ports/getopt** — argv expansion with atexit cleanup, CLEAN

### Key Findings
- All 5 ports use proper atexit() cleanup patterns for err()/errx() bypass paths
- colrm's getline() buffer tracking is exemplary: static tracking pointer + NULL guard after loop completes
- factor only has static allocations (prime tables, pattern tables) — no dynamic memory
- getopt and colrm both properly use amiport_expand_argv() with atexit(cleanup) paired

### Verdicts
- **true**: CLEAN (0 bytes allocations)
- **false**: CLEAN (0 bytes allocations)
- **colrm**: CLEAN (getline buffer properly tracked, atexit cleanup perfect)
- **factor**: CLEAN (static tables only, no dynamic allocations)
- **getopt**: CLEAN (single-owner argv expansion, atexit cleanup)

All 5 ports approved for shipping.

## Detailed Notes

### 1. true (1 line main)
- `main()` returns constant 0
- No includes of malloc/free/getline/amiport_* that allocate
- No error paths beyond a single return
- **Verdict: CLEAN**

### 2. false (1 line main)
- `main()` returns constant 10 (RETURN_ERROR for AmigaOS)
- No allocations whatsoever
- **Verdict: CLEAN**

### 3. colrm (getline buffer pattern)
- **getline_buf static tracking pointer**: initialized NULL at line 71
- **Allocation path**: `while(getline(&line, &linesz, stdin) != -1)` at line 141
- **Tracking**: `getline_buf = line;` at line 142 after each successful getline
- **Loop exit guard**: `getline_buf = NULL;` at line 237 AFTER loop completes
- **atexit cleanup**: Line 99 registers cleanup function
- **cleanup() function**: Lines 74-82
  - Frees getline_buf if non-NULL
  - Sets getline_buf to NULL after free
  - Calls amiport_free_argv() for argv expansion
  - Flushes stdout
- **Error paths**: err()/errx() at lines 106, 121, 126, 135, 239, 241 all routed through atexit cleanup via cleanup()
- **Double-free prevention**: The NULL guard at line 237 AFTER getline loop completes prevents atexit from double-freeing
- **Verdict: CLEAN** — textbook getline() cleanup pattern

### 4. factor (static tables only)
- **Dynamic allocations**: ZERO
- **Static allocations**:
  - `char table[TABSIZE]` at line 257 (stack-local in pr_bigfact, 262144 bytes on stack)
  - `char buf[100]` at line 126 (stack-local, safe)
  - `const char pattern[]` in pattern.c (read-only data section)
  - `const ubig prime[]` in pr_tbl.c (read-only data section)
- **Error paths**: err()/errx() at lines 133, 150, 159, 163, 167, 174, 178, 180
- **atexit cleanup**: Line 129 registers cleanup (only flushes stdout)
- **No malloc/free calls in source**
- **Verdict: CLEAN** — pure computation, no memory allocation

### 5. getopt (argv expansion only)
- **Dynamic allocations**: Single-owner argv expansion via amiport_expand_argv()
- **Allocation source**: Line 46 `amiport_expand_argv(&argc, &argv);`
- **atexit cleanup**: Line 48 `atexit(cleanup);`
- **cleanup() function**: Lines 31-35
  - Calls amiport_free_argv() (paired with amiport_expand_argv)
  - Flushes stdout
- **Error paths**: err() at line 51 (pledge failure) routed through atexit
- **All exit paths covered**:
  - Normal exit at line 70 via exit(status)
  - err() path at line 51 via atexit
- **Verdict: CLEAN** — single-owner argv with perfect atexit pairing

## Allocation Pairing Summary

| Port | Allocation | Type | Free | All Paths? | Notes |
|------|-----------|------|------|-----------|-------|
| true | None | N/A | N/A | N/A | Zero allocations |
| false | None | N/A | N/A | N/A | Zero allocations |
| colrm | getline() | malloc in stdio | free in cleanup | Yes ✓ | Tracking pointer + NULL guard at loop exit + atexit |
| colrm | argv expansion | amiport_expand_argv | amiport_free_argv in cleanup | Yes ✓ | atexit cleanup |
| factor | None dynamic | N/A | N/A | N/A | Only static tables |
| getopt | argv expansion | amiport_expand_argv | amiport_free_argv in cleanup | Yes ✓ | atexit cleanup |

## Cross-Cutting Pattern Notes
- All multi-exit ports (colrm, factor, getopt) use atexit(cleanup) — perfect consistency
- colrm's getline buffer tracking is the exemplary pattern for any buffered I/O
- factor demonstrates that static allocations (even TABSIZE = 262144) are safe when stack-local in non-recursive functions
- Zero realloc() calls in batch — no unsafe intermediate pointer risks

## Approved for Shipping
All 5 ports: **APPROVED** — zero memory safety issues detected.
