# ADR-014: FS-UAE Automated Testing Pipeline

## Status

Accepted

## Date

2026-03-21

## Context

Category 3-4 ports (console UI apps, network apps) cannot be tested with vamos because they require a full AmigaOS environment with console.device, Intuition, or a TCP/IP stack. Previously, testing these ports required manual interactive sessions in FS-UAE — slow, unrepeatable, and impossible to automate.

We needed an automated testing pipeline that:
- Boots a real AmigaOS 3.x environment in FS-UAE
- Executes test cases without human interaction
- Captures results in a machine-readable format
- Terminates the emulator cleanly when done
- Handles crashes/hangs with a timeout watchdog

Two approaches were considered:
1. **Startup-Sequence + UAEQuit** — boot AmigaOS, run an ARexx test harness from the Startup-Sequence, write results to a shared volume, then UAEQuit terminates FS-UAE
2. **Serial console** — connect to FS-UAE's serial port over TCP, send commands, parse output

## Decision

We chose the **Startup-Sequence + UAEQuit** approach. The serial console approach was deferred as a future debug mode (see TODOS.md).

### Architecture

1. **`make test-fsemu TARGET=ports/<name>`** invokes `scripts/test-fsemu.sh`
2. The script generates a custom Startup-Sequence that:
   - Mounts the port directory as a shared volume
   - Launches the ARexx test harness (`test-runner.rexx`)
   - Runs UAEQuit to signal FS-UAE to exit
3. **ARexx test harness** (`toolchain/templates/test-runner.rexx`) reads test case definitions from `test-fsemu-cases.txt` in the port directory and:
   - Executes each test case (command + expected output/exit code)
   - Writes TAP (Test Anything Protocol) formatted results to the RESULTS: volume
4. **UAEQuit** is a small Amiga binary (`toolchain/uaequit/`) that sends the UAEQ trap instruction to signal FS-UAE to exit gracefully
5. **Shared volumes** — the port directory is mounted as a volume so results can be read by the host after FS-UAE exits
6. **Watchdog** — the host script has a configurable timeout; if FS-UAE doesn't exit within it, the process is killed and the test is marked as timed out
7. **TEST-REPORT.md** is generated from the TAP output after FS-UAE exits

### Test case format (`test-fsemu-cases.txt`)

Each test is a block of 3 lines. Two assertion modes are supported:

```
TEST: description of test
CMD: WORK:program args WORK:inputfile.txt
EXPECT: expected first-line output (exact match)

TEST: another test
CMD: WORK:program -u WORK:input.txt
EXPECT_CONTAINS: substring to find in output
```

- `EXPECT:` — exact match of the first line of stdout
- `EXPECT_CONTAINS:` — substring match (useful for multi-line output like diff)
- Empty `EXPECT:` matches empty output (e.g., identical files produce no output)
- Input files must be pre-created (no piping in ARexx ADDRESS COMMAND)

### Limitations

- FS-UAE 3.x runs with a window (true headless requires FS-UAE 4.0+, not yet released)
- No interactive input testing (ARexx can only drive non-interactive commands)
- Network tests require AmiTCP or Roadshow configured in the FS-UAE environment

## Consequences

### Positive

- Category 3-4 ports can be tested automatically, not just manually
- TAP output integrates with standard test tooling
- TEST-REPORT.md provides human-readable results alongside the port
- UAEQuit approach is reliable — no serial parsing fragility
- Same ARexx harness template works across all ports

### Negative

- Requires FS-UAE installed and a Kickstart ROM available
- Slower than vamos testing (full AmigaOS boot per test run)
- FS-UAE 3.x opens a visible window during tests (cannot run truly headless)

### Neutral

- Serial console debug mode deferred to future work (see TODOS.md)
- ARexx is the standard AmigaOS scripting language — appropriate choice for an Amiga test harness
