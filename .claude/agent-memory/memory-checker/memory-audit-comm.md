---
name: ports/comm memory safety audit
description: OpenBSD comm v1.11 memory safety review — CLEAN, no dynamic allocations
type: reference
---

# Memory Safety Audit: ports/comm (OpenBSD v1.11)

**Date:** 2026-03-24
**Status:** CLEAN — Approved for shipping
**Files Analyzed:** comm.c (215 lines)
**Category:** 1 (simple CLI, no interactive features)

---

## Summary

`comm` has **ZERO dynamic allocations** and **perfectly balanced file handle usage**. This port has no memory leaks, no use-after-free risks, no double-free risks, and no file descriptor leaks. All error paths are properly handled. The code is **memory-safe and approved for Aminet shipping**.

---

## Key Findings

### 1. Allocation Profile
- **malloc/calloc/realloc:** 0 uses
- **strdup:** 0 uses
- **Free calls needed:** 0 required
- **Impact:** Entire allocation safety analysis is N/A — no dynamic memory used

### 2. Static Allocations
Two static buffers are used for line input:
- **Line 78:** `static char line1[MAXLINELEN], line2[MAXLINELEN];`
  - MAXLINELEN = LINE_MAX + 1 = 2048 + 1 = 2049 bytes
  - Both allocated on BSS (binary data segment), not stack
  - Safe: static allocation is idempotent — same memory reused across loop iterations
  - **No leak**: Not freed because it's not allocated; it's pre-allocated at program load

### 3. File Handle Lifecycle

#### Input Files (fp1, fp2)

**Opened at lines 112-113:**
```c
fp1 = file(argv[0]);  /* line 112 */
fp2 = file(argv[1]);  /* line 113 */
```

**File function (lines 197-207):**
- Returns stdin directly if filename is `"-"` (line 203)
- Calls fopen() for regular files (line 204)
- Calls err(10, ...) and exits immediately if fopen fails (line 205)

**Read at lines 133-135:**
- `fgets()` calls read lines during main loop
- Loop exits on EOF (fgets returns NULL, file1done/file2done set)

**Closed via: IMPLICITLY at exit()**
- Both fp1 and fp2 are **never explicitly closed** with `fclose()`
- But: If either fp1 or fp2 is stdin, calling fclose(stdin) would be **dangerous** (known pitfall: crashes the console handler on Workbench)
- **Analysis**: The code is CORRECT — do NOT add fclose() calls
- On AmigaOS with `-noixemul`, file handles are closed automatically when the process exits
- If the code has been running on Unix/Linux, fp1 and fp2 are closed by the OS at process exit anyway
- **Verdict**: SAFE — no fclose() needed, and adding it could introduce the stdin crash bug

#### Output Files
- Line 176: `fflush(stdout)` (not fclose) — correct, avoids closing the shell's output handle
- Line 177: `ferror(stdout)` check
- Line 180: `exit(0)` — stdout is NOT closed, stdout is left open for the shell

---

## Exit Path Analysis

### main() Exit Paths

| Path | Location | File Handles Cleanup | Exit Code | Status |
|------|----------|---------------------|-----------|--------|
| Normal completion (file EOF) | line 141 or 146 | N/A (loop break) | 0 (line 180) | SAFE |
| stdout write error | line 154, 164, 170 | N/A (break loop) | 0 (line 180) | SAFE |
| Ctrl-C pressed | line 129 | N/A (break loop) | 0 (line 180) | SAFE |
| stdout ferror | line 178 | N/A (checked before exit) | 10 (err call) | SAFE |
| fopen(file1) fails | file() line 205 | N/A (exits immediately) | 10 (err call) | SAFE |
| fopen(file2) fails | file() line 205 | N/A (exits immediately) | 10 (err call) | SAFE |
| argc != 2 | line 110 | N/A (exits immediately) | 10 (usage call) | SAFE |
| pledge() fails | line 85 | N/A (exits immediately) | 10 (err call) | SAFE |
| getopt failure | line 104 | N/A (exits immediately) | 10 (usage call) | SAFE |

**All exit paths are safe.** There is no code path that:
- Leaves a regular file open (stdin is handled correctly)
- Leaves stdout in an invalid state
- Attempts to fclose(stdin) (the dangerous operation on Workbench)

### show() Early Return (line 190)
- Called from lines 140 and 145 (when one file reaches EOF)
- Returns early on Ctrl-C (line 190)
- No file handles are opened inside show()
- Pass-through function (fp is borrowed from caller, not owned by show())
- **Status**: SAFE

### usage() Exit (line 213)
- Calls `exit(10)` directly
- No cleanup needed (no files opened yet when getopt fails)
- **Status**: SAFE

---

## Detailed Code Walkthrough

### Stack Safety
- **Stack cookie**: `long __stack = 32768;` (line 59) — adequate for a simple comparison loop
- **Local variables in main()**:
  - `int comp, file1done, file2done, read1, read2` — 5 ints = 20 bytes
  - `int ch, flag1, flag2, flag3` — 4 ints = 16 bytes
  - `FILE *fp1, *fp2` — 2 pointers = 8 bytes
  - `char *col1, *col2, *col3` — 3 pointers = 12 bytes
  - `char **p` — 1 pointer = 4 bytes
  - `int (*compare)(...)` — 1 function pointer = 4 bytes
  - **Total locals**: ~64 bytes
- **Static line buffers**: 2 × 2049 bytes = 4098 bytes (BSS, not stack)
- **Safe**: Stack frame <<< 32KB available

### No Recursion
- main() calls file() and show()
- file() and show() do not call main() — no recursion possible
- show() is not recursive (simple do-while loop over fgets)

### Ctrl-C Handling
- Lines 127-130: Check in main loop
- Lines 188-191: Check in show() loop
- Correct use of amiport_check_break() (required on AmigaOS for Ctrl-C interruptibility)

---

## Known Pitfalls: All Avoided

| Pitfall | Status | Notes |
|---------|--------|-------|
| vsnprintf(NULL, 0, ...) | N/A | No vsnprintf usage |
| MB_CUR_MAX conditional | N/A | No multibyte code |
| fclose(stdin) | AVOIDED | Code never calls fclose on input files |
| Exit path cleanup | OK | No malloc'd data to clean |
| amiport_open + fdopen | N/A | Uses fopen(), not amiport_open() |
| __progname weak symbol | OK | No usage() that formats __progname |
| argv expansion cleanup | N/A | Does not use amiport_expand_argv() |
| stdin redirect in Execute | OK | printf output only, no input() |

---

## Allocation Table

| Location | Type | Allocated | Freed | All Paths? | Issue |
|----------|------|-----------|-------|-----------|-------|
| line 78 | static char[] | Static BSS | N/A | N/A | CLEAN — static allocation |
| line 112-113 | FILE* (fopen) | In file() | Never | YES | INTENTIONAL — see notes below |
| stdin passed | FILE* (returned) | N/A (preexisting) | N/A | N/A | SAFE — borrowed handle |
| stdout | FILE* (global) | N/A (preexisting) | fflush only | N/A | SAFE — correct handling |

**Note on FILE* closure:** Both fp1 and fp2 are left open at program exit. This is INTENTIONAL and CORRECT:
- If either is stdin, calling fclose() would crash the Workbench console (known AmigaOS pitfall)
- The AmigaOS exit handler closes all file handles when the process terminates
- POSIX systems also close all file descriptors at process exit
- DO NOT add fclose() calls — this would introduce a new crash risk

---

## Recommendations

**SHIPPING VERDICT: APPROVED** ✓

No changes required. The code is memory-safe and handles file I/O correctly for AmigaOS.

### Optional Enhancements (not required for shipping)

1. **Add explicit fclose() for non-stdin files only** (if desired for cleanliness):
   ```c
   /* After main loop, before exit: */
   if (fp1 != stdin) fclose(fp1);
   if (fp2 != stdin) fclose(fp2);
   ```
   This is NOT required (OS cleanup is automatic) but may be seen as more "proper" on Unix-like systems. On AmigaOS with `-noixemul`, it makes no functional difference.

2. **Explicit test for the stdin case**:
   The code passes `"-"` as an argument and correctly returns stdin (line 203). This behavior works but is not explicitly tested. Test case recommended:
   ```
   TEST: Use stdin as file1
   LAUNCH: WORK:comm - WORK:file2.txt
   (input second file content on stdin)
   EXPECT_RC: 0
   ```
   But this is not memory-safety related.

---

## Final Assessment

✓ **No memory leaks**
✓ **No double-free risks**
✓ **No use-after-free**
✓ **No file descriptor leaks**
✓ **No stack overflow risks**
✓ **No allocation failures to handle**
✓ **Correct error path handling**
✓ **Safe stdin/stdout treatment**

**Status: CLEAN — ready for Aminet submission**

---

## Cross-Reference

- **Crash Patterns KB**: No patterns match this code
- **Known Pitfalls**: All avoided
- **AmigaOS Specific**: stdin/stdout handling is exemplary
