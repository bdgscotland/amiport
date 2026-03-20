# ADR-006: m68000 Target for vamos Compatibility

## Status

Accepted (amends [ADR-003](003-amigaos-3x-primary-target.md))

## Date

2026-03-20

## Context

ADR-003 specified 68020+ as the default target CPU. During smoke testing, we discovered that the development branch of vamos (amitools 0.8.x + machine68k 0.4.x) has a regression with m68020-compiled binaries — they trigger F-line emulator exceptions (exception 0x0b) on instructions the CPU emulator can't decode.

Binaries compiled with `-m68000` run correctly in the same vamos version.

## Decision

Switch the default compile target from `-m68020` to `-m68000` across all Makefiles (posix-shim, examples, ports, tests). This ensures all binaries work in vamos for automated testing.

The generated binaries are also compatible with a wider range of Amiga hardware (A500, A600, A1200 without 68020 accelerator).

## Consequences

### Positive

- All binaries work in vamos for automated testing
- Wider hardware compatibility (any Amiga with 68000+)
- No functional impact for CLI utilities (68020 optimizations are negligible for text processing)

### Negative

- Loses 68020 instruction set optimizations (marginal for our use case)
- If vamos fixes the regression, we could switch back — but m68000 compatibility is arguably better anyway
- Code using `LONG __stack` or Amiga types is unaffected (these are ABI conventions, not instruction set features)
