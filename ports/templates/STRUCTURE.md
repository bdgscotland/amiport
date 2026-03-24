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
| `<name>-<version>.lha` | `make package` | Distribution archive |
| `<name>_native` | `make compare` | Native build for output comparison (temp) |
| `test_*.txt` | `make test` | Test input/output files (temp) |

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
