# ADR-005: Pre-Built Docker Images Over Building From Source

## Status

Accepted (amends [ADR-002](002-docker-first-toolchain.md))

## Date

2026-03-20

## Context

ADR-002 specified building the bebbo-gcc cross-compiler from source inside Docker. In practice, this had two problems:

1. The upstream repo (`github.com/bebbo/amiga-gcc`) intermittently returns 404, breaking the Docker build
2. Building from source takes 10-20 minutes, which is a poor first-run experience

Pre-built Docker images exist on Docker Hub (`amigadev/m68k-amigaos-gcc` and others) that provide the same toolchain ready to use.

## Decision

Use pre-built Docker images as the primary toolchain source. The setup script (`toolchain/scripts/setup-toolchain.sh`) now:

1. Checks if a pre-built image is already available locally
2. If not, pulls `amigadev/m68k-amigaos-gcc` from Docker Hub
3. Falls back to building from our Dockerfile only if the pull fails

Wrapper scripts (`toolchain/scripts/m68k-amigaos-gcc`, `m68k-amigaos-ar`, `m68k-amigaos-ranlib`) transparently route compiler invocations to Docker or native toolchains, so Makefiles don't need to know which is in use.

## Consequences

### Positive

- First-run setup takes seconds (image pull) instead of 10-20 minutes (source build)
- No dependency on upstream GitHub repo availability
- Wrapper scripts make Docker usage invisible to Makefiles

### Negative

- Less control over the exact compiler version and patches
- Pre-built image is x86-only, runs via Rosetta on ARM Macs (works but shows platform warnings)
- Depends on third-party Docker Hub image being maintained
