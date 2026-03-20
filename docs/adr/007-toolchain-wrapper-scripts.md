# ADR-007: Toolchain Wrapper Scripts for Transparent Docker/Native Routing

## Status

Accepted

## Date

2026-03-20

## Context

Makefiles need to invoke `m68k-amigaos-gcc` but the compiler may be inside a Docker container or installed natively. Without wrapper scripts, every Makefile would need Docker-aware build logic, creating duplication and complexity.

## Decision

Provide wrapper scripts in `toolchain/scripts/` (`m68k-amigaos-gcc`, `m68k-amigaos-ar`, `m68k-amigaos-ranlib`) that:

1. Check for a native compiler in PATH (excluding the wrapper itself)
2. If not found, invoke via Docker with the project root mounted as `/work`
3. Auto-detect which Docker image is available (`amigadev/m68k-amigaos-gcc` or `amiport/bebbo-gcc`)

Makefiles set `CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc` where `TOOLCHAIN_BIN` points to the wrapper directory. All Docker complexity is hidden behind a standard compiler interface.

## Consequences

### Positive

- Makefiles are simple and standard — just `$(CC) $(CFLAGS) -o $@ $<`
- Same Makefile works with Docker or native toolchain, no conditional logic
- Adding new wrapper tools (e.g., `m68k-amigaos-strip`) follows the same pattern
- Working directory is correctly mapped into the container

### Negative

- Each compiler invocation starts a new Docker container (~1-2s overhead per file)
- Multi-file builds are slower than a single Docker session compiling everything
- Wrapper scripts add a layer of indirection that could confuse debugging
