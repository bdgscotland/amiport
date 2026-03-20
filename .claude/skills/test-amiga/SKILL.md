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
5. **Compare with expected output** — run the native version and diff
6. **Report results** — pass/fail with details

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
```

## Output Comparison

For CLI utilities, compare output against the native version:

```bash
# Run native version
cat testfile | wc > expected.txt

# Run Amiga version
vamos ./wc testfile > actual.txt

# Compare
diff expected.txt actual.txt
```

## Reporting

```
# Test Results: <program>

## Environment
- vamos version: X.Y.Z
- Target: AmigaOS 3.x / 68020

## Tests
| Test Case | Input | Expected | Actual | Status |
|-----------|-------|----------|--------|--------|
| ...       | ...   | ...      | ...    | PASS/FAIL |

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
