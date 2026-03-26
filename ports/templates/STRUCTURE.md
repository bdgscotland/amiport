# Port Directory Structure

Every port lives in `ports/<name>/` with this layout:

```
ports/<name>/
├── original/          # Unmodified POSIX source — never edited, reference only
│   └── <name>.c       # (or multiple .c/.h files for multi-file ports)
├── ported/            # Transformed AmigaOS source — this is what gets compiled
│   └── <name>.c       # Every change has a /* amiport: ... */ comment
├── Makefile           # Build rules — copied from templates/Makefile.template
├── PORT.md            # Porting log — copied from templates/PORT.md.template
├── <name>.readme      # Aminet readme — generated from templates/readme.template
├── test-fsemu-cases.txt          # FS-UAE functional test suite (REQUIRED)
└── test-fsemu-visual-cases.txt   # FS-UAE visual verification tests (Category 3+ — ADR-024)
```

## File Purposes

| File | Created At | By | Purpose |
|------|------------|-----|---------|
| `original/*.c` | Stage 2 | Pipeline | Preserved reference copy of upstream source |
| `ported/*.c` | Stage 3 | Pipeline | Transformed source with amiport shim calls |
| `Makefile` | Stage 2 | Template | Build, test, compare, package rules |
| `PORT.md` | Stage 2+ | Template + Pipeline | Documents every transformation and test result |
| `<name>.readme` | Stage 7 | Template | Aminet-format readme for distribution |
| `test-fsemu-cases.txt` | Stage 5b | test-designer | Functional test suite (non-interactive + ITEST) |
| `test-fsemu-visual-cases.txt` | Stage 5e | test-designer | Visual verification tests with SCRAPE (Category 3+, ADR-024) |

## Build Artifacts (not checked in)

| File | Created By | Purpose |
|------|-----------|---------|
| `<name>` | `make build` | Compiled AmigaOS binary |
| `<name>-<DISPLAY_VERSION>.lha` | `make package` | Distribution archive |
| `<name>_native` | `make compare` | Native build for output comparison (temp) |
| `test_*.txt` | `make test` | Test input/output files (temp) |

## Versioning

Each port Makefile defines:
- **VERSION** — upstream version (e.g., `1.68`). Only changes when pulling new upstream source.
- **REVISION** — port revision (default `1`). Increment when `ported/`, Makefile, shim deps, or tests change but upstream version stays the same.
- **DISPLAY_VERSION** — computed by `common.mk`: `VERSION` for rev 1, `VERSION-REVISION` for rev 2+ (e.g., `1.68-2`).

DISPLAY_VERSION flows to: `$VER` string in source, `.readme Version:` field, LHA filename, website, PORTS.md.

## Naming Conventions

- **Directory name** = **binary name** = **TARGET variable** (all lowercase)
- **Source files** in `ported/` keep their original names from `original/`
- **Makefile** always includes `../common.mk` — never duplicates shared rules
- **PORT.md** sections follow the order in `PORT.md.template` — no reordering

## What Goes Where

- **All build/test artifacts** stay inside the port directory — never in project root
- **Test input files** are created in the port directory, not `/tmp` or root
- **Native comparison binaries** are created and cleaned within the port directory
- **No files** should be created outside `ports/<name>/` during any pipeline stage
