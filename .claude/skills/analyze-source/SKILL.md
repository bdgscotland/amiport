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
5. **Classify each issue** by severity:
   - `trivial` — Direct mapping exists, mechanical replacement
   - `needs-shim` — Requires amiport posix-shim wrapper
   - `blocking` — No Amiga equivalent, requires redesign or stubbing
6. **Produce a structured report**

## Reference Material

Consult these files for accurate classification:
- `references/posix-to-amiga-map.md` — Master mapping table
- `references/common-patterns.md` — Frequently encountered patterns

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
- **Portability verdict**: [EASY | MODERATE | HARD | INFEASIBLE]

## POSIX Headers Found
| Header | Files Using It | Severity | Replacement |
|--------|---------------|----------|-------------|
| ...    | ...           | ...      | ...         |

## System Calls Requiring Changes
| Function | File:Line | Severity | Notes |
|----------|-----------|----------|-------|
| ...      | ...       | ...      | ...   |

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
