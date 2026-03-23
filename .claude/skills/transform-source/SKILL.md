---
name: transform-source
description: Transform C source code for Amiga compatibility. Replaces POSIX calls with Amiga equivalents or posix-shim wrappers. Use after analyze-source has identified issues.
argument-hint: <path-to-source>
allowed-tools: Read, Write, Edit, Bash, Grep, Glob
---

# Transform Source for Amiga Compatibility

You are transforming the C source code at `$ARGUMENTS` from POSIX/Linux to AmigaOS 3.x compatibility.

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

## Tiered Transformation (ADR-008)

Transformations differ by tier. See `docs/posix-tiers.md` for the full classification.

### Tier 1 — Shim (automated)
- Apply mechanically: header swap + function rename to `amiport_*` wrappers
- Comment: `/* amiport: replaced X() with amiport_X() */`
- No human review needed

### Tier 2 — Emulation (semi-automated)
- Apply the transformation using `amiport_emu_*` wrappers from `lib/posix-emu/`
- Add include: `#include <amiport-emu/select.h>` (or whichever module)
- **Always add a caveat comment**: `/* amiport-emu: select() emulated via WaitForChar() polling — 20ms granularity, no socket support */`
- Document caveats in PORT.md
- Link against `libamiport-emu.a` in addition to `libamiport.a`

### Tier 3 — Redesign (human review required)
- Do NOT silently stub these
- Do NOT auto-apply redesign patterns
- Add a clear comment marking the location: `/* amiport-redesign: NEEDS HUMAN REVIEW — fork()+exec() pattern, see redesign-patterns.md */`
- The port-coordinator presents redesign options to the user

## Rules

- **NEVER remove functionality** — stub it if no Amiga equivalent exists
- **Prefer posix-shim wrappers** (`amiport_*` functions) over raw AmigaDOS calls (Tier 1)
- **Prefer posix-emu wrappers** (`amiport_emu_*` functions) for Tier 2 over inline emulation
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

**Tier 2 Emulation wrappers** (in `lib/posix-emu/`):
- I/O multiplexing: `amiport_emu_select()` — emulated via WaitForChar() polling
- Memory mapping: `amiport_emu_mmap()`, `amiport_emu_munmap()` — read-only, AllocMem()+Read()
- Pipes: `amiport_emu_pipe()` — via PIPE: device
- Timers: `amiport_emu_alarm()`, `amiport_emu_check_alarm()` — via timer.device

Check `lib/posix-emu/include/amiport-emu/` headers for exact signatures and emulation notices.

**Console UI wrappers** (in `lib/console-shim/`):
- ncurses API subset: `initscr()`, `endwin()`, `getch()`, `addch()`, `addstr()`, `mvprintw()`, `attron()`, `attroff()`, `start_color()`, `init_pair()`, `keypad()`, `curs_set()`, `box()`, `newwin()`
- Headers: `<amiport-console/curses.h>`, `<amiport-console/term.h>`
- Link with: `-lamiport-console`

**Network wrappers** (in `lib/bsdsocket-shim/`):
- Socket API: `amiport_socket()`, `amiport_connect()`, `amiport_bind()`, `amiport_listen()`, `amiport_accept()`, `amiport_send()`, `amiport_recv()`, `amiport_closesocket()`, `amiport_net_select()`, `amiport_gethostbyname()`
- IP utilities: `amiport_inet_addr()`, `amiport_inet_ntoa()`, `amiport_inet_aton()`
- Headers: `<amiport-net/socket.h>`, `<amiport-net/netinet/in.h>`, `<amiport-net/netdb.h>`, `<amiport-net/arpa/inet.h>`
- Link with: `-lamiport-net`

Check `lib/console-shim/include/amiport-console/` and `lib/bsdsocket-shim/include/amiport-net/` headers for exact API.

## Reference Material

- `.claude/skills/transform-source/references/transformation-rules.md` — Detailed rules for each Tier 1 transformation type
- `.claude/skills/transform-source/references/amiga-api-reference.md` — AmigaOS API quick reference
- `.claude/skills/transform-source/references/redesign-patterns.md` — Tier 3 redesign pattern templates
- `docs/posix-tiers.md` — Master tier classification with all functions and emulation details
- `docs/references/adcd/FUNCTIONS.md` — Cross-reference for HOW to use AmigaOS functions (prose + examples)
- `docs/references/adcd/INCLUDES.json` — Maps include paths to relevant ADCD documentation chapters
- `docs/references/adcd/examples/` — Real AmigaOS code examples by library

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

For platform compatibility (alignment macros, compiler workarounds):
```c
#include <amiport/compat.h>
```
Use `AMIPORT_ALIGN(size, align)` when code uses `offsetof()` for memory alignment in custom allocators — 68k `offsetof()` returns 2 instead of 4/8, corrupting allocator metadata (crash-patterns #15).

## Output

After transformation, report:
- Number of files modified
- **Tier 1** transformations applied (by type) — automated, shim wrappers
- **Tier 2** transformations applied — emulation wrappers with caveats
- **Tier 3** issues flagged — redesign needed, awaiting human review
- Any shim/emu functions used that needed to be verified or added
- Build command to compile the result:
  - Include `-lamiport-emu` if Tier 2 functions used
  - Include `-lamiport-console` if ncurses functions used (Category 3)
  - Include `-lamiport-net` if socket functions used (Category 4)
