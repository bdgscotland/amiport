# ADR-004: vamos for CLI Testing Over Full Emulation

## Status

Accepted

## Date

2026-03-19

## Context

Testing ported Amiga binaries requires some form of AmigaOS runtime. Options range from full hardware emulation (FS-UAE, WinUAE) to lightweight virtual runtimes (vamos from amitools).

## Decision

Use vamos as the primary testing runtime for CLI programs. Reserve full emulation (FS-UAE) for GUI programs or hardware-dependent code.

## Consequences

### Positive

- vamos is fast, headless, and scriptable — ideal for CI/CD
- No ROM images or Workbench disks required (legal simplicity)
- Direct stdout/stderr capture for output comparison
- Can be installed via pip (`pip install amitools`)

### Negative

- vamos only supports a subset of AmigaOS libraries (exec, dos, utility)
- Cannot test Intuition/graphics/audio programs
- Some AmigaDOS edge cases may behave differently than real hardware
- Not a substitute for real Amiga testing for production-quality ports
