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
- Amiga boilerplate added (version string, stack cookie)

### 3. Build

```bash
/build-amiga
```

Cross-compiles the ported source using the Docker toolchain. Handles:
- Compiler flag selection based on target profile
- Linking with posix-shim library (+ console-shim/bsdsocket-shim for Categories 3-4)
- Iterative error fixing

### 4. Test

```bash
/test-amiga
```

Runs the compiled binary in vamos and verifies output.

### 5. Package

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
