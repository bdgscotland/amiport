---
name: test-amiga
description: Test compiled Amiga binaries using vamos (virtual AmigaOS runtime). Verifies correct execution and output comparison. Use after a successful build.
allowed-tools: Bash, Read, Write
---

# Test Amiga Binary

You are testing a compiled Amiga binary using vamos from the amitools package.

## Prerequisites

- `vamos` installed (`pip install amitools`)
- Compiled Amiga binary from the build stage

## Process

1. **Verify vamos is available** — `which vamos`
2. **Prepare test inputs** — create test files if needed
3. **Run the binary** — `vamos <binary> [args]`
4. **Capture output** — stdout and stderr
5. **Check the exit code** — vamos passes through the Amiga return code
6. **Compare with expected output** — run the native version and diff
7. **Report results** — pass/fail with details

## Running vamos

```bash
# Basic execution
vamos ./program

# With arguments
vamos ./program -arg1 -arg2 inputfile

# With Amiga volume assignments
vamos -V "Work:$(pwd)" ./program

# Capture output for comparison
vamos ./program > amiga_output.txt 2>&1

# With a timeout (prevents hangs from blocking on stdin or infinite loops)
timeout 30 vamos ./program

# Feeding stdin
echo "input data" | vamos ./program
# Or from file:
vamos ./program < input.txt
```

## Exit Codes

AmigaOS uses different exit codes than POSIX. vamos passes these through:

| Amiga Constant | Value | Meaning | POSIX Equivalent |
|----------------|-------|---------|------------------|
| `RETURN_OK`    | 0     | Success | `EXIT_SUCCESS` (0) |
| `RETURN_WARN`  | 5     | Warning (non-fatal) | No direct equivalent |
| `RETURN_ERROR` | 10    | Error | `EXIT_FAILURE` (1) |
| `RETURN_FAIL`  | 20    | Fatal error | `EXIT_FAILURE` (1) |

When comparing exit codes:
- Code 0 = success (same as POSIX)
- Code 5 = warning — may be acceptable, check if the native version would also warn
- Code 10 or 20 = something went wrong, investigate
- If the native version returns 1 (failure), the Amiga version may return 10 or 20 — this is OK as long as the program correctly detected the error condition

## Error Handling

### vamos crashes vs program errors
- **vamos itself crashes** (segfault, Python traceback): This is an emulator issue, not a bug in your port. Try running with `vamos -v` for verbose output. Common causes: unsupported library call, bad memory access at address 0.
- **Program returns non-zero exit code**: This is your program reporting an error. Check stderr output.
- **Program hangs (no output, doesn't exit)**: The program is likely blocking on stdin or stuck in a loop. Use `timeout` to detect this:
  ```bash
  timeout 10 vamos ./program
  # Exit code 124 means timeout was reached
  ```

### Common vamos issues
- **"Unknown library"**: The program calls an Amiga library that vamos doesn't emulate. Check which library and whether the code can be stubbed.
- **"Invalid memory access"**: Usually a null pointer dereference or BPTR misuse. Debug by checking the address — `0x00000000` usually means a NULL was returned and not checked.
- **Garbled output encoding**: vamos expects ISO-8859-1 (Amiga native). If the source outputs UTF-8, characters above 127 will look wrong — this is expected and not a bug.

### Programs that need stdin
Some programs (like `cat`, `wc` without file args) read from stdin and will hang in vamos if not given input:
```bash
# Provide input via pipe
echo "test input" | vamos ./program

# Or redirect from file
vamos ./program < testfile.txt

# Or use timeout to detect the hang
timeout 5 vamos ./program
```

## Output Comparison

For CLI utilities, compare output against the native version:

```bash
# Run native version
./program_native testfile > expected.txt 2>&1
native_exit=$?

# Run Amiga version
vamos ./program testfile > actual.txt 2>&1
amiga_exit=$?

# Compare output
diff expected.txt actual.txt

# Compare exit codes (accounting for Amiga conventions)
echo "Native exit: $native_exit, Amiga exit: $amiga_exit"
```

When comparing, allow for these expected differences:
- Trailing whitespace variations
- Path separators in error messages (the Amiga version may show Amiga-style paths)
- Slightly different error message wording from shim functions

## Reporting

```
# Test Results: <program>

## Environment
- vamos version: X.Y.Z
- Target: AmigaOS 3.x / 68020

## Tests
| Test Case | Input | Expected | Actual | Exit Code | Status |
|-----------|-------|----------|--------|-----------|--------|
| ...       | ...   | ...      | ...    | ...       | PASS/FAIL |

## Summary
- Tests passed: N/M
- Overall: PASS/FAIL
```

## Limitations

vamos supports:
- exec.library (basic task management)
- dos.library (file I/O, directory ops, environment)
- utility.library

vamos does NOT support:
- intuition.library (GUI)
- graphics.library (rendering)
- audio.device
- Most hardware-related libraries

If the program requires unsupported libraries, note this and suggest FS-UAE testing instead.
