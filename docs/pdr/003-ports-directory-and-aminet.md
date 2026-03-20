# PDR-003: Ports Directory as Output Collection

## Status

Accepted

## Date

2026-03-20

## Problem

When `/port-project` produces a working Amiga port, there was no defined place to put it. The skill said "create a `ported/` directory alongside the original" but that left ports scattered wherever the user's source happened to be.

## Target Users

- Amiga enthusiasts looking for pre-built ports of Unix utilities
- Contributors wanting to see what's already been ported
- The porting pipeline itself, which needs a consistent structure to build/test/package

## Decision

All ports go into `ports/<program-name>/` with a standard structure:

```
ports/<name>/
├── original/       # Upstream POSIX source
├── ported/         # Transformed AmigaOS source
├── Makefile        # Includes ../common.mk
├── PORT.md         # Porting log
└── <name>.readme   # Generated for Aminet
```

Each port packages independently as an LHA for individual Aminet upload — no monolithic collection. `make list-ports` shows status, `make build-ports` builds everything.

## Rationale

- Aminet takes individual uploads, not collections — per-port packaging is the right granularity
- Standard directory structure means the pipeline can be fully automated
- `ports/common.mk` with shared build settings (DRY) keeps Makefiles minimal
- PORT.md serves as both documentation and template for the `/port-project` skill

## Success Criteria

- `make list-ports` shows all ports with build status
- `make build TARGET=ports/<name>` builds any port
- `make -C ports/<name> package` creates an Aminet-ready LHA
- First real port (cal) successfully uses this structure

## Alternatives Considered

- **Separate repo per port**: Too much overhead, loses the shared shim library
- **Monolithic LHA collection**: Aminet prefers individual uploads; harder to update one port without re-releasing everything
- **Output to user's source directory**: No central collection, can't list/build all ports
