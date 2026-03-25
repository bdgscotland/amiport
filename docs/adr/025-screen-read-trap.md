# ADR-025: FS-UAE Custom Trap for Interactive Visual Verification

## Status

Accepted

## Date

2026-03-24

## Context

ADR-024 (ANSI capture) enables visual verification of static display (file load,
help text) by capturing CMD_WRITE output. However, interactive rendering --
cursor movement from keystrokes, typed character echo -- does NOT go through
CMD_WRITE. console.device renders these directly to the RastPort, bypassing
the capture hook.

This creates a gap: we can assert what a program displays on load (EXPECT_AT),
but not where the cursor moves after a keystroke. For editors like mg, cursor
movement IS the core functionality.

Additionally, mg's cursor arrow keys were broken: AmigaOS console.device sends
CSI (0x9B) for cursor keys, but mg's VT100 bindings expect ESC [ (0x1B 0x5B).
The METABIT handler incorrectly decomposed 0x9B as Meta-ESC.

### Rejected Approaches

**A: CSI 6n Device Status Report.** Send CSI 6n to console.device, which
replies with CSI row;col R. Uncertain whether the response goes through
CMD_WRITE (likely goes to input stream instead). Timing-sensitive.

**B: ScreenRead discovers ConUnit via ACTION_DISK_INFO.** ScreenRead runs
in the test harness process, not in mg's console. Input() returns the harness
console, not mg's. ACTION_DISK_INFO would find the wrong ConUnit.

## Decision

### 1. FS-UAE Trap Mode 150

Add mode 150 to the mousehack_done trap dispatcher. The handler reads ConUnit
cursor position (cu_XCCP, cu_YCCP) and screen dimensions from emulated Amiga
memory using get_word(), and writes a JSON file to the ANSI log directory.

Register convention (matches existing modes 101/102):
- **D1** = mode number (150) -- NOT D0
- **A1** = ConUnit address (0 = auto-detect from unit_logs[])

When conunit_addr is 0, the handler uses the most recently active ConUnit
from the existing unit_logs[] array (highest address = newest console window).

ConUnit field offsets (verified by hardware-expert against ADCD conunit.h):
- +62: cu_XCCP (WORD) -- cursor column
- +64: cu_YCCP (WORD) -- cursor row
- +42: cu_XMax (WORD) -- max column
- +44: cu_YMax (WORD) -- max row

### 2. ScreenRead Binary

Minimal Amiga-side trigger binary (toolchain/screenread/). Does NOT discover
the ConUnit -- passes 0 in A1, letting the trap use unit_logs[].

Probes 3 candidate rtarea addresses (RTAREA_DEFAULT 0xF00000, RTAREA_BACKUP
0xEF0000, RTAREA_BACKUP_2 0xDB0000) for an A-line trap opcode before calling.
Exits safely with an error message on non-UAE systems.

Uses a .s asm stub for the trap call (not inline asm) due to bebbo-gcc
register allocation limitations.

### 3. EXPECT_TRAP_CURSOR Directive

New test directive `EXPECT_TRAP_CURSOR row,col` reads cursor position from
the .screen JSON file. Separate from existing `EXPECT_CURSOR` (which reads
from pyte's ANSI reconstruction). The trap cursor is authoritative for
interactive operations.

### 4. CSI 0x9B Translation

Add CSI check before the METABIT handler in mg's kbd.c. Decomposes 0x9B
into ESC + [ (the correct VT100 sequence). Guarded by `#ifdef __AMIGA__`.

This is a general pitfall affecting ALL console programs that use VT100
key bindings on AmigaOS.

### 5. Host-Side Key Injection via osascript

AddIEvents() (used by KeyInject) does not reliably deliver keystrokes to RAW
mode programs in visual test passes. The emulated input.device processes
AddIEvents() events differently from SDL-injected keyboard events.

Solution: `scripts/inject-keys.sh` sends keystrokes from the macOS host via
`osascript` (System Events), which feeds into FS-UAE's SDL input path -- the
same path as physical keypresses.

Sentinel handshake protocol:
1. ARexx harness writes `T:keys-request-N` with the KEYS token sequence
2. Host-side `test-fsemu.sh` polls for sentinel, calls inject-keys.sh
3. inject-keys.sh batches all keystrokes into ONE osascript call (~1.7s total)
4. Host writes `T:keys-done-N` to signal completion
5. ARexx harness polls for `keys-done-N` before proceeding

The `CLEANUP:` directive sends quit keys after SCRAPE capture is complete.

### 6. ConUnit Cursor Limitations in RAW Mode

cu_XCCP/cu_YCCP are only updated in COOKED mode. When a program switches to
RAW mode via SetMode(fh, 1), console.device's CSI processing is bypassed and
the cursor fields stay at (0,0). RastPort cp_x/cp_y also always read (0,0)
because console.device uses Layer-level rendering.

For RAW mode programs, cursor position is verified via the program's own
status line. mg's status line (row 29 on a 30-row window) shows (line,col)
and is updated via CMD_WRITE, so ANSI capture + pyte reconstruction can
read it with EXPECT_AT.

### 7. FS-UAE Joystick Port 1 Arrow Key Theft

FS-UAE defaults to mapping host arrow keys to joystick port 1. This must be
disabled with `joystick_port_1_mode = nothing` in BOTH the static config
(`toolchain/configs/amiport-test.fs-uae`) AND the generated config in
`scripts/test-fsemu.sh`. Without this, arrow keys produce zero bytes in
RAW mode programs.

## Consequences

### Positive

- Interactive cursor verification is now automated
- mg cursor keys work correctly on AmigaOS
- Trap mechanism is extensible (future modes can read character maps, window state)
- ScreenRead follows the KeyInject pattern (familiar to contributors)
- Host-side key injection (osascript) reliably delivers to RAW mode programs
- Status line scraping provides cursor verification without ConUnit access
- Sentinel handshake gives clean host/guest synchronization

### Negative

- Requires forked FS-UAE (not stock)
- Cross-repo dependency (FS-UAE + amiport)
- unit_logs[] selection assumes highest-address unit is the test target
- Host-side injection is macOS-specific (osascript)
- EXPECT_TRAP_CURSOR only works for COOKED mode programs

### Neutral

- EXPECT_TRAP_CURSOR and EXPECT_CURSOR coexist with different data sources
- ScreenRead is test-only infrastructure, not shipped to users
- Two key injection paths: KeyInject (functional) vs inject-keys.sh (visual)
