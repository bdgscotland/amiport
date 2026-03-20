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
3. **Apply transformations in order** (see `.claude/skills/transform-source/references/transformation-rules.md`):
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
- **Add version string**: `static const char *verstag = "$VER: progname 1.0 (DD.MM.YYYY)";` (use today's date)
- **Set stack size**: `LONG __stack = 32768;`

## Shim Availability

The posix-shim (`lib/posix-shim/`) provides these wrapper families. Check the actual headers before using a function — some wrappers referenced in the mapping docs are planned but not yet implemented:

**Implemented and available:**
- File I/O: `amiport_open`, `amiport_close`, `amiport_read`, `amiport_write`, `amiport_lseek`, `amiport_unlink`, `amiport_rename`, `amiport_access`, `amiport_isatty`
- Directory: `amiport_opendir`, `amiport_readdir`, `amiport_closedir`
- Stat: `amiport_stat`, `amiport_fstat`, `amiport_mkdir`
- Time: `amiport_time`, `amiport_gettimeofday`, `amiport_usleep`
- Process: `amiport_sleep`, `amiport_getcwd`, `amiport_chdir`, `amiport_getenv`, `amiport_getpid`
- Signal: `amiport_signal`, `amiport_raise`, `amiport_check_break`
- Getopt: `amiport_getopt`
- String: `amiport_strtok_r`
- Misc: `amiport_tmpfile`
- Errno: `amiport_map_ioerr` (maps AmigaDOS IoErr to errno)

If you need a wrapper that doesn't exist yet, check `lib/posix-shim/include/` headers to verify. If it's truly missing, either add it to the shim or stub the call inline with a comment.

## Reference Material

- `.claude/skills/transform-source/references/transformation-rules.md` — Detailed rules for each transformation type
- `.claude/skills/transform-source/references/amiga-api-reference.md` — AmigaOS API quick reference

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

Or simply `#include <amiport/amiport.h>` for everything.

## Output

After transformation, report:
- Number of files modified
- Number of transformations applied (by type)
- Any blocking issues that were stubbed
- Any shim functions used that needed to be verified or added
- Build command to compile the result
