---
name: port-status
description: Show comprehensive status of a single port — build, tests, reviews, catalog, site data, documentation
user-invocable: true
args: "<port-name>"
---

# Port Status Dashboard

Show the completion status of a single port across all required dimensions.

## Usage

The user invokes `/port-status <name>` where `<name>` is the port directory name (e.g., `grep`, `jq`, `wget`).

## What to Check

Run ALL of these checks for the given port and output a status dashboard:

### 1. Core Artifacts

| Check | How | Pass |
|-------|-----|------|
| Binary exists | `ports/<name>/<name>` file exists | File present |
| Binary size | `ls -la ports/<name>/<name>` | Non-zero |
| LHA package | `ports/<name>/<name>-*.lha` exists | File present |
| Aminet readme | `ports/<name>/<name>.readme` exists | File present |
| Versioned readme | `ports/<name>/<name>-*.readme` exists | File present |
| Original source | `ports/<name>/original/` has .c files | At least 1 |
| Ported source | `ports/<name>/ported/` has .c files | At least 1 |

### 2. Metadata Consistency

| Check | How | Pass |
|-------|-----|------|
| Makefile VERSION | `grep '^VERSION' ports/<name>/Makefile` | Set |
| Makefile REVISION | `grep '^REVISION' ports/<name>/Makefile` | Set (default 1) |
| $VER string | `grep '\$VER' ports/<name>/ported/*.c` | Matches VERSION-REVISION |
| .readme Version | `grep '^Version:' ports/<name>/<name>.readme` | Matches VERSION-REVISION |
| DESCRIPTION length | `grep '^DESCRIPTION' ports/<name>/Makefile` | <= 40 chars, ASCII only |

### 3. Test Coverage

| Check | How | Pass |
|-------|-----|------|
| Functional tests | `ports/<name>/test-fsemu-cases.txt` exists | File present |
| Test count | Count `TEST:` and `ITEST:` lines | >= 1 |
| Visual tests | `ports/<name>/test-fsemu-visual-cases.txt` exists | Required for Cat 3+ |
| Test input files | `ports/<name>/test-*.txt` files | Present if tests reference them |

### 4. Documentation

| Check | How | Pass |
|-------|-----|------|
| PORT.md exists | File present | Yes |
| PORT.md quality | `wc -l` >= 60, no "To be filled" | Both conditions met |
| PORTS.md entry | `grep '<name>' PORTS.md` | Row exists with current version |

### 5. Catalog & Site Data

| Check | How | Pass |
|-------|-----|------|
| catalog.json entry | `jq '.candidates[] | select(.name=="<name>")' data/catalog.json` | Entry exists |
| catalog status | `.status` field | "ported" |
| catalog binary_kb | `.measured_binary_kb` | Matches actual `ls -la` size (within 1KB) |
| catalog test_count | `.test_count` | Matches actual test count |
| site catalog synced | `diff data/catalog.json site/data/catalog.json` | No diff |
| site package JSON | `site/data/packages/<name>.json` exists | File present |
| site package version | Version in package JSON | Matches Makefile VERSION-REVISION |
| site sha256 | `sha256` field in package JSON | Non-empty |
| site machine_sha256 | `machine_sha256` field in package JSON | Non-empty |

### 6. Quality Gates

| Check | How | Pass |
|-------|-----|------|
| Memory audit | Look for memory-checker notes in PORT.md | Section present |
| Perf optimization | Look for perf-optimizer notes in PORT.md | Section present |
| Agent memory | `ports/<name>/.claude/agent-memory/` exists | Directory present |

## Output Format

```
## Port Status: <name> <version>

### Summary: X/Y checks passed [ALL CLEAR | N issues]

### Core Artifacts
  [✓] Binary: ports/<name>/<name> (XXX KB)
  [✓] LHA: ports/<name>/<name>-<ver>.lha (XXX KB)
  [✗] Versioned readme: MISSING
  ...

### Metadata
  [✓] VERSION=1.68, REVISION=2 → display 1.68-2
  [✗] $VER says "1.68" but REVISION=2, should be "1.68-2"
  ...

### Tests
  [✓] Functional: 25 tests in test-fsemu-cases.txt
  [✓] Visual: 4 tests in test-fsemu-visual-cases.txt
  ...

### Documentation
  [✓] PORT.md: 85 lines, no placeholders
  [✗] PORTS.md: version shows "1.68" but should be "1.68-2"
  ...

### Catalog & Site
  [✓] catalog.json: status=ported, binary=45KB, tests=25
  [✗] site package JSON: sha256 is empty
  ...

### Quality Gates
  [✓] Memory audit: completed (PORT.md §Memory Safety)
  [✗] Perf optimization: no section in PORT.md
  ...
```

Use `[✓]` for pass, `[✗]` for fail, `[~]` for warning (non-blocking).
