# ADR-023: Automated Interactive Testing for Category 3+ Ports

## Status

Proposed

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

**TBD** — This ADR is proposed for discussion. Recommended starting point: **Approach B (FS-UAE Net Protocol)** for immediate pragmatic value, with **Approach D (Console.device Driver)** as the long-term goal for real-hardware testing.

The approaches are not mutually exclusive — B gives us automated testing on the development machine now, D gives us testing on real hardware later.

## Implementation Plan (Approach B)

1. Research FS-UAE net protocol capabilities (keyboard event injection, screenshot)
2. Add `--net-protocol` flag to `test-fsemu.sh` for interactive test mode
3. Define interactive test case format extending `test-fsemu-cases.txt`:
   ```
   TEST_INTERACTIVE: scroll and search
   LAUNCH: WORK:less WORK:test-interactive.txt
   WAIT: 2000
   SEND_KEY: SPACE
   WAIT: 500
   SCREENSHOT: after-scroll.png
   SEND_KEY: /
   SEND_KEYS: FINDME
   SEND_KEY: RETURN
   WAIT: 500
   SCREENSHOT: after-search.png
   SEND_KEY: q
   WAIT: 1000
   EXPECT_EXIT: 0
   ```
4. Build screenshot comparison tooling (pixel diff with tolerance)
5. Integrate into `make test-fsemu` pipeline

## Implementation Plan (Approach D — Long-term)

1. Write `toolchain/test-console-driver/driver.c` (~200 lines)
2. Uses `OpenWindow()` + console.device to create a controlled console
3. Launches test subject via `SystemTags()` with console I/O redirected
4. ARexx sends keystroke injection commands to the driver
5. Driver captures console.device output buffer and writes to file
6. Test harness compares captured output against expected

## Consequences

### Positive
- Category 3+ ports can be tested automatically
- Regression testing catches interactive bugs before shipping
- Reduces manual testing burden as port catalog grows
- Enables CI for console programs

### Negative
- Approach B adds FS-UAE version dependency (net protocol support varies)
- Screenshot comparison adds maintenance burden (reference images need updating)
- Approach D requires significant implementation effort
- Interactive test timing is inherently less deterministic than CLI tests

### Neutral
- Non-interactive FS-UAE tests (current `test-fsemu-cases.txt`) remain the primary test mechanism
- Interactive tests supplement but don't replace the existing framework
- Manual interactive verification remains valid for one-off checks
