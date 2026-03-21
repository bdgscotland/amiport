# ADR-013: Port Directory Hygiene

## Status

Accepted

## Date

2026-03-21

## Context

AI agents running the porting pipeline create stray files: binaries in `ported/` subdirectories, profiling artifacts (`gmon.out`), packaging archives in the wrong place, mirrored absolute paths (`Users/duncan/Developer/...`), and stale test binaries. Without strict rules, every porting session leaves debris that clutters `git status` and gets accidentally committed.

At the same time, compiled Amiga binaries ARE distributable artifacts that should be committed — users cloning the repo get working programs without needing the cross-compiler toolchain.

## Decision

### What goes where in a port directory

```
ports/<name>/
├── original/          # Unmodified upstream source ONLY (.c, .h)
├── ported/            # Transformed source ONLY (.c, .h)
├── Makefile           # Build rules
├── PORT.md            # Porting log
├── <name>.readme      # Aminet readme
└── <name>             # Compiled Amiga binary (committed)
```

### What gets committed

| File | Committed? | Notes |
|------|-----------|-------|
| `ports/<name>/<name>` | YES | The compiled Amiga binary |
| `ports/<name>/<name>.readme` | YES | Aminet readme |
| `ports/<name>/PORT.md` | YES | Porting documentation |
| `ports/<name>/Makefile` | YES | Build rules |
| `ports/<name>/original/*.c` `*.h` | YES | Upstream reference |
| `ports/<name>/ported/*.c` `*.h` | YES | Transformed source |

### What NEVER gets committed

| Pattern | Reason |
|---------|--------|
| `ports/<name>/ported/<name>` | Binary in source dir — build goes in port root |
| `ports/<name>/*-*.readme` | Versioned readmes (e.g. `cal-1.0.readme`) — use `<name>.readme` |
| `ports/<name>/*.lha` `*.zip` | Packaging archives — ephemeral |
| `ports/<name>/gmon.out` | GCC profiling artifact |
| `ports/<name>/*.o` | Object files |
| `ports/<name>/*_native` | Native comparison binaries |
| `ports/<name>/test_*` (binaries) | Stale test executables |
| `Users/` or any absolute path dirs | Agent bugs mirroring host paths |

### .gitignore enforcement

The root `.gitignore` must include patterns for all "never committed" items above. Agents must not bypass `.gitignore` with `git add -f`.

## Consequences

### Positive

- Clean `git status` after every porting session
- Users cloning the repo get working binaries immediately
- Consistent directory layout across all ports
- Agents have unambiguous rules for file placement

### Negative

- Binary files in git increase repo size (~30-60KB per port)
- Must rebuild binaries when changing shim library (but `make build-ports` handles this)

### Neutral

- Every port must have a `<name>.readme` file — no exceptions
