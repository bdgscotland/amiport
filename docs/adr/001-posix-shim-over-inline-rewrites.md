# ADR-001: POSIX Shim Library Over Inline Rewrites

## Status

Accepted

## Date

2026-03-19

## Context

When porting POSIX C code to AmigaOS, every `open()`, `read()`, `opendir()`, etc. needs to be replaced with AmigaOS equivalents (`Open()`, `Read()`, `Lock()`/`Examine()`). There are two approaches:

1. **Inline rewrite**: Replace each call site with the equivalent AmigaDOS code directly
2. **Shim library**: Provide wrapper functions (`amiport_open()`, etc.) that handle the translation, and only change the call sites to use the wrappers

## Decision

Use a POSIX shim library (`lib/posix-shim/`). Claude's source transformations swap `#include` headers and rename function calls to `amiport_*` wrappers rather than inlining complex AmigaDOS code at every call site.

## Consequences

### Positive

- Ported code is cleaner and more readable
- Transformations are simpler and more reliable (header swap + function rename vs. complex inline rewrite)
- Shim bugs can be fixed in one place, benefiting all ports
- Easier to support multiple AmigaOS versions via `#ifdef` in the shim

### Negative

- Extra library dependency for ported programs
- Small runtime overhead from wrapper function calls
- Shim must be maintained and tested independently
