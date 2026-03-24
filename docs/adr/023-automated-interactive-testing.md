# ADR-023: Automated Interactive Testing for Category 3+ Ports

## Status

**Accepted**

## Date

2026-03-24

## Context

Category 3 (Console UI) and Category 4 (Network) ports require interactive testing that our current FS-UAE test harness cannot automate. The harness uses ARexx `ADDRESS COMMAND` which runs programs non-interactively — it can pass arguments and capture stdout, but cannot send keystrokes mid-execution.

This was discovered during the less 692 port (first Category 3 port). The automated test suite (20/20 pass) uses `-X -F -E` flags to make less non-interactive, but this doesn't test:
- Screen rendering (cursor positioning, clear screen, scrolling)
- Interactive input (key handling, search, command line editing)
- Terminal state management (raw mode enter/exit, attribute reset)

Three interactive bugs were found only through manual testing:
1. Alternate screen buffer sequences not supported by console.device
2. Window size query failing because console was in cooked mode
3. Backspace showing ^H because termios VERASE wasn't populated

Without automated interactive testing, every Category 3+ port requires manual FS-UAE verification — this doesn't scale as the port catalog grows.

## Candidate Approaches

### A: input.device Keystroke Injection (Amiga-native)

Write an ARexx helper that uses `input.device` `IND_WRITEEVENT` to inject keyboard events at the AmigaOS system level. The test harness:

1. Launches the program in the background via `Run`
2. Waits for the program to initialize (fixed delay or sentinel output)
3. Injects keystrokes via `input.device` from the ARexx test script
4. Waits between keystrokes for screen updates
5. Captures final screen state (read from console window or redirect)
6. Sends 'q' to quit, verifies clean exit

**Pros:** Pure Amiga-native, works on real hardware, no emulator dependency.
**Cons:** Requires opening input.device from ARexx (complex), keystroke timing is fragile, no direct screen content capture API, injected keys go to the frontmost window (race condition if another window activates).

### B: FS-UAE Net Protocol (Host-side)

FS-UAE has a network control protocol for remote automation. The host-side `test-fsemu.sh` script:

1. Launches FS-UAE with net protocol enabled
2. Waits for program to start (monitor serial output or timeout)
3. Sends keyboard events via the FS-UAE net protocol from the host
4. Takes screenshots at key points via FS-UAE screenshot command
5. Compares screenshots against reference images (pixel diff or OCR)
6. Sends quit key, verifies clean exit

**Pros:** No Amiga-side changes needed, host-side tooling in Python/bash, screenshot-based verification is thorough.
**Cons:** FS-UAE net protocol may not be fully documented, screenshot comparison is brittle (font rendering differences), adds host-side complexity, doesn't work on real hardware.

### C: Named PIPE: Device (Amiga-side input redirect)

Modify the console-shim or ported programs to accept `PIPE:test-input` as an alternative to `"*"` for tty input. The ARexx harness:

1. Opens `PIPE:less-input` for writing
2. Launches `less` with tty redirected to `PIPE:less-input`
3. Writes keystroke sequences to the pipe with delays
4. Captures stdout to verify output
5. Closes pipe (triggers EOF/quit)

**Pros:** Simple, pure AmigaDOS, works on real hardware.
**Cons:** Requires per-port source changes or a shim-level hook, PIPE: timing is unreliable, no screen content verification (only stdout capture), programs that query terminal size via the pipe will fail.

### D: Console.device Snoop + Raw Event Injection (Hybrid)

Write a small C helper program (`test-console-driver`) that:

1. Opens a CON: window
2. Runs the test subject in that window (via SystemTags)
3. Injects keystrokes via the console.device write side
4. Reads the console.device output to capture screen state
5. Compares against expected screen content

This is the most architecturally sound approach — it gives full control of both input and output on the console.device level.

**Pros:** Full control, works on real hardware, captures actual screen rendering, no timing fragility.
**Cons:** Requires writing a C helper program (~200 lines), needs console.device expertise, most implementation effort.

## Decision

**Approach A — Amiga-native keystroke injection** using `commodities.library/AddIEvents()` + `keymap.library/MapANSI()`.

### Why Not Approach B (FS-UAE Net Protocol)?

Research confirmed that **FS-UAE has no remote control API**. The "net protocol" does not exist. FS-UAE's Lua scripting branch (cnvogelg/fs-uae) is dormant and undocumented, with no keyboard injection support. vAmiga's RetroShell similarly lacks keyboard injection to the emulated Amiga. Host-side injection via osascript/xdotool is fragile (focus management, platform-specific, Accessibility permissions).

### Why Approach A?

1. **Documented by Commodore** — ADCD `Lib_examples/mapansi.c` provides a canonical working example of `MapANSI()` + `AddIEvents()` for keyboard injection
2. **Works on real hardware AND FS-UAE** — no emulator dependencies
3. **Prior art** — FakeKey (Aminet, 1993) proves the approach has worked for 30+ years
4. **Minimal code** — ~200 lines of C89, builds with existing bebbo-gcc toolchain
5. **Clean integration** — ARexx test harness orchestrates, KeyInject injects, no host-side changes

### Implementation

A standalone C89 tool **KeyInject** (`toolchain/keyinject/keyinject.c`) that:

1. Opens `keymap.library` V37 and `commodities.library` V37
2. Allocates an `InputEvent` in `MEMF_PUBLIC` memory
3. Parses a comma-separated key sequence from argv[1]
4. For printable characters: uses `MapANSI()` to convert ASCII to rawkey code + qualifier
5. For special keys (SPACE, RETURN, ESC, cursors): uses direct rawkey codes
6. Injects each key via `AddIEvents()` (key-down then key-up with `IECODE_UP_PREFIX`)
7. Uses `Delay()` from dos.library for timing between keys (50 ticks/sec on PAL)

The ARexx test harness (`test-runner.rexx`) is extended with `ITEST:` blocks:
```
ITEST: Interactive quit with q key
LAUNCH: WORK:less WORK:test-file.txt
KEYS: WAIT1500,q
EXPECT_RC: 0
```

The harness launches the program via `Run`, waits 3 seconds for initialization, invokes `WORK:KeyInject <keys>`, waits 3 seconds for the program to exit, then checks the return code. If KeyInject is not available (e.g., on vamos), interactive tests are skipped with TAP `# SKIP`.

### Phase 2 (Future): Screen Content Verification

Exit code verification confirms the program doesn't crash but doesn't verify screen rendering. Phase 2 will add screenshot-based verification, likely by injecting FS-UAE's screenshot hotkey from the Amiga side via KeyInject, then comparing screenshots on the host.

## Consequences

### Positive
- Category 3+ ports can be tested automatically — `make test-fsemu` runs both non-interactive and interactive tests
- Regression testing catches interactive bugs before shipping
- Reduces manual testing burden as port catalog grows
- Works on real AmigaOS hardware, not just emulators
- Graceful degradation — interactive tests skip on vamos, non-interactive tests unaffected

### Negative
- No screen content verification yet (Phase 2)
- Interactive test timing is less deterministic than CLI tests (mitigated by generous WAIT delays)
- Adds ~30 seconds per interactive test to the test-fsemu run time

### Neutral
- Non-interactive FS-UAE tests remain the primary test mechanism
- Interactive tests supplement but don't replace manual verification for visual bugs
- KeyInject is a general-purpose tool usable beyond the test harness
