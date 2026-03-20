# ADR-012: Post-Porting Quality Gates

## Status
Accepted

## Date
2026-03-20

## Context
The porting pipeline (analyze → transform → build → test) produces working Amiga binaries, but "compiles and runs" is not sufficient for quality. Amiga-specific issues — stack overflow on 68000, BPTR leaks, CHIP RAM waste, non-native path conventions — can't be caught by the cross-compiler or vamos tests. These require domain-specific review.

Similarly, ported code often contains performance anti-patterns that are invisible on modern hardware but devastating on a 7MHz 68000: strlen() in loop conditions, unnecessary division, poor memory access patterns.

## Decision
Add two post-test quality tools to the pipeline:

### /review-amiga (skill — recommended for all ports)
Amiga-specific code review covering: stack safety, BPTR handling, memory patterns, path conventions, 68k architecture issues, and AmigaOS conventions. Produces an Amiga Readiness Score. Should be run before packaging/publishing.

### perf-optimizer (agent — optional, for performance-critical ports)
68k performance specialist dispatched when a port works but runs slowly. Knows instruction timing for 68000-68060, CHIP/FAST RAM characteristics, cache behavior, and DMA contention. Suggests targeted optimizations with estimated impact.

### When to use each
- `/review-amiga`: Run on every port before publishing to Aminet. Mandatory for ports that will be distributed.
- `perf-optimizer`: Dispatch when users report slowness, or for ports of programs that process significant data (grep, sed, sort).

## Consequences

### Positive
- Catches Amiga-specific bugs that the cross-compiler and vamos miss
- Performance optimization guided by actual hardware knowledge
- Quality bar for Aminet publications

### Negative
- Additional step in the pipeline (but optional/recommended, not blocking)
- perf-optimizer suggestions may not be testable in vamos (need real hardware)
