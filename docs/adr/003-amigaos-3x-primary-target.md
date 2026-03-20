# ADR-003: AmigaOS 3.x as Primary Target

## Status

Accepted (amended by [ADR-006](006-m68000-target-for-vamos.md) — default CPU changed to 68000)

## Date

2026-03-19

## Context

AmigaOS exists in several major versions (1.3, 2.0, 3.x, 4.x) plus compatible systems (MorphOS, AROS). Each has different API surfaces, capabilities, and community activity levels.

## Decision

Target AmigaOS 3.x on 68020+ as the primary platform. Support for other versions via `#ifdef` in the shim library where practical.

## Consequences

### Positive

- AmigaOS 3.x has the richest classic API (dos.library 37+, full Exec, Intuition, etc.)
- Largest active retro Amiga community uses 3.x
- Best supported by bebbo-gcc and the NDK
- 68020+ allows use of 32-bit addressing, reducing pointer size workarounds

### Negative

- Excludes stock A500/A1000 users (68000 only, typically running 1.3)
- AmigaOS 4.x has much better POSIX support natively — our shim is less needed there
- Must still handle 3.x limitations (no memory protection, cooperative multitasking)
