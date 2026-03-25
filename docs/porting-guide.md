# Porting Guide

## Quick Start

The fastest way to port a C program to the Amiga:

```bash
# In Claude Code, from the amiport directory:
/port-project /path/to/your/source.c
```

This runs the full pipeline: analyze, transform, build, test, package.

## Manual Pipeline

For more control, run each stage individually:

### 0. Research

Dispatch the `aminet-researcher` agent to check whether the tool already exists for AmigaOS 3.x. If a recent, functional port exists, don't duplicate work.

### 0b. Dependency Audit (conditional)

For projects with external library dependencies (beyond libc/libm), dispatch the `dependency-auditor` agent. Skip for single-file CLI tools with only standard C library deps.

### 1. Analyze

```bash
/analyze-source /path/to/source/
```

Produces a portability report identifying:
- POSIX headers that need replacement
- System calls that need shimming
- Blocking patterns (fork, mmap, pthreads) that require redesign
- **Port category** (CLI, scripting, console UI, network, GUI)
- Severity classification (trivial / needs-shim / needs-emu / needs-redesign)

### 2. Transform

```bash
/transform-source /path/to/source/
```

Creates a `ported/` directory alongside the original with:
- Headers swapped for amiport shim equivalents
- Function calls replaced with `amiport_*` wrappers
- Blocking patterns stubbed with clear messages
- Amiga boilerplate added (version string, stack cookie, argv expansion)
- `__progname` auto-initialized by `amiport_expand_argv()` — no per-port boilerplate needed
- Long-running loops get `amiport_check_break()` for Ctrl-C support (no OS-level SIGINT on AmigaOS)

### 2b. Hardware Review (conditional — Category 3+ ports)

For Category 3 (Console UI), Category 4 (Network), or any port with unusual memory allocation, dispatch the `hardware-expert` agent to review hardware assumptions before building.

### 3. Build

```bash
/build-amiga
```

Cross-compiles the ported source using the Docker toolchain. Handles:
- Compiler flag selection based on target profile
- Linking with posix-shim library (+ console-shim/bsdsocket-shim for Categories 3-4)
- Iterative error fixing

**Category 3 (Console UI) build configuration:**
- Add `-I../../lib/console-shim/include` to CFLAGS
- Add `-L../../lib/console-shim -lamiport-console` to LDFLAGS
- For termcap programs (less, vi): `#include <amiport-console/term.h>` replaces `<termcap.h>`
- For termios programs: `#include <amiport/termios.h>` replaces `<termios.h>`
- For ncurses programs (nano, htop): `#include <amiport-console/curses.h>` replaces `<curses.h>`
- Hand-craft `defines.h` for programs that use autoconf (set HAVE_TERMCAP_H=1, HAVE_TERMIOS_H=1, etc.)
- Use `-std=gnu99` if the upstream source requires C99 features (ADR-022)

### 4. Test

First, dispatch the `test-designer` agent to generate a comprehensive test suite for the port. Test cases should NOT be written manually — the test-designer produces thorough coverage including edge cases, error paths, and boundary conditions.

Then validate test suite completeness:

```bash
make check-test-coverage
```

This checks that the generated test suite meets the project's coverage standards.

Run the compiled binary in vamos:

```bash
/test-amiga
```

This works for Category 1-2 ports (CLI tools, scripting interpreters).

For Category 3-4 ports (console UI, network), also run automated FS-UAE testing:

```bash
make test-fsemu TARGET=ports/<name>
```

This boots FS-UAE, runs an ARexx test harness inside AmigaOS, and produces a `TEST-REPORT.md` with TAP-format results. Test cases are defined in `test-fsemu-cases.txt` inside the port directory. The emulator exits automatically when tests complete (via UAEQuit) or a watchdog timeout fires. See ADR-014 for details.

For Category 3 (Console UI) and Category 4 (Network) ports, add `ITEST:` blocks to `test-fsemu-cases.txt` for automated interactive testing. These use KeyInject (`toolchain/keyinject/`) to inject keystrokes via `commodities.library/AddIEvents()`. Interactive tests are skipped on vamos. Example:

```
ITEST: quit with q key
LAUNCH: WORK:less WORK:test-file.txt
KEYS: WAIT1500,q
EXPECT_RC: 0
```

For **visual verification** (ADR-024), create a **separate** `test-fsemu-visual-cases.txt` with `SCRAPE` tests. Functional and visual tests MUST be separate FS-UAE passes -- never mix them. Resource exhaustion at ~13 ITESTs is a hard wall.

```
# In test-fsemu-visual-cases.txt (NOT test-fsemu-cases.txt):
ITEST: Visual: file content appears on screen
LAUNCH: WORK:mg -n WORK:test-file.txt
KEYS: WAIT2000,CTRL_X,WAIT300,CTRL_C
SCRAPE
EXPECT_AT 1,1,Hello, Amiga world!
EXPECT_CURSOR 1,1
EXPECT_RC: 0
```

Run the two passes separately:
```bash
make test-fsemu TARGET=ports/<name>           # Functional pass
make test-fsemu TARGET=ports/<name> VISUAL=1  # Visual pass (--visual flag)
```

Requires the forked FS-UAE with ANSI console capture (`~/Developer/fs-uae/`). Note: `CMD_WRITE` captures static display (file load, help text) but NOT interactive echo (typed characters, cursor movement).

For **cursor position verification**, add `SCREEN_READ` and `EXPECT_TRAP_CURSOR row,col` directives. This reads the cursor directly from the ConUnit struct in emulated memory via a custom FS-UAE trap. Use this for interactive cursor operations (arrow keys, C-n/C-p, search positioning). `EXPECT_CURSOR` only works for static display (ANSI reconstruction).

### 5. Review (recommended)

Run `/review-amiga` on the ported source to check for Amiga-specific issues:
- Stack safety and sizing
- BPTR handling and cleanup
- Memory allocation patterns
- Path conventions (no hardcoded Unix paths)
- AmigaOS conventions (version string, return codes)

Then dispatch the `memory-checker` agent (**mandatory**) to find memory leaks, double-frees, and allocation safety issues. AmigaOS has no memory protection — every leak persists until reboot.

For performance-critical ports, optionally dispatch the `perf-optimizer` agent to suggest 68k-specific optimizations.

For Category 3+ ports (console UI, network, GUI), dispatch the `hardware-expert` agent to review hardware assumptions — Chip RAM allocation sizes, chipset-specific features, address space assumptions. Also dispatch it to audit any reference docs or agent prompts that contain hardware claims.

### 6. Package

Built into the build skill — creates an LHA archive with the binary and documentation.

## What Can Be Ported

**Good candidates** (high success rate):
- CLI utilities (wc, cat, head, tail, sort, uniq, cut, tr)
- Text processors (sed-like tools, simple parsers)
- Math/science tools (calculators, unit converters)
- Games with text/simple graphics
- Compression tools (with bundled algorithm implementations)

**Category 2 — Scripting interpreters** (Lua, bc, awk):
- Mostly portable C, minimal POSIX surface
- May need dlopen stubs, path remapping
- Test via vamos

**Category 3 — Console UI apps** (less, vim, nano):
- Programs using ncurses/termcap
- Link with `-lamiport-console` (maps ncurses to Amiga console.device ANSI escapes)
- Replace `#include <curses.h>` → `#include <amiport-console/curses.h>`
- Test interactively via FS-UAE (vamos has limited terminal support)
- See `docs/references/console-ansi-mapping.md` for supported escape sequences

**Category 4 — Network apps** (curl, wget, irc):
- Programs using BSD sockets
- Link with `-lamiport-net` (wraps bsdsocket.library with auto lifecycle)
- Replace `#include <sys/socket.h>` → `#include <amiport-net/socket.h>` etc.
- Test via FS-UAE with a TCP/IP stack (AmiTCP or Roadshow)
- See `docs/references/bsdsocket-mapping.md` for full API mapping

**Not currently supported**:
- GUI applications (GTK, Qt, X11) — Category 5, future
- Programs requiring fork/exec process model
- Programs depending on shared memory (mmap with MAP_SHARED)
- Multithreaded programs (pthreads)
- Programs with x86 inline assembly

## Tips for Successful Ports

1. **Start small**: Port the simplest version of a program first
2. **Check dependencies**: Programs with few external library dependencies port easiest
3. **Read the analysis report**: It tells you exactly what's blocking
4. **Test incrementally**: Build and test after each transformation, not just at the end
5. **Stack size**: Set `__stack` to at least 32768 — many ported programs need more stack than the Amiga default
6. **Alignment differs on 68k**: `offsetof()` returns 2-byte alignment, not 4/8 as on x86/ARM. Custom allocators using `offsetof()` for block alignment will corrupt metadata. Use `AMIPORT_ALIGN()` from `<amiport/compat.h>` (crash-patterns #15).
7. **Use -O0 for struct-heavy code**: bebbo-gcc (GCC 6.5.0b) corrupts struct-by-value returns > 8 bytes at `-O1`/`-O2`. If a port crashes with data corruption after struct-returning functions, rebuild with `-O0` (crash-patterns #16).
