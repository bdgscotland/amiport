Paths: ports/**/*.c, ports/**/*.h

# Mandatory FS-UAE Testing After Code Changes

**Any change to ported source code (`ports/*/ported/*.c` or `ports/*/ported/*.h`) MUST be followed by FS-UAE testing before the work is considered complete.**

## The Rule

After editing any file in `ports/<name>/ported/`, you MUST:

1. **Rebuild**: dispatch the `build-manager` agent
2. **vamos smoke test**: `make test` from the port directory (with appropriate `VAMOS_STACK`)
3. **FS-UAE full test**: `make test-fsemu TARGET=ports/<name>` from the project root
4. **All tests must pass**: 0 failures in the TAP output

Do NOT skip step 3. vamos does not simulate real AmigaOS behavior — programs that pass on vamos can crash on real hardware (crash-patterns #7, #10). The FS-UAE test is the only way to verify correctness on real AmigaOS.

## When FS-UAE Tests Fail

- **Timeout (no results)**: Likely a Guru Meditation. Re-run with `--debug` flag to capture Enforcer data. Check crash-patterns.md for the error code.
- **Test hung at test N**: Check `test-fsemu-cases.txt` for the hung command. Most common cause: stack overflow (crash-patterns #7).
- **Wrong output**: Compare expected vs actual in the TAP output. May need to fix the test expectation OR the ported code.
- **RC mismatch**: Check if err()/errx() output goes to stderr (not captured by test harness). Error path tests should use EXPECT_RC: only.

## Stack Size

The test-runner.rexx harness sets `Stack 262144` before every command. This matches the `__stack` cookie value for most ports. If a port needs more, update both the `__stack` value in the source AND the `VAMOS_STACK` in the Makefile.

## Test Coverage

The test-fsemu-cases.txt MUST test **every single flag** the program accepts. Extract the OPTIONS string from getopt() and verify each flag has at least one test case. See `docs/test-coverage-standard.md`.

## Interactive Testing (Category 3+ MANDATORY)

For **Category 3 (Console UI) and Category 4 (Network)** ports, add **automated interactive tests** using `ITEST:` blocks in `test-fsemu-cases.txt`. These use **KeyInject** (`toolchain/keyinject/`) to inject keystrokes via `commodities.library/AddIEvents()`. See ADR-023.

### Automated Interactive Tests (ITEST: blocks)

Add `ITEST:` blocks to `test-fsemu-cases.txt` for automated keystroke injection:

```
ITEST: Interactive quit with q key
LAUNCH: WORK:less WORK:test-scroll.txt
KEYS: WAIT1500,q
EXPECT_RC: 0

ITEST: Interactive search then quit
LAUNCH: WORK:less WORK:test-scroll.txt
KEYS: WAIT2000,/,WAIT500,F,I,N,D,RETURN,WAIT1000,q
EXPECT_RC: 0
```

**KEYS token types:**
- Named keys: `SPACE`, `RETURN`, `ESC`, `TAB`, `BACKSPACE`, `DELETE`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `F1`-`F10`, `HELP`
- Modifier+key: `CTRL_<key>` (hold Ctrl), `ALT_<key>` (hold Alt/Meta) — e.g., `CTRL_X`, `CTRL_C`, `ALT_g`, `CTRL_SLASH`; `<key>` is any single char or named key
- Single characters: `a`-`z`, `0`-`9`, `/`, `.`, `-`, etc.
- Delays: `WAIT<ms>` (e.g., `WAIT500` = 500ms)

**Rules for ITEST: blocks:**
- Do NOT use `SAY` in the test harness during interactive tests — it contaminates the shared console
- Include `WAIT1500` or more at the start of KEYS to let the program initialize
- The test harness waits 3s for init, runs KeyInject, waits 3s for exit, then force-kills if needed
- Interactive tests are skipped on vamos (KeyInject requires AmigaOS)
- FS-UAE config must include `fast_memory = 8192` (multiple CLIs exhaust 2MB Chip RAM)

### Visual Verification Tests — Separate Pass (ADR-024)

**Functional and visual tests MUST be separate FS-UAE passes.** Never mix them in one suite. Resource exhaustion at ~13 ITESTs is a hard wall, and visual tests require the forked FS-UAE with ANSI capture support.

**Test file split:**
- `test-fsemu-cases.txt` — non-interactive `TEST:` blocks + functional `ITEST:` blocks (exit code verification)
- `test-fsemu-visual-cases.txt` — `SCRAPE` visual verification tests only

**Running visual tests:**
```bash
make test-fsemu TARGET=ports/<name>           # Functional pass (default)
make test-fsemu TARGET=ports/<name> VISUAL=1  # Visual pass (--visual flag)
```

The `--visual` flag tells `scripts/test-fsemu.sh` to use `test-fsemu-visual-cases.txt` instead and enables ANSI capture via the forked FS-UAE (`~/Developer/fs-uae/`). Host-side `scripts/verify-screen.py` uses pyte to reconstruct the terminal screen from captured ANSI output.

**SCRAPE test format** (in `test-fsemu-visual-cases.txt`):
```
ITEST: Visual: file content appears on screen
LAUNCH: WORK:mg -n WORK:test-file.txt
KEYS: WAIT2000,CTRL_X,WAIT300,CTRL_C
SCRAPE
EXPECT_AT 1,1,Hello, Amiga world!
EXPECT_RC: 0
```

**Current limitation:** `CMD_WRITE` captures static display (file load, help text) but NOT interactive echo (typed characters, cursor movement). For RAW mode programs (mg, less, nano), ConUnit cursor fields stay at (0,0) -- use status line scraping via EXPECT_AT instead.

**Cursor position verification (ADR-025):**

For COOKED mode programs, use `EXPECT_TRAP_CURSOR` with ScreenRead:
```
ITEST: Visual: cursor moves after keystroke
LAUNCH: WORK:program WORK:test-file.txt
KEYS: WAIT2000,DOWN,WAIT1000
SCRAPE
SCREEN_READ
EXPECT_TRAP_CURSOR 2,1
CLEANUP: CTRL_X,WAIT300,CTRL_C
EXPECT_RC: 0
```

For RAW mode programs (mg, less, nano), verify cursor via the program's status line:
```
ITEST: Visual: cursor at line 2 after DOWN
LAUNCH: WORK:mg -n WORK:test-file.txt
KEYS: WAIT2000,DOWN,WAIT1000
SCRAPE
EXPECT_AT 29,28,2:1
CLEANUP: CTRL_X,WAIT300,CTRL_C
EXPECT_RC: 0
```

`SCREEN_READ` triggers the ScreenRead binary to dump ConUnit cursor position via FS-UAE trap. `EXPECT_TRAP_CURSOR row,col` reads the .screen JSON. This is authoritative for COOKED mode interactive cursor operations. For RAW mode, ConUnit stays at (0,0) -- use EXPECT_AT on the program's own status line instead.

### Host-Side Key Injection for Visual Tests (ADR-025)

Visual ITEST blocks use **host-side key injection** via `scripts/inject-keys.sh` instead of Amiga-side KeyInject. This is necessary because `AddIEvents()` does not reliably deliver keystrokes to RAW mode programs in visual test passes.

**How it works:**
1. The ARexx harness writes a sentinel file `T:keys-request-N` containing the KEYS token sequence
2. The host-side `test-fsemu.sh` detects the sentinel and calls `inject-keys.sh <pid> <keys>`
3. `inject-keys.sh` builds a single `osascript` (macOS System Events) call with all keystrokes batched (~1.7s total overhead vs ~5s per individual keystroke)
4. After injection completes, the host writes `T:keys-done-N` to signal the harness
5. The ARexx harness polls for `keys-done-N` before proceeding

**CLEANUP directive:** Visual tests use `CLEANUP: <keys>` to send quit keystrokes AFTER SCRAPE capture completes. This ensures the program's display is stable when captured, then the program is cleanly exited. Example: `CLEANUP: CTRL_X,WAIT300,CTRL_C` for mg.

**Key injection rule of thumb:**
- **Functional ITESTs** (test-fsemu-cases.txt): Amiga-side KeyInject via AddIEvents() -- works reliably for exit code verification
- **Visual ITESTs** (test-fsemu-visual-cases.txt): Host-side inject-keys.sh via osascript -- required for RAW mode programs

### Manual Interactive Verification (supplemental)

Manual testing remains useful for **visual aspects that automated SCRAPE tests cannot yet verify** — interactive echo, cursor movement during typing, and complex screen transitions (ADR-025 scope).

**How to run manual tests:**

1. Install the port binary: `make install-emu`
2. Launch FS-UAE: `make emu`
3. In the Amiga Shell: `WORK:<program> WORK:test-file.txt`
4. Work through the checklist, document in PORT.md under "Interactive Test Results"

**Interactive test checklist for console pagers (less, more):**
- [ ] SPACE scrolls forward one screen
- [ ] b scrolls backward
- [ ] /pattern + ENTER searches forward
- [ ] q quits cleanly (shell prompt returns, no garbage on screen)
- [ ] Status line (:) appears at bottom of window
- [ ] Backspace works during search input (not ^H)
- [ ] Screen clears properly on start and restores on quit

**Interactive test checklist for console editors (nano, vim, mg):**
- [ ] Cursor keys move cursor
- [ ] Text input appears at cursor position
- [ ] Save and quit work
- [ ] Screen redraws correctly after window operations
