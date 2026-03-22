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
