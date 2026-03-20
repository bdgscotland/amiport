# ADR-002: Docker-First Cross-Compilation Toolchain

## Status

Accepted

## Date

2026-03-19

## Context

Amiga cross-compilers (bebbo's amiga-gcc, VBCC) are notoriously difficult to build from source, with platform-specific quirks and dependency chains. Users need a reproducible way to compile for Amiga targets.

## Decision

Use Docker containers as the primary toolchain delivery mechanism. Provide Dockerfiles for both bebbo-gcc and VBCC. Native installation is supported but not the default path.

## Consequences

### Positive

- Reproducible builds across all host platforms (macOS, Linux, Windows via WSL)
- No host system pollution from cross-compiler installation
- Easy CI/CD integration
- Version pinning of toolchain components

### Negative

- Requires Docker installed on the host
- Larger disk footprint than native installation
- Slight build speed penalty from container overhead
