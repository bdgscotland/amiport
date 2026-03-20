# ADR-009: Console UI Shim for ncurses Porting

## Status

Accepted

## Date

2026-03-20

## Context

Many high-value Unix programs use ncurses (or termcap/terminfo) for interactive terminal UIs: less, vim, nano, htop, mc, mutt, lynx. These programs can't be ported with the existing posix-shim because they need terminal control capabilities — cursor positioning, color, line drawing, raw keyboard input.

AmigaOS has two relevant subsystems:

1. **console.device** — Supports a subset of ANSI/VT100 escape sequences for cursor control, color, scrolling, and basic screen manipulation. Programs running in an Amiga Shell window can use these directly.

2. **Raw console mode** — The console can be opened in RAW: mode (vs CON:) for character-at-a-time input without line buffering, analogous to Unix's `cbreak`/`raw` terminal modes.

The key insight is that Amiga console.device is already ANSI-compatible enough for many ncurses use cases. We don't need to emulate a full VT100 — we need a minimal ncurses API that generates ANSI escape sequences and reads raw input.

## Decision

Create `lib/console-shim/` providing a subset of the ncurses API sufficient to port interactive console programs. The shim:

1. **Outputs ANSI escape sequences** to the Amiga console.device via stdout
2. **Reads raw input** by opening a RAW: console handler or using `console.device` packet-mode
3. **Provides the ncurses API surface** that the source-analyzer categorizes as Tier 1 (direct) or Tier 2 (emulated)

### Supported ncurses API subset

**Tier 1 (direct ANSI mapping):**
- `initscr()` / `endwin()` — setup/teardown (save/restore terminal state)
- `move(y, x)` / `mvaddch()` / `mvaddstr()` — cursor positioning + output
- `addch()` / `addstr()` / `printw()` — output at current position
- `clear()` / `erase()` / `clrtoeol()` / `clrtobot()` — screen clearing
- `refresh()` — flush output buffer
- `attron()` / `attroff()` / `attrset()` — bold, underline, reverse
- `start_color()` / `init_pair()` / `COLOR_PAIR()` — ANSI 8-color support
- `getmaxy()` / `getmaxx()` — query window size
- `scrollok()` — enable/disable scrolling

**Tier 2 (emulated with caveats):**
- `getch()` / `ungetch()` — raw character input (needs RAW: mode)
- `nodelay()` / `timeout()` — non-blocking input (via `WaitForChar()`)
- `keypad()` — function key / arrow key decoding
- `curs_set()` — cursor visibility
- `newwin()` / `subwin()` — basic sub-window support (limited)
- `wborder()` / `box()` — line drawing (using `+`, `-`, `|` or Unicode if supported)

**Not supported (Tier 3 / out of scope):**
- `newterm()` / multiple terminals
- Mouse support (`mousemask()`, `getmouse()`)
- Wide character support (`addwch()`, etc.)
- Panels, menus, forms libraries
- Pad support
- Full terminfo database

### Library structure

```
lib/console-shim/
├── include/amiport-console/
│   ├── curses.h          # Main header (drop-in for <curses.h>)
│   └── term.h            # Terminal capability stubs
├── src/
│   ├── initscr.c         # Initialization and cleanup
│   ├── output.c          # Character/string output with ANSI escapes
│   ├── input.c           # Raw keyboard input
│   ├── color.c           # ANSI color support
│   ├── window.c          # Window management (stdscr + basic subwindows)
│   └── attr.c            # Attribute handling
└── Makefile
```

### Header replacement

```
#include <curses.h>     → #include <amiport-console/curses.h>
#include <ncurses.h>    → #include <amiport-console/curses.h>
#include <term.h>       → #include <amiport-console/term.h>
```

### Testing strategy

Console UI programs cannot be effectively tested via vamos (limited terminal emulation). Testing strategy:

1. **Build** via the normal cross-compilation pipeline
2. **Basic smoke test** via vamos (program starts, prints version, exits cleanly)
3. **Interactive testing** via FS-UAE (CIDR-004 workflow)
4. **Future:** Automated FS-UAE testing with input scripting

## Consequences

### Positive

- Unlocks porting of high-demand programs (less, vim, nano)
- Leverages AmigaOS's existing ANSI compatibility — minimal emulation needed
- Self-contained library, doesn't affect existing CLI ports
- The subset covers the vast majority of real ncurses usage in portable programs

### Negative

- Not a full ncurses replacement — programs using advanced features need manual work
- No mouse support (Amiga console.device doesn't support mouse reporting)
- Line drawing characters may look different depending on the Amiga font
- Testing is harder (FS-UAE required for interactive verification)
- Window size detection may not work in all console environments

### Neutral

- Programs using only printf/fgets for "interactive" output (like less's simple pager mode) may work with just posix-shim; console-shim is for programs that actively control the terminal
