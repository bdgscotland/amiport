---
name: review-amiga
description: Amiga-specific code review for ported programs. Checks for stack overflow risks, memory allocation patterns, BPTR handling, library cleanup, 68k alignment issues, and AmigaOS conventions. Use after porting or before publishing to Aminet.
argument-hint: <path-to-ported-source>
context: fork
allowed-tools: Read, Grep, Glob, Bash, Agent
---

# Amiga Code Review

You are reviewing the ported C code at `$ARGUMENTS` for AmigaOS-specific correctness and quality. This is NOT a general code review — it targets issues that only matter on the Amiga.

## What to Check

### 1. Stack Safety
- **Stack cookie present?** Every ported program needs `LONG __stack = NNNNN;`
- **Stack size adequate?** Default is 4KB. Ported programs typically need 32KB+. Recursive programs (find, diff, tree) need 64KB+.
- **Large stack allocations?** Flag any `char buf[4096]` or larger on the stack. On 68000, these cause silent stack overflow. Recommend `static` or heap allocation.
- **Deep recursion?** Flag recursive functions and check if `__stack` accounts for max depth.

### 2. Memory Patterns
- **CHIP vs FAST RAM**: `AllocMem()` with `MEMF_CHIP` wastes precious chip RAM. Ported code should use `MEMF_PUBLIC` or `MEMF_ANY` unless accessing custom chips.
- **Memory leaks on exit**: AmigaOS doesn't reclaim all memory on exit like Unix. Check that every `AllocVec`/`AllocMem` has a matching `FreeVec`/`FreeMem`. (Note: `malloc`/`free` via clib2 ARE cleaned up.)
- **Large allocations**: Flag any single allocation > 512KB — classic Amigas may only have 2MB total. Recommend graceful failure handling.

### 3. BPTR Handling
- **Never dereference a BPTR directly** — it's an address >> 2. Use `BADDR()` macro.
- **Lock cleanup**: Every `Lock()` must have a matching `UnLock()`. Check all error paths.
- **File handle cleanup**: Every `Open()` must have a matching `Close()`. Check all error paths.
- **CurrentDir cleanup**: If code calls `CurrentDir()`, the old lock must be saved and restored.

### 4. AmigaOS Library Protocol
- **OpenLibrary/CloseLibrary**: If code opens Amiga libraries directly, check they're closed on all exit paths (including error paths).
- **Version checking**: `OpenLibrary("dos.library", 36)` — check minimum version matches API usage. V36 = AmigaOS 2.0, V39 = AmigaOS 3.0.
- **Global base pointers**: If using `DOSBase`, `SysBase` etc. directly, verify they're set.

### 5. Path Handling
- **No hardcoded Unix paths**: Flag any `/tmp/`, `/dev/null`, `/home/`, `/usr/`. Must use `T:`, `NIL:`, `PROGDIR:`, etc.
- **Path separator**: Amiga uses `/` like Unix but volumes use `:`. Flag code that checks for leading `/` as absolute path indicator.
- **Case sensitivity**: Amiga filesystem is case-insensitive. Flag code that relies on case-sensitive filenames.

### 6. 68k Architecture
- **Alignment**: 68000 requires word (2-byte) alignment for word/long access. `__attribute__((packed))` structs accessed via pointer will crash on 68000.
- **Integer overflow**: If `int` is used for sizes > 32KB, verify the compiler config uses 32-bit int.
- **Endianness**: 68k is big-endian. Flag any code assuming little-endian byte order.

### 7. AmigaOS Conventions
- **Version string**: Must have `$VER: name version (DD.MM.YYYY)` — this is how the `version` command identifies programs.
- **Return codes**: AmigaOS uses RETURN_OK (0), RETURN_WARN (5), RETURN_ERROR (10), RETURN_FAIL (20). EXIT_SUCCESS (0) and EXIT_FAILURE (1) work but are non-standard on Amiga.
- **Ctrl-C handling**: Long-running programs should check for `SIGBREAKF_CTRL_C` periodically via `SetSignal()` or the amiport shim's `amiport_check_break()`.
- **stdout buffering**: Some Amiga terminals don't line-buffer. Use `fflush(stdout)` after important output.

### 8. amiport Shim Usage
- **Correct wrapper used?** Every POSIX call should go through `amiport_*` wrappers, not raw AmigaDOS.
- **Comments present?** Every transformed line should have `/* amiport: ... */`.
- **Tier classification correct?** Tier 1 functions use `amiport_*`, Tier 2 use `amiport_emu_*`.
- **fopen/fclose macro collision?** If the ported code implements its own file wrappers, check for the `amiport/stdio.h` macro collision (see CLAUDE.md known pitfalls).

### 9. Logic Bug Patterns (automated checks)
These produce **wrong output**, not crashes. You MUST run these grep commands on the ported source and report all matches found:

**Mandatory automated checks — run ALL of these:**
```bash
# Same-file detection (crash-patterns.md #4)
grep -rn 'st_ino\|st_dev' ported/*.c ported/*.h 2>/dev/null
# Case-sensitive filename comparison
grep -rn 'strcmp.*file\|strcmp.*path\|strcmp.*name' ported/*.c 2>/dev/null
# Hardlink/symlink assumptions
grep -rn 'S_ISLNK\|lstat\|readlink\|st_nlink' ported/*.c 2>/dev/null
# Stub return values used in logic
grep -rn 'getuid\|getgid\|getpid\|umask' ported/*.c 2>/dev/null
# File descriptor arithmetic
grep -rn 'STDIN_FILENO\|STDOUT_FILENO\|STDERR_FILENO' ported/*.c 2>/dev/null
# Long-running loops without Ctrl-C check
grep -rn 'while.*1\|for.*;;' ported/*.c 2>/dev/null
```

**For each match found, apply this triage:**
- **st_ino/st_dev**: Verify the code works with fib_DiskKey-based values (see crash-patterns.md #4). If used for same-file detection, flag as WARN.
- **strcmp on filenames/paths**: Flag as WARN — AmigaOS is case-insensitive, should use `strcasecmp()` or Amiga's `Stricmp()`.
- **S_ISLNK/lstat/readlink**: Flag as WARN — AmigaOS has no symlinks, this code path will never execute. Check if the fallback path is correct.
- **st_nlink**: Flag as WARN if used for hardlink detection — may not be populated by the shim.
- **getuid/getgid**: Flag as WARN if used in conditionals — always returns 0 on AmigaOS, so `if (getuid() == 0)` is always true.
- **getpid**: Flag as INFO if used for uniqueness (e.g., temp filenames) — works but returns a fixed value on some configs.
- **umask**: Flag as INFO — stubbed to no-op, file permissions are ignored on AmigaOS.
- **STDIN/STDOUT/STDERR_FILENO**: Flag as INFO if used in arithmetic; flag as WARN if used to determine fd count or ranges.
- **while(1)/for(;;)**: Flag as CRITICAL if the loop body does NOT contain `amiport_check_break()` or `SetSignal(...SIGBREAKF_CTRL_C...)`. On AmigaOS there is no OS-level SIGINT — without an explicit check, the user cannot interrupt the program. Fix by adding `#include <amiport/signal.h>` and `if (amiport_check_break()) return;` inside the loop.

**If a grep returns no matches, report "No matches — OK" for that check.** Do not skip checks that return empty results; the absence of matches is useful information.

## Reference Documentation

When reviewing API usage correctness, consult:
- `docs/references/adcd/FUNCTIONS.md` — Cross-reference for how functions are documented across all ADCD manuals
- `docs/references/adcd/TYPES.md` — Struct field documentation and usage patterns
- `docs/references/adcd/examples/` — Canonical code examples from Commodore's own documentation

## Output Format

```
# Amiga Code Review: <program>

## Critical Issues (must fix before release)
1. [STACK] ...
2. [MEMORY] ...

## Warnings (should fix)
3. [PATH] ...
4. [CONVENTION] ...

## Suggestions (nice to have)
5. [PERF] ...

## Amiga Readiness Score
- Stack safety: OK / WARN / FAIL
- Memory handling: OK / WARN / FAIL
- Path handling: OK / WARN / FAIL
- Library cleanup: OK / WARN / FAIL
- Conventions: OK / WARN / FAIL
- Logic bugs: OK / WARN / FAIL (from automated checks in §9)
- Overall: READY / NEEDS WORK / NOT READY
```
