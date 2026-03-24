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
- Single characters: `a`-`z`, `0`-`9`, `/`, `.`, `-`, etc.
- Delays: `WAIT<ms>` (e.g., `WAIT500` = 500ms)

**Rules for ITEST: blocks:**
- Do NOT use `SAY` in the test harness during interactive tests — it contaminates the shared console
- Include `WAIT1500` or more at the start of KEYS to let the program initialize
- The test harness waits 3s for init, runs KeyInject, waits 3s for exit, then force-kills if needed
- Interactive tests are skipped on vamos (KeyInject requires AmigaOS)
- FS-UAE config must include `fast_memory = 8192` (multiple CLIs exhaust 2MB Chip RAM)

### Manual Interactive Verification (supplemental)

Manual testing remains useful for **visual verification** (screen rendering, cursor positioning, display artifacts) that automated tests cannot verify yet (Phase 2 of ADR-023).

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
