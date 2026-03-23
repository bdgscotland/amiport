---
name: crash-patterns
description: Load the crash patterns knowledge base for AmigaOS debugging. Contains documented crash signatures, root causes, and fixes discovered across all ports. Use when debugging crashes, reviewing code for crash risks, or writing transformations that touch memory/IO/exit paths.
---

# Crash Patterns Knowledge Base

Load the crash patterns reference before debugging, transforming, or reviewing code.

Read the file `docs/references/crash-patterns.md` now. It contains:
- Numbered crash patterns with signatures, root causes, and fixes
- vamos vs real hardware behavior differences
- Common misdiagnoses and their actual root causes
- ARexx/FS-UAE testing pitfalls

**Every pattern in this file was discovered from a real bug in a real port.** Check against it before:
- Writing exit/cleanup code (patterns #5, #9)
- Handling file I/O (pattern #12)
- Using stack-heavy functions (patterns #7, #10)
- Writing test harnesses (pattern #9)
- Using vsnprintf (pattern #5)
- Mixing amiport_open with fdopen (pattern #12)
- Using MB_CUR_MAX in conditionals (pattern #11)
