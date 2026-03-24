# ADR-024: FS-UAE Visual Verification via ANSI Capture

## Status

**Accepted**

## Date

2026-03-24

## Context

ADR-023 (automated interactive testing) solved keystroke injection with KeyInject, but the test harness can only verify exit codes — it cannot assert what appeared on screen. Interactive programs like mg, less, and nano produce ANSI escape sequences that position text, set attributes, and move the cursor. A test that sends C-x C-c and checks RC=0 proves the program didn't crash, but doesn't prove it rendered correctly.

Visual bugs discovered only through manual FS-UAE testing include:
- Screen not clearing on startup (missing CSI 2J)
- Status line rendering at wrong row
- Cursor positioned incorrectly after search
- File content not displayed (silent I/O failure)

The gap: we can inject keystrokes (ADR-023) but cannot verify the resulting screen state.

### Rejected Approaches

**A: AmigaOS-side screen scraping.** console.device's character map is private to the console handler — there is no public API to read characters from screen positions. The ADCD confirms this: CMD_READ reads input, not screen contents. IntuiText/RastPort approaches require knowing the exact font metrics and decoding bitmapped pixels.

**B: OCR on screenshots.** screencapture + Tesseract/OCR is fragile (font-dependent, slow, imprecise coordinates), and introduces a heavy dependency.

**C: FS-UAE headless console mode.** The existing `console_emulation` mode intercepts console.device but replaces the entire display — no graphical window, no ANSI sequences for cursor positioning. It's designed for text-only pipe output, not visual verification.

## Decision

Fork FS-UAE (`bdgscotland/fs-uae`) to add a **console capture mode** that logs raw ANSI output from console.device to a file, then use **pyte** (Python terminal emulator) on the host to reconstruct the screen state for character-level assertions.

### Architecture

```
                    FS-UAE (graphical mode)
                           |
              console.device CMD_WRITE
                           |
                    consolehook.cpp
                     /          \
            real display    ANSI log file
            (unchanged)     (raw bytes)
                               |
                    [host copies at SCRAPE]
                               |
                    verify-screen.py + pyte
                               |
                    EXPECT_AT / EXPECT_CURSOR
```

### Key Design Choices

1. **Passthrough tap, not replacement.** The consolehook intercepts CMD_WRITE but still forwards to the real console.device. The graphical display works normally. This means tests can be run with the FS-UAE window visible for debugging.

2. **Environment variable activation.** Setting `AMIPORT_ANSI_LOG=/path/to/file` enables capture. When unset, FS-UAE behaves identically to stock — zero overhead, no code paths touched.

3. **Cumulative log with point-in-time snapshots.** The ANSI log grows from boot. At SCRAPE time, the host copies the log. pyte processes the entire stream to reconstruct the screen state at that moment. This is correct because terminal state depends on the full history (modes, cursor saves, scrolling regions).

4. **Assertions in test-fsemu-cases.txt.** Visual assertions live alongside the ITEST block they verify — no separate test files. The ARexx harness writes assertion files to RESULTS: for the host to process.

## Implementation

### FS-UAE Patch (consolehook.cpp)

- `console_capture` global flag (alongside existing `console_emulation`)
- `get_ansi_log()` opens log file on first CMD_WRITE (unbuffered)
- `consolehook_activate()` returns true when either flag is set
- `consolehook_ret()` skips console reader thread in capture-only mode
- `consolehook_beginio()` writes raw bytes to log file, then optionally passes through to stdout (emulation mode only)

### Test Case Format

New optional directives for ITEST: blocks in test-fsemu-cases.txt:

```
ITEST: Visual: file content appears on screen
LAUNCH: WORK:mg -n WORK:test-file.txt
KEYS: WAIT2000,CTRL_X,WAIT300,CTRL_C
SCRAPE
EXPECT_AT 1,1,Hello, Amiga world!
EXPECT_AT 2,1,This is line two.
EXPECT_CURSOR 1,1
EXPECT_RC: 0
```

- `SCRAPE` — triggers host-side ANSI log snapshot after KEYS injection
- `EXPECT_AT row,col,text` — asserts text at screen position (1-based)
- `EXPECT_CURSOR row,col` — asserts cursor position (1-based)

### Flow

1. ARexx harness launches program, injects KEYS, waits for screen update
2. If SCRAPE is set, writes `RESULTS:scrape-N.assertions` with assertion lines
3. Writes `RESULTS:scrape-N` sentinel
4. Host watchdog detects sentinel, copies ANSI log to `scrapes/scrape-N.log`
5. Host deletes sentinel, ARexx continues with quit sequence
6. After all tests, host runs `verify-screen.py` for each snapshot
7. Visual pass/fail is included in the test report

### Files

| File | Purpose |
|------|---------|
| `~/Developer/fs-uae/consolehook.cpp` | Patched FS-UAE console hook with ANSI logging |
| `~/Developer/fs-uae/main.cpp` | `console_capture` flag initialization |
| `~/Developer/fs-uae/include/uae.h` | `console_capture` extern declaration |
| `scripts/verify-screen.py` | pyte-based screen reconstruction and assertions |
| `toolchain/templates/test-runner.rexx` | SCRAPE/EXPECT_AT/EXPECT_CURSOR parsing |
| `scripts/test-fsemu.sh` | ANSI log setup, scrape sentinel handling, visual assertion runner |

## Consequences

### Positive

- Character-level screen verification without OCR or pixel matching
- Works in graphical mode — visual debugging still possible
- Zero overhead when AMIPORT_ANSI_LOG is unset
- pyte handles ANSI escape sequences correctly (battle-tested library)
- Assertions are deterministic — no flaky pixel comparisons

### Negative

- Requires forked FS-UAE binary (not stock `brew install fs-uae`)
- Console dimensions must be known (77x30 default from ITEST window)
- Cannot verify color/attribute rendering (pyte supports it, but assertions would be fragile)
- ANSI log includes boot output — first SCRAPE snapshot includes shell prompt noise

### Risks

- AmigaOS console.device may use escape sequences pyte doesn't recognize (unlikely — console.device uses standard ANSI/VT100 subset)
- Multiple console windows (ITEST uses NewShell) may interleave output in the log (mitigated: only one ITEST runs at a time)
