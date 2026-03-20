---
name: analyze-source
description: Analyze C source code for Amiga portability issues. Scans headers, system calls, and language features to produce a structured porting report. Use when starting a new port or evaluating whether a project can be ported.
argument-hint: <path-to-source>
allowed-tools: Read, Grep, Glob, Bash, Agent
---

# Analyze Source for Amiga Portability

You are analyzing C source code to determine what changes are needed to port it to AmigaOS 3.x on the Commodore Amiga.

## Process

1. **Scan all `.c` and `.h` files** in the provided path
2. **Catalog `#include` directives** — identify which headers are POSIX-specific vs standard C
3. **Identify system calls** — find all function calls that are POSIX/Linux-specific
4. **Detect blocking patterns** — fork/exec, mmap, pthreads, sockets, x86 asm
5. **Check architecture assumptions** (see below)
6. **Classify each issue** by severity:
   - `trivial` — Direct mapping exists, mechanical replacement
   - `needs-shim` — Requires amiport posix-shim wrapper
   - `blocking` — No Amiga equivalent, requires redesign or stubbing
7. **Produce a structured report**

## Architecture & Compiler Checks

The Amiga's m68k architecture and C89 compilers differ from modern x86/Linux in ways that cause subtle bugs. Look for these specifically:

### Endianness
- m68k is **big-endian**. Flag any code that assumes little-endian byte order: casting between `char*` and integer types, manual byte-swapping that assumes LE, network byte order functions used for host byte order (they're no-ops on BE).

### Integer Sizes
- On some m68k compilers, `int` may be 16-bit (though bebbo-gcc uses 32-bit). Flag any code relying on `sizeof(int) == 4` or `sizeof(long) == 8`. Amiga `LONG` is always 32-bit.
- `size_t` may differ. Flag raw casts between `size_t` and `int`.

### Alignment
- m68k requires 16-bit alignment for word/longword access. Flag `__attribute__((packed))` structs that are accessed via pointer — these cause bus errors on 68000 (ok on 68020+, but slower).
- Flag pointer arithmetic that casts between types of different alignment requirements.

### GNU Extensions
- Flag `__attribute__((…))` (except `unused` which is harmless), statement expressions `({…})`, `typeof`, zero-length arrays, `__builtin_*` functions.
- C99/C11 features to flag: `//` comments, mixed declarations and code, VLAs, `_Bool`, compound literals, designated initializers (some are OK with bebbo-gcc but risky).

### 64-bit Types
- No native 64-bit on m68k. Flag `long long`, `int64_t`, `uint64_t` — these work via emulation (slow) and may not be available in all runtimes.

## Reference Material

Consult these files for accurate classification:
- `.claude/skills/analyze-source/references/posix-to-amiga-map.md` — Master mapping table
- `.claude/skills/analyze-source/references/common-patterns.md` — Frequently encountered patterns

## Output Format

Produce a report in this structure:

```
# Portability Analysis: <project name>

## Summary
- Total source files: N
- Lines of code: N
- Trivial issues: N
- Needs-shim issues: N
- Blocking issues: N
- Architecture issues: N
- **Portability verdict**: [EASY | MODERATE | HARD | INFEASIBLE]

## POSIX Headers Found
| Header | Files Using It | Severity | Replacement |
|--------|---------------|----------|-------------|
| ...    | ...           | ...      | ...         |

## System Calls Requiring Changes
| Function | File:Line | Severity | Notes |
|----------|-----------|----------|-------|
| ...      | ...       | ...      | ...   |

## Architecture Issues
| Issue | File:Line | Severity | Notes |
|-------|-----------|----------|-------|
| ...   | ...       | ...      | ...   |

## Blocking Issues
[Detailed description of each blocking issue and suggested workaround]

## Recommended Approach
[Summary of what the porting effort looks like]
```

## Verdicts

- **EASY**: Only trivial and needs-shim issues. Standard pipeline will handle it.
- **MODERATE**: A few blocking issues that can be stubbed without losing core functionality.
- **HARD**: Multiple blocking issues affecting core functionality. Significant redesign needed.
- **INFEASIBLE**: Fundamental architecture incompatibility (e.g., requires fork/exec process model throughout, or is inherently multithreaded).
