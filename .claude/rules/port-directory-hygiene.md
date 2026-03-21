# Port Directory Hygiene (ADR-013)

## What Goes Where

Every port directory (`ports/<name>/`) must contain exactly:

```
ports/<name>/
├── original/          # Upstream source ONLY (.c, .h)
├── ported/            # Transformed source ONLY (.c, .h)
├── Makefile           # Build rules
├── PORT.md            # Porting log
├── <name>.readme      # Aminet readme (REQUIRED for every port)
└── <name>             # Compiled Amiga binary (COMMITTED)
```

## Committed Artifacts

- **Compiled binary** (`ports/<name>/<name>`) — YES, commit it. Users get working binaries without needing the toolchain.
- **Aminet readme** (`ports/<name>/<name>.readme`) — YES, required for every port. Use the template from `ports/templates/readme.template`.
- Source files in `original/` and `ported/` — YES.

## NEVER Create These

| Pattern | Why |
|---------|-----|
| Binary in `ported/` dir | Build output goes in port root, not source dir |
| `*-*.readme` (versioned) | Use `<name>.readme` only, not `cal-1.0.readme` |
| `.lha`, `.zip` archives | Ephemeral packaging — not committed |
| `gmon.out`, `*.o`, `*_native` | Build/profiling artifacts |
| Directories mirroring absolute paths (`Users/`, `/tmp/`) | Agent bugs |
| Any file in the project root | All port files stay inside `ports/<name>/` |

## After Every Port

Before committing, verify:
1. Binary exists at `ports/<name>/<name>`
2. Readme exists at `ports/<name>/<name>.readme`
3. No stray files in `ported/` besides `.c` and `.h`
4. No files created outside `ports/<name>/`
