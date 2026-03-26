---
name: ports/unexpand memory safety audit
description: Memory safety review of unexpand 1.13 (2026-03-26)
type: reference
---

# Memory Safety Audit: unexpand 1.13

**Status: CLEAN**
**Verdict: Approved for shipping — zero dynamic allocations**

## Allocations Found

| Location | Type | Free'd? | All paths? | Issue |
|----------|------|---------|------------|-------|
| N/A | None | N/A | N/A | No malloc/calloc/strdup calls |

## Memory Usage Analysis

**Static buffers:**
- Line 55: `char genbuf[BUFSIZ]` — static, stack-allocated, safe
- Line 56: `char linebuf[BUFSIZ]` — static, stack-allocated, safe

**Dynamic allocations:**
- None found. Program uses only static buffers and local variables.

**File handles:**
- Line 96: `freopen(argv[0], "r", stdin)` — returns FILE*, no new allocation, reassigns stdin
- stdin is not explicitly closed (correct — AmigaOS auto-closes on process exit)

## Exit Path Analysis

All exit paths covered:
- Line 111: `exit(0)` — normal exit ✓
- Line 82: `exit(10)` — pledge() error (stub, never triggers)
- Line 89: `exit(10)` — usage() error ✓
- Line 98: `exit(10)` — freopen() error ✓

**Cleanup function (lines 62-67):**
```c
static void cleanup(void)
{
    amiport_free_argv();
    (void)fflush(stdout);
}
```

Called at line 78 via `atexit(cleanup)`. Required for argv expansion, even though no other allocations exist.

## Summary
- Total allocations: 0 (only static buffers)
- Properly freed: N/A
- Leaks found: 0
- File handles: Properly handled (stdin reassignment via freopen, auto-close on exit)

## Verdict

**CLEAN** — Approved for shipping. Exemplary memory safety with zero dynamic allocations. Static buffers only, all exit paths covered by atexit cleanup.

