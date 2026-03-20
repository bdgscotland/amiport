# Console ANSI Escape Sequence Mapping

Reference for the `console-shim` library — maps ncurses operations to ANSI escape sequences supported by Amiga console.device.

## Amiga Console.device ANSI Support

The Amiga console.device (CON:/RAW: handlers) supports a subset of ANSI X3.64 / VT100 escape sequences. This is sufficient for most terminal UI programs.

## Escape Sequence Reference

### Cursor Movement

| ncurses Function | ANSI Sequence | Description | Amiga Support |
|-----------------|---------------|-------------|---------------|
| `move(y, x)` | `ESC[{y+1};{x+1}H` | Move cursor to position | Yes |
| Cursor up | `ESC[{n}A` | Move cursor up n rows | Yes |
| Cursor down | `ESC[{n}B` | Move cursor down n rows | Yes |
| Cursor forward | `ESC[{n}C` | Move cursor right n cols | Yes |
| Cursor back | `ESC[{n}D` | Move cursor left n cols | Yes |
| `home` | `ESC[H` | Move cursor to (0,0) | Yes |
| Save cursor | `ESC 7` | Save cursor position | Yes |
| Restore cursor | `ESC 8` | Restore cursor position | Yes |

### Screen Clearing

| ncurses Function | ANSI Sequence | Description | Amiga Support |
|-----------------|---------------|-------------|---------------|
| `clear()` | `ESC[2J ESC[H` | Clear screen, home cursor | Yes |
| `erase()` | `ESC[2J` | Clear screen | Yes |
| `clrtoeol()` | `ESC[K` | Clear to end of line | Yes |
| `clrtobot()` | `ESC[J` | Clear to end of screen | Yes |

### Text Attributes (SGR — Select Graphic Rendition)

| ncurses Attribute | ANSI Sequence | Description | Amiga Support |
|------------------|---------------|-------------|---------------|
| `A_NORMAL` | `ESC[0m` | Reset all attributes | Yes |
| `A_BOLD` | `ESC[1m` | Bold/bright | Yes |
| `A_DIM` | `ESC[2m` | Dim | Partial (ignored by some consoles) |
| `A_UNDERLINE` | `ESC[4m` | Underline | Yes |
| `A_BLINK` | `ESC[5m` | Blink | Yes (but slow) |
| `A_REVERSE` | `ESC[7m` | Reverse video | Yes |
| `A_INVIS` | `ESC[8m` | Invisible | Partial |

### Colors (SGR)

| Color | Foreground | Background | Amiga Support |
|-------|-----------|-----------|---------------|
| Black | `ESC[30m` | `ESC[40m` | Yes |
| Red | `ESC[31m` | `ESC[41m` | Yes |
| Green | `ESC[32m` | `ESC[42m` | Yes |
| Yellow | `ESC[33m` | `ESC[43m` | Yes |
| Blue | `ESC[34m` | `ESC[44m` | Yes |
| Magenta | `ESC[35m` | `ESC[45m` | Yes |
| Cyan | `ESC[36m` | `ESC[46m` | Yes |
| White | `ESC[37m` | `ESC[47m` | Yes |

**Note:** Amiga console.device supports 8 standard ANSI colors. The actual displayed colors depend on the Workbench palette (4-color screens show fewer colors). High-color modes (256-color, true-color) are not supported.

### Cursor Visibility

| ncurses Function | ANSI Sequence | Description | Amiga Support |
|-----------------|---------------|-------------|---------------|
| `curs_set(0)` | `ESC[?25l` | Hide cursor | Partial (depends on console handler) |
| `curs_set(1)` | `ESC[?25h` | Show cursor (normal) | Yes |

### Screen Modes

| Feature | ANSI Sequence | Description | Amiga Support |
|---------|---------------|-------------|---------------|
| Alt screen | `ESC[?47h` | Switch to alternate screen | No (we fake it with clear) |
| Normal screen | `ESC[?47l` | Switch back to normal screen | No |

### Input (RAW: mode)

| Key | Escape Sequence Received | ncurses Key Code |
|-----|-------------------------|-----------------|
| Up arrow | `ESC[A` | `KEY_UP` |
| Down arrow | `ESC[B` | `KEY_DOWN` |
| Right arrow | `ESC[C` | `KEY_RIGHT` |
| Left arrow | `ESC[D` | `KEY_LEFT` |
| Home | `ESC[H` or `ESC[1~` | `KEY_HOME` |
| End | `ESC[F` or `ESC[4~` | `KEY_END` |
| Insert | `ESC[2~` | `KEY_IC` |
| Delete | `ESC[3~` | `KEY_DC` |
| Page Up | `ESC[5~` | `KEY_PPAGE` |
| Page Down | `ESC[6~` | `KEY_NPAGE` |
| F1-F4 | `ESC OP` through `ESC OS` | `KEY_F(1)` - `KEY_F(4)` |
| F5-F8 | `ESC[11~` through `ESC[14~` | `KEY_F(5)` - `KEY_F(8)` |
| F9-F10 | `ESC[20~` through `ESC[21~` | `KEY_F(9)` - `KEY_F(10)` |
| Backspace | `0x08` or `0x7F` | `KEY_BACKSPACE` |
| Enter | `0x0D` | `'\n'` |

**Note:** Amiga function key sequences may differ from VT100. The console.device generates slightly different sequences depending on the console handler (CON:, KingCON, ViNCEd). The shim handles the most common variants.

## Console Mode Switching

| Mode | AmigaDOS API | POSIX Equivalent |
|------|-------------|-----------------|
| Cooked (line-buffered) | `SetMode(fh, 0)` — CON: | Default terminal mode |
| Raw (char-at-a-time) | `SetMode(fh, 1)` — RAW: | `tcsetattr()` with raw/cbreak |

The console-shim calls `SetMode(Input(), 1)` during `initscr()` and `SetMode(Input(), 0)` during `endwin()`.

## Window Size Detection

Amiga console doesn't have a standard `TIOCGWINSZ` ioctl. Options:

1. **Default 80x24** — safe fallback
2. **Intuition window query** — get the Window pointer via the console handler's process, then read `Window->Width/Height` and divide by font dimensions
3. **Environment variables** — check `LINES` and `COLUMNS` env vars
4. **CSI device status** — some console handlers respond to `ESC[6n` with cursor position

The console-shim currently defaults to 80x24 with plans to add Intuition-based detection.

## Limitations

1. **No mouse support** — console.device doesn't report mouse events
2. **No alternate screen buffer** — Amiga console has only one screen buffer; we clear/restore as best we can
3. **Color depends on palette** — a 4-color Workbench screen can only show 4 of the 8 ANSI colors
4. **Line drawing characters** — we use ASCII fallbacks (`+`, `-`, `|`) since Amiga fonts may not have box-drawing characters
5. **Wide characters** — not supported; Amiga console is strictly 8-bit
6. **Window resize** — no SIGWINCH equivalent; programs must poll or use fixed size
