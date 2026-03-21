---
name: debug-amiga
description: Debug a crashed Amiga port using Enforcer hit capture and autonomous fix loop. Runs test-fsemu in debug mode, captures crashes, and dispatches the debug-agent to fix them. Use after a port builds but crashes at runtime.
allowed-tools: Bash, Read, Write, Edit, Grep, Glob
---

# Debug Amiga Port

You are debugging a compiled Amiga port that crashes at runtime. You run the port under Enforcer (via FS-UAE debug mode), capture crash data, and dispatch the debug-agent to autonomously fix the crashes.

## Usage

```
/debug-amiga <port-directory>
```

Example: `/debug-amiga ports/grep`

## Prerequisites

- Compiled Amiga binary in the port directory (run `/build-amiga` first if needed)
- FS-UAE configured with AmigaOS 3.x installation
- Debug tools installed (`make setup-debug-tools`)
- `scripts/test-fsemu.sh` and `scripts/debug-report.py` available

## Process

### Step 1: Validate the Port Directory

Verify the port directory exists and contains the expected structure:

```bash
# Check port directory exists
ls <port-directory>/

# Check compiled binary exists
ls <port-directory>/$(basename <port-directory>)

# Check ported source exists
ls <port-directory>/ported/
```

If the binary doesn't exist, tell the user to build first (`/build-amiga <port-directory>`) and stop.

### Step 2: Run Debug Test

Run FS-UAE with Enforcer, Mungwall, and SegTracker enabled:

```bash
scripts/test-fsemu.sh --debug <port-directory>
```

This will:
1. Start serial-capture.py on an ephemeral port
2. Boot FS-UAE with 68030 CPU (MMU for Enforcer), serial capture, and debug tools in Startup-Sequence
3. Run the port's test harness under Enforcer monitoring
4. Capture all Enforcer hits to `enforcer.log`
5. Exit after tests complete (or 90-second timeout)

### Step 3: Check for Crashes

After FS-UAE exits, check if Enforcer captured any hits:

```bash
# Parse Enforcer output into structured JSON
python3 scripts/debug-report.py parse <port-directory>/enforcer.log > <port-directory>/crashes.json

# Map crash addresses to source lines
python3 scripts/debug-report.py map <port-directory>/crashes.json <port-directory>/$(basename <port-directory>) > <port-directory>/mapped-crashes.json
```

**If no crashes found:** Report the port is clean and exit. Example output:

```
# Debug Results: <program>

## Enforcer Test: CLEAN
No Enforcer hits detected. The port runs without illegal memory access.
```

**If crashes found:** Proceed to Step 4.

### Step 4: Dispatch the Debug Agent

The debug-agent receives the mapped crash data and enters the autonomous fix loop. Provide it with:

- The port directory path
- The mapped crash JSON (source file, line number, crash type, registers)
- The port name for build/test commands

The debug-agent will:
1. Classify each crash (obvious vs. needs investigation)
2. Check the crash patterns KB (`docs/references/crash-patterns.md`)
3. Apply fixes with git stash rollback safety
4. Rebuild via `make build TARGET=<port-directory>`
5. Retest via `scripts/test-fsemu.sh --debug <port-directory>`
6. Loop up to 5 iterations

### Step 5: Report Results

After the debug-agent completes (or exhausts its iteration budget), report:

```
# Debug Results: <program>

## Crashes Found
| # | Type | Address | Source Location | Classification |
|---|------|---------|-----------------|----------------|
| 1 | LONG-READ | 0x00000014 | ported/main.c:47 | Obvious (NULL deref) |

## Fixes Applied
| Iteration | Crash | Fix | Result |
|-----------|-------|-----|--------|
| 1 | NULL deref at main.c:47 | Added NULL check after find_node() | FIXED — 0 Enforcer hits |

## Summary
- Total crashes found: N
- Crashes fixed: N
- Iterations used: N/5
- Final status: CLEAN / UNRESOLVED (N remaining)
```

## Hard Crash Detection

If FS-UAE exits without UAEQuit (no clean exit signal), the enforcer.log is empty, and TAP results show no test completion, this is a **hard crash** (Guru Meditation). Report this to the user:

```
# Debug Results: <program>

## Hard Crash Detected
FS-UAE exited abnormally with no Enforcer output and no test completion.
This indicates a Guru Meditation (illegal instruction, bus error) that
crashed the system before Enforcer could report.

The debug-agent can attempt printf breadcrumb diagnosis — this inserts
checkpoint prints at function entry points to narrow the crash location.
```

The debug-agent handles hard crashes by inserting `fprintf(stderr, "CHECKPOINT: function_name\n")` statements, which are captured via the serial port.

## Limitations

- **Layer 1 only:** Currently uses Enforcer-based passive crash detection. Layer 2 (GDB via bgdbserver) is deferred pending TCP routing verification.
- **Source mapping requires -gstabs:** Binaries must be compiled with STABS debug info. If addr2line returns `??:0`, the binary needs rebuilding with `-gstabs`.
- **Multi-hunk binaries require .map file:** Address mapping for multi-hunk programs needs a linker map (`-Wl,-Map`). Without it, debug-report.py will error rather than guess wrong.
- **68030 CPU in debug mode:** Debug test runs use 68030 (for MMU/Enforcer), which differs from normal test runs (68020). The 68030 data cache could theoretically mask or expose timing-sensitive bugs.
