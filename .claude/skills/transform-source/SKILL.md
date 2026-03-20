---
name: transform-source
description: Transform C source code for Amiga compatibility. Replaces POSIX calls with Amiga equivalents or posix-shim wrappers. Use after analyze-source has identified issues.
argument-hint: <path-to-source>
allowed-tools: Read, Write, Edit, Bash, Grep, Glob
---

# Transform Source for Amiga Compatibility

You are transforming C source code from POSIX/Linux to AmigaOS 3.x compatibility.

## Process

1. **Read the analysis report** if one exists, or run `/analyze-source` first
2. **Create output directory** — copy source to a `ported/` directory alongside the original
3. **Apply transformations in order** (see references/transformation-rules.md):
   1. Header replacements
   2. Type replacements
   3. Function call replacements
   4. Constant replacements
   5. Conditional compilation blocks
   6. Amiga boilerplate (version string, stack cookie)
4. **Document every change** with an `/* amiport: ... */` comment
5. **Report what was changed**

## Rules

- **NEVER remove functionality** — stub it if no Amiga equivalent exists
- **Prefer posix-shim wrappers** (`amiport_*` functions) over raw AmigaDOS calls
- **Preserve the original** — always work on a copy in `ported/`
- **Use C89** — no C99 features (no `//` comments, no mixed declarations, no VLAs)
- **Add version string**: `static const char *verstag = "$VER: progname 1.0 (DD.MM.YYYY)";`
- **Set stack size**: `LONG __stack = 32768;`

## Reference Material

- `references/transformation-rules.md` — Detailed rules for each transformation type
- `references/amiga-api-reference.md` — AmigaOS API quick reference

## Include Path

Transformed code should use these includes for shim functions:
```c
#include <amiport/unistd.h>
#include <amiport/dirent.h>
#include <amiport/getopt.h>
#include <amiport/signal.h>
#include <amiport/sys/stat.h>
#include <amiport/sys/time.h>
```

## Output

After transformation, report:
- Number of files modified
- Number of transformations applied (by type)
- Any blocking issues that were stubbed
- Build command to compile the result
