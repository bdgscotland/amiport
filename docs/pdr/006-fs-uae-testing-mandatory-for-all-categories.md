# PDR-006: FS-UAE Testing Mandatory for All Port Categories

## Status

Accepted

## Date

2026-03-21

## Problem

The original testing strategy (ADR-004, ADR-014) used vamos for Category 1-2 ports (CLI tools, scripting interpreters) and FS-UAE only for Category 3-4 ports (console UI, network apps). This seemed reasonable: vamos is fast (~1 second) and sufficient for testing stdin/stdout/exit-code behavior.

In practice, ports that passed vamos testing failed on real AmigaOS:
- **libnix `exit()` hangs** — `exit()` calls atexit handlers that hang when stdio is connected to a console window. Only visible in FS-UAE or real hardware, never in vamos (which has no console).
- **Environment variable behavior** — vamos simulates `getenv()` from host environment. Real AmigaOS uses `ENV:` and `ENVARC:` assigns with different semantics.
- **Stack cookie behavior** — vamos ignores `__stack` cookies and uses a fixed 8KB stack. FS-UAE respects the cookie. Programs that work in vamos with 8KB may crash on real hardware with 4KB default.
- **File system differences** — vamos maps host paths transparently. Real AmigaOS has assign names (S:, T:, LIBS:), volume names, and 30-character filename limits on FFS.

## Target Users

- Port maintainers who need confidence that a port works on real Amiga hardware
- End users who download ports expecting them to work on their Amiga
- The porting pipeline itself, which gates "done" on test success

## Decision

FS-UAE testing (`make test-fsemu TARGET=ports/<name>`) is mandatory for ALL port categories, not just Categories 3-4. The `/port-project` skill includes FS-UAE testing as Stage 5b, after vamos testing (Stage 5a).

- **Stage 5a (vamos)** — fast smoke test, catches compilation errors, basic logic bugs, and exit code issues
- **Stage 5b (FS-UAE)** — full integration test on a real AmigaOS 3.x environment, catches libnix quirks, environment issues, and console behavior

Both stages must pass before a port is considered complete.

## Rationale

- vamos is a useful rapid iteration tool but is NOT a faithful AmigaOS replica — it differs in stack handling, console I/O, environment variables, and file system semantics
- Every port that shipped with vamos-only testing required post-release fixes for issues that FS-UAE would have caught
- The cost is ~30 seconds per FS-UAE test run (vs ~1 second for vamos) — acceptable given it prevents shipping broken ports
- FS-UAE testing infrastructure already exists (ADR-014) — this is a policy change, not a new capability

## Success Criteria

- Zero ports ship with libnix `exit()` hangs or console I/O issues
- `/port-project` refuses to mark a port complete without Stage 5b passing
- All ports in `ports/` have both vamos and FS-UAE test results in their PORT.md

## Alternatives Considered

- **FS-UAE only for Category 3+**: The previous policy. Failed because Category 1-2 ports still had environment and libnix issues only visible in FS-UAE.
- **FS-UAE only when vamos finds issues**: Reactive rather than proactive. The bugs FS-UAE catches are precisely the ones vamos CANNOT find — waiting for vamos to flag them means waiting forever.
- **Real hardware testing**: Ideal but impractical for CI. FS-UAE with a configured AmigaOS 3.x HDF is the closest practical approximation.
