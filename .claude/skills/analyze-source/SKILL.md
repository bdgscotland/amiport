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
6. **Classify each issue** by tier and severity (see ADR-008 and `docs/posix-tiers.md`):
   - **Tier 1 — Shim** (green): `trivial` or `needs-shim` — Direct or near-direct AmigaDOS mapping via `lib/posix-shim/`
   - **Tier 2 — Emulation** (yellow): `needs-emu` — Approximate mapping via `lib/posix-emu/` with documented behavioural differences
   - **Tier 3 — Redesign** (red): `needs-redesign` — No library can bridge this; requires structural rewrite using patterns from `redesign-patterns.md`
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
- `docs/posix-tiers.md` — Master tier classification with all functions, emulation details, and redesign patterns
- `.claude/skills/analyze-source/references/posix-to-amiga-map.md` — POSIX-to-AmigaOS function mapping table
- `.claude/skills/analyze-source/references/common-patterns.md` — Frequently encountered patterns
- `.claude/skills/transform-source/references/redesign-patterns.md` — Tier 3 redesign pattern templates

## Tier Classification Decision Tree

When encountering a POSIX function not already classified:

1. Does AmigaDOS have a function that does the same thing with the same calling convention?
   - YES → **Tier 1** (shim) — `trivial` or `needs-shim`
   - MOSTLY → **Tier 1** with documented edge cases
   - NO → continue
2. Can the behaviour be emulated with a stateful wrapper that handles the common case?
   - YES, common case works → **Tier 2** (emulation) — `needs-emu`
   - YES, but only niche cases → **Tier 3** (redesign)
   - NO → continue
3. Does the function represent a fundamentally different execution model?
   - YES → **Tier 3** (redesign) — `needs-redesign`
   - No Amiga equivalent at all → stub with warning, classify as **Tier 3**

## Output Format

Produce a report in this structure:

```
# Portability Analysis: <project name>

## Summary
- Total source files: N
- Lines of code: N
- Tier 1 (shim) issues: N (green)
- Tier 2 (emulation) issues: N (yellow)
- Tier 3 (redesign) issues: N (red)
- Architecture issues: N
- **Portability verdict**: [EASY | MODERATE | HARD | INFEASIBLE]

## POSIX Headers Found
| Header | Files Using It | Tier | Severity | Replacement |
|--------|---------------|------|----------|-------------|
| ...    | ...           | ...  | ...      | ...         |

## Tier 1 — Shim (Automated)
| Function | File:Line | Severity | Shim Wrapper | Notes |
|----------|-----------|----------|--------------|-------|
| ...      | ...       | ...      | ...          | ...   |

## Tier 2 — Emulation (Semi-automated)
| Function | File:Line | Emu Wrapper | Caveats |
|----------|-----------|-------------|---------|
| ...      | ...       | ...         | ...     |

## Tier 3 — Redesign (Human Review Required)
| Function | File:Line | Pattern | Recommendation |
|----------|-----------|---------|----------------|
| ...      | ...       | ...     | ...            |

[Detailed description of each Tier 3 issue with redesign pattern options]

## Architecture Issues
| Issue | File:Line | Severity | Notes |
|-------|-----------|----------|-------|
| ...   | ...       | ...      | ...   |

## Recommended Approach
[Summary of what the porting effort looks like, broken down by tier]
```

## Verdicts

- **EASY**: Only trivial and needs-shim issues. Standard pipeline will handle it.
- **MODERATE**: A few blocking issues that can be stubbed without losing core functionality.
- **HARD**: Multiple blocking issues affecting core functionality. Significant redesign needed.
- **INFEASIBLE**: Fundamental architecture incompatibility (e.g., requires fork/exec process model throughout, or is inherently multithreaded).
