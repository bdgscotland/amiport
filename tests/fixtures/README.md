# Test Fixtures for Metadata Validation

Synthetic test fixtures for regression-testing `scripts/check-port-metadata.sh`.

## Fixture Structure

```
tests/fixtures/
├── bad-port/          # Multi-violation test case
├── no-ver-port/       # Missing $VER string test case
└── README.md          # This file
```

## bad-port

Deliberately exhibits 6 validation failures and 1 warning:

| Check | Status | Details |
|-------|--------|---------|
| Required files | FAIL | Missing `.readme` file |
| Template placeholders | FAIL | PORT.md contains `__SOURCE_URL__`, `__SOURCE_VERSION__` |
| Version consistency | FAIL | Conflicting `$VER` strings: `2.0` (bad-port.c) vs `1.5` (extra.c) |
| Version mismatch | FAIL | Makefile VERSION=2.0 but PORT.md Version=1.0 |
| PORTS.md entry | FAIL | Not in catalog |
| Stray artifacts | FAIL | Contains gmon.out |
| Test report | WARN | Shows 0/0 passed (empty breakdown table) |

### Files

- `Makefile` — VERSION=2.0
- `PORT.md` — Version=1.0, with placeholders
- `original/bad-port.c` — Upstream source stub
- `ported/bad-port.c` — $VER: bad-port 2.0
- `ported/extra.c` — $VER: extra 1.5 (conflicting)
- `test-fsemu-cases.txt` — Valid test cases file
- `TEST-REPORT.md` — Empty report (0/0 passed)
- `gmon.out` — Stray profiling artifact
- ~~bad-port.readme~~ — Deliberately missing

## no-ver-port

Deliberately exhibits 1 critical failure and 1 procedural failure:

| Check | Status | Details |
|-------|--------|---------|
| Required files | PASS | All present |
| Template placeholders | PASS | None found |
| Version consistency | FAIL | No $VER string in ported/*.c |
| PORTS.md entry | FAIL | Not in catalog |
| Stray artifacts | PASS | None found |

### Files

- `Makefile` — VERSION=1.0
- `PORT.md` — Version=1.0, valid table
- `no-ver-port.readme` — Valid Aminet readme
- `original/no-ver-port.c` — Upstream source stub
- `ported/no-ver-port.c` — **No $VER string** (violation)
- `test-fsemu-cases.txt` — Valid test cases file

## Usage

Run validation script against fixtures:

```bash
PORTS_DIR=tests/fixtures bash scripts/check-port-metadata.sh
```

Expected output:
- `bad-port`: 0 clean, 0 warnings, 1 failed
- `no-ver-port`: 0 clean, 0 warnings, 1 failed
- **Total**: 0 clean, 0 warnings, 2 failed → exit 1

## Purpose

These fixtures enable:
1. Automated regression testing of metadata checks without real ports
2. Documentation of what each validation rule catches
3. CI/CD verification that the validation script works correctly
4. Development iteration on new validation checks

## Do Not Modify

These fixtures are **frozen regression test cases**. Do not add/remove violations or change error conditions without updating test documentation.

