---
name: review-amiga
description: Amiga-specific code review for ported programs. Checks for stack overflow risks, memory allocation patterns, BPTR handling, library cleanup, 68k alignment issues, and AmigaOS conventions. Use after porting or before publishing to Aminet.
argument-hint: <path-to-ported-source>
allowed-tools: Read, Grep, Glob, Bash, Agent
---

# Amiga Code Review

You are reviewing ported C code for AmigaOS-specific correctness and quality. This is NOT a general code review — it targets issues that only matter on the Amiga.

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
- Overall: READY / NEEDS WORK / NOT READY
```
