# Port Metadata Validation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Automated validation of port metadata consistency — versions, placeholders, required files, PORTS.md entries, test report quality, and stray artifacts — wired into make, pre-commit, and CI.

**Architecture:** A single bash script (`scripts/check-port-metadata.sh`) iterates over `ports/*/`, runs 6 validation checks per port, and exits non-zero on any FAIL. Follows the exact pattern of existing `scripts/check-test-coverage.sh`. Wired into the Makefile, pre-commit hook, and CI workflow.

**Tech Stack:** Bash, grep, sed, awk (matching existing check scripts)

**Spec:** `docs/superpowers/specs/2026-03-22-port-metadata-validation-design.md`

**Spec deviations:**
- `$VER` count rule relaxed from "exactly one" to "at least one, all must agree on version". Reason: lua legitimately produces two binaries (lua, luac) each with their own `$VER` string.
- `test-fsemu.sh` report enrichment deferred — code review found the report generation already works correctly (lines 490-605 of `scripts/test-fsemu.sh`). The grep TEST-REPORT.md showing "0/0 passed" was from a run where ARexx crashed (UTF-8 bug), not a code deficiency. No code change needed — just re-run tests.

---

### Task 1: Create the validation script

**Files:**
- Create: `scripts/check-port-metadata.sh`

**Context:** Follow the pattern in `scripts/check-test-coverage.sh` — iterate `ports/*/`, skip `templates` and `common-test-data`, report per-port, summarize at end.

**Refinements from code survey:**
- PORT.md version: some ports (cal, diff) use an older format without `| Version | X.Y |` — if no version found in PORT.md, WARN (not FAIL) and skip that source from the 4-way comparison
- `$VER` count: lua has 2 `$VER` strings legitimately (lua.c + luac.c produce two binaries). Rule: at least 1 required, all must agree on version number. Zero is FAIL, 2+ with conflicting versions is FAIL.
- Stray artifacts: add `*.o` to the list (lua has a stray `.o` in `ported/`)

- [ ] **Step 1: Write the script**

```bash
#!/usr/bin/env bash
set -euo pipefail

# check-port-metadata.sh — CI gate: validate port metadata consistency
#
# For each port in ports/*/:
#   1. Required files exist (Makefile, PORT.md, .readme, test-fsemu-cases.txt, source files)
#   2. No template placeholders remain (__PLACEHOLDER__ patterns)
#   3. Version consistency across Makefile, .readme, PORT.md, $VER string
#   4. PORTS.md entry exists
#   5. TEST-REPORT.md quality (if present)
#   6. No stray build artifacts (.lha, gmon.out, *_native, *.map, *.o in ported/)
#
# Exit 0 if all ports pass, exit 1 if any fail.

PORTS_DIR="${PORTS_DIR:-ports}"
PORTS_CATALOG="PORTS.md"
failed=0
warned=0
checked=0

# Verify PORTS.md exists
if [ ! -f "$PORTS_CATALOG" ]; then
    echo "FATAL: $PORTS_CATALOG not found at project root"
    exit 1
fi

for dir in "$PORTS_DIR"/*/; do
    name=$(basename "$dir")

    # Skip non-port directories
    if [ ! -f "$dir/Makefile" ]; then
        continue
    fi
    if [ "$name" = "templates" ] || [ "$name" = "common-test-data" ]; then
        continue
    fi

    checked=$((checked + 1))
    port_failed=0
    port_warned=0

    # ----------------------------------------------------------
    # Check 1: Required files
    # ----------------------------------------------------------
    missing=""
    [ ! -f "$dir/Makefile" ] && missing="$missing Makefile"
    [ ! -f "$dir/PORT.md" ] && missing="$missing PORT.md"
    [ ! -f "$dir/${name}.readme" ] && missing="$missing ${name}.readme"
    [ ! -f "$dir/test-fsemu-cases.txt" ] && missing="$missing test-fsemu-cases.txt"

    has_original=$(find "$dir/original" -name '*.c' 2>/dev/null | head -1)
    [ -z "$has_original" ] && missing="$missing original/*.c"

    has_ported=$(find "$dir/ported" -name '*.c' 2>/dev/null | head -1)
    [ -z "$has_ported" ] && missing="$missing ported/*.c"

    if [ -n "$missing" ]; then
        echo "FAIL  $name: required files — missing:$missing"
        port_failed=1
    else
        echo "PASS  $name: required files"
    fi

    # ----------------------------------------------------------
    # Check 2: No template placeholders
    # ----------------------------------------------------------
    placeholders=""
    for f in "$dir/PORT.md" "$dir/Makefile" "$dir/${name}.readme"; do
        if [ -f "$f" ]; then
            found=$(grep -oE '__[A-Z_]+__' "$f" 2>/dev/null | sort -u | tr '\n' ' ')
            [ -n "$found" ] && placeholders="$placeholders $(basename "$f"):$found"
        fi
    done

    if [ -n "$placeholders" ]; then
        echo "FAIL  $name: placeholders remain —$placeholders"
        port_failed=1
    else
        echo "PASS  $name: no placeholders"
    fi

    # ----------------------------------------------------------
    # Check 3: Version consistency
    # ----------------------------------------------------------
    # Extract from Makefile (VERSION = X.Y or VERSION=X.Y)
    ver_makefile=$(grep -E '^VERSION\s*=' "$dir/Makefile" 2>/dev/null | head -1 | sed 's/.*=\s*//' | tr -d ' ')

    # Extract from .readme (Version:      X.Y)
    ver_readme=""
    if [ -f "$dir/${name}.readme" ]; then
        ver_readme=$(grep -E '^Version:' "$dir/${name}.readme" 2>/dev/null | head -1 | sed 's/Version:\s*//' | tr -d ' ')
    fi

    # Extract from PORT.md (| Version | X.Y |)
    ver_portmd=""
    if [ -f "$dir/PORT.md" ]; then
        ver_portmd=$(grep -E '^\| Version \|' "$dir/PORT.md" 2>/dev/null | head -1 | awk -F'|' '{print $3}' | tr -d ' ')
    fi

    # Extract from $VER string in ported/*.c
    ver_source=""
    ver_count=0
    ver_versions=""
    ver_conflict=false
    if [ -d "$dir/ported" ]; then
        while IFS= read -r line; do
            ver_count=$((ver_count + 1))
            # Extract version: $VER: name X.Y (date) -> X.Y
            v=$(echo "$line" | sed -E 's/.*\$VER: [^ ]+ ([^ ]+) .*/\1/')
            ver_versions="$ver_versions $v"
            [ -z "$ver_source" ] && ver_source="$v"
        done < <(grep -rh '\$VER:' "$dir/ported/"*.c 2>/dev/null || true)
    fi

    # Check $VER count and consistency
    if [ "$ver_count" -eq 0 ]; then
        echo "FAIL  $name: version — no \$VER string in ported/*.c"
        port_failed=1
    else
        # Check all $VER strings agree
        ver_conflict=false
        for v in $ver_versions; do
            if [ "$v" != "$ver_source" ]; then
                ver_conflict=true
            fi
        done
        if [ "$ver_conflict" = true ]; then
            echo "FAIL  $name: version — conflicting \$VER strings:$ver_versions"
            port_failed=1
        fi
    fi

    # Compare available versions (skip sources where version not found)
    if [ -n "$ver_makefile" ] && [ "$ver_count" -gt 0 ] && [ "$ver_conflict" = false ]; then
        mismatch=""
        [ -n "$ver_readme" ] && [ "$ver_readme" != "$ver_makefile" ] && mismatch="$mismatch .readme=$ver_readme"
        [ -n "$ver_portmd" ] && [ "$ver_portmd" != "$ver_makefile" ] && mismatch="$mismatch PORT.md=$ver_portmd"
        [ -n "$ver_source" ] && [ "$ver_source" != "$ver_makefile" ] && mismatch="$mismatch \$VER=$ver_source"

        if [ -n "$mismatch" ]; then
            echo "FAIL  $name: version — Makefile=$ver_makefile but$mismatch"
            port_failed=1
        elif [ -z "$ver_portmd" ]; then
            echo "WARN  $name: version — consistent ($ver_makefile) but PORT.md has no version row"
            port_warned=1
        else
            echo "PASS  $name: version ($ver_makefile)"
        fi
    elif [ -z "$ver_makefile" ]; then
        echo "FAIL  $name: version — no VERSION in Makefile"
        port_failed=1
    fi

    # ----------------------------------------------------------
    # Check 4: PORTS.md entry
    # ----------------------------------------------------------
    if grep -qE "^\|[[:space:]]*\[?${name}\]?" "$PORTS_CATALOG" 2>/dev/null; then
        echo "PASS  $name: PORTS.md entry"
    else
        echo "FAIL  $name: PORTS.md entry — not found"
        port_failed=1
    fi

    # ----------------------------------------------------------
    # Check 5: TEST-REPORT.md quality
    # ----------------------------------------------------------
    if [ -f "$dir/TEST-REPORT.md" ]; then
        has_zero=$(grep -c '0/0 passed' "$dir/TEST-REPORT.md" 2>/dev/null || true)
        # Check breakdown table has data rows (lines starting with | N | where N is a digit)
        has_data_rows=$(grep -cE '^\| [0-9]' "$dir/TEST-REPORT.md" 2>/dev/null || true)

        if [ "$has_zero" -gt 0 ]; then
            echo "WARN  $name: test report — shows 0/0 passed (stale?)"
            port_warned=1
        elif [ "$has_data_rows" -eq 0 ]; then
            echo "WARN  $name: test report — empty breakdown table"
            port_warned=1
        else
            echo "PASS  $name: test report"
        fi
    else
        echo "SKIP  $name: test report (no TEST-REPORT.md)"
    fi

    # ----------------------------------------------------------
    # Check 6: Stray artifacts
    # ----------------------------------------------------------
    strays=""
    for f in "$dir"/*.lha; do [ -f "$f" ] && strays="$strays $(basename "$f")"; done
    for f in "$dir"/gmon.out; do [ -f "$f" ] && strays="$strays gmon.out"; done
    for f in "$dir"/*_native; do [ -f "$f" ] && strays="$strays $(basename "$f")"; done
    for f in "$dir"/*.map; do [ -f "$f" ] && strays="$strays $(basename "$f")"; done
    # Check for .o files in ported/ (build artifacts)
    for f in "$dir"/ported/*.o; do [ -f "$f" ] && strays="$strays ported/$(basename "$f")"; done

    if [ -n "$strays" ]; then
        echo "FAIL  $name: stray artifacts —$strays"
        port_failed=1
    else
        echo "PASS  $name: no stray artifacts"
    fi

    # Separator between ports
    [ "$port_failed" -gt 0 ] && failed=$((failed + 1))
    [ "$port_warned" -gt 0 ] && [ "$port_failed" -eq 0 ] && warned=$((warned + 1))
    echo ""
done

echo "Checked $checked ports: $((checked - failed - warned)) clean, $warned warnings, $failed failed"

if [ "$failed" -gt 0 ]; then
    exit 1
fi
```

- [ ] **Step 2: Make it executable**

Run: `chmod +x scripts/check-port-metadata.sh`

- [ ] **Step 3: Run against real ports to verify**

Run: `bash scripts/check-port-metadata.sh`

Expected: grep FAILS on version mismatch + stray artifacts, cut/sed FAIL on version mismatch, lua FAIL on stray .o, cal/diff WARN on missing PORT.md version row.

- [ ] **Step 4: Commit**

```bash
git add scripts/check-port-metadata.sh
git commit -m "feat: add port metadata validation script (check-port-metadata.sh)"
```

---

### Task 2: Create synthetic test fixtures

**Files:**
- Create: `tests/fixtures/bad-port/Makefile`
- Create: `tests/fixtures/bad-port/PORT.md`
- Create: `tests/fixtures/bad-port/TEST-REPORT.md`
- Create: `tests/fixtures/bad-port/original/bad-port.c`
- Create: `tests/fixtures/bad-port/ported/bad-port.c`
- Create: `tests/fixtures/bad-port/ported/extra.c`
- Create: `tests/fixtures/bad-port/test-fsemu-cases.txt`
- Create: `tests/fixtures/bad-port/gmon.out`
- Create: `tests/fixtures/no-ver-port/Makefile`
- Create: `tests/fixtures/no-ver-port/PORT.md`
- Create: `tests/fixtures/no-ver-port/no-ver-port.readme`
- Create: `tests/fixtures/no-ver-port/original/no-ver-port.c`
- Create: `tests/fixtures/no-ver-port/ported/no-ver-port.c`
- Create: `tests/fixtures/no-ver-port/test-fsemu-cases.txt`

**Context:** Two fixtures with deliberate errors. `bad-port` tests: missing .readme, template placeholders, version conflicts, stray artifacts, bad test report. `no-ver-port` tests: zero `$VER` strings.

- [ ] **Step 1: Create bad-port fixture**

Create `tests/fixtures/bad-port/Makefile`:
```makefile
# Synthetic test fixture — deliberate errors for check-port-metadata.sh
TARGET   = bad-port
VERSION  = 2.0
CATEGORY = 1
SOURCES  = bad-port.c extra.c
```

Create `tests/fixtures/bad-port/PORT.md`:
```markdown
# Port: bad-port

## Overview

| Field | Value |
|-------|-------|
| Program | bad-port |
| Version | 1.0 |
| Source | __SOURCE_URL__ (__SOURCE_VERSION__) |
| Category | 1 — CLI tool |
```

Create `tests/fixtures/bad-port/TEST-REPORT.md`:
```markdown
# FS-UAE Test Report: bad-port

## Summary

| Field | Value |
|-------|-------|
| Result | **PASS** — 0/0 passed |

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
```

Create `tests/fixtures/bad-port/original/bad-port.c`:
```c
/* synthetic test fixture */
int main(void) { return 0; }
```

Create `tests/fixtures/bad-port/ported/bad-port.c`:
```c
/* synthetic test fixture */
static const char *verstag = "$VER: bad-port 2.0 (22.03.2026)";
int main(void) { return 0; }
```

Create `tests/fixtures/bad-port/ported/extra.c`:
```c
/* synthetic test fixture — conflicting version */
static const char *verstag = "$VER: extra 1.5 (22.03.2026)";
void helper(void) {}
```

Create `tests/fixtures/bad-port/test-fsemu-cases.txt`:
```
TEST: basic test
CMD: WORK:bad-port
EXPECT_RC: 0
```

Create `tests/fixtures/bad-port/gmon.out` (empty):
```bash
touch tests/fixtures/bad-port/gmon.out
```

- [ ] **Step 2: Create no-ver-port fixture**

Create `tests/fixtures/no-ver-port/Makefile`:
```makefile
# Synthetic test fixture — no $VER string
TARGET   = no-ver-port
VERSION  = 1.0
CATEGORY = 1
SOURCES  = no-ver-port.c
```

Create `tests/fixtures/no-ver-port/PORT.md`:
```markdown
# Port: no-ver-port

## Overview

| Field | Value |
|-------|-------|
| Program | no-ver-port |
| Version | 1.0 |
| Category | 1 — CLI tool |
```

Create `tests/fixtures/no-ver-port/no-ver-port.readme`:
```
Short:        Synthetic test fixture
Type:         util/cli
Architecture: m68k-amigaos >= 3.0
Uploader:     test@test.com
Author:       Test
Version:      1.0
```

Create `tests/fixtures/no-ver-port/original/no-ver-port.c`:
```c
int main(void) { return 0; }
```

Create `tests/fixtures/no-ver-port/ported/no-ver-port.c` (**no $VER string**):
```c
/* deliberately missing $VER string */
int main(void) { return 0; }
```

Create `tests/fixtures/no-ver-port/test-fsemu-cases.txt`:
```
TEST: basic test
CMD: WORK:no-ver-port
EXPECT_RC: 0
```

- [ ] **Step 3: Run script against fixtures**

Run: `PORTS_DIR=tests/fixtures bash scripts/check-port-metadata.sh`

Expected for bad-port:
- FAIL: missing `bad-port.readme`
- FAIL: placeholder `__SOURCE_URL__`, `__SOURCE_VERSION__` in PORT.md
- FAIL: conflicting `$VER` strings (2.0 vs 1.5)
- FAIL: PORTS.md entry not found
- WARN: test report shows 0/0 passed
- FAIL: stray artifact (gmon.out)

Expected for no-ver-port:
- FAIL: no `$VER` string in ported/*.c
- FAIL: PORTS.md entry not found

- [ ] **Step 4: Commit**

```bash
git add tests/fixtures/
git commit -m "test: add synthetic fixtures for metadata validation"
```

---

### Task 3: Wire into Makefile

**Files:**
- Modify: `Makefile:12` (add to .PHONY)
- Modify: `Makefile:46-49` (add help text)
- Add target after `check-fix-propagation` (~line 268)

- [ ] **Step 1: Add target to Makefile**

Add to `.PHONY` line: `check-port-metadata`

Add help text:
```
	@echo "  check-port-metadata  Validate port metadata consistency"
```

Add target:
```makefile
check-port-metadata:
	@bash scripts/check-port-metadata.sh
```

- [ ] **Step 2: Verify it runs**

Run: `make check-port-metadata`
Expected: Same output as running the script directly. Exits non-zero due to existing violations.

- [ ] **Step 3: Commit**

```bash
git add Makefile
git commit -m "feat: add make check-port-metadata target"
```

---

### Task 4: Fix existing port metadata violations

**Files:**
- Modify: `ports/grep/PORT.md` (fix version 1.0 → 1.68)
- Modify: `ports/cut/PORT.md` (fix version 1.0 → 1.28)
- Modify: `ports/sed/PORT.md` (fix version 1.0 → 1.47)
- Remove: `ports/grep/gmon.out`, `ports/grep/grep-1.68.lha`, `ports/grep/grep.map`
- Remove: `ports/lua/ported/lua.o`
- Remove: any other stray `.lha`, `.map`, `gmon.out` files

**Context:** These must be fixed so the pre-commit hook and CI pass. These are the exact bugs that motivated this feature.

- [ ] **Step 1: Survey all violations**

Run: `bash scripts/check-port-metadata.sh` and note every FAIL.

- [ ] **Step 2: Fix PORT.md version rows**

For grep: change `| Version | 1.0 |` to `| Version | 1.68 |`
For cut: change `| Version | 1.0 |` to `| Version | 1.28 |`
For sed: change `| Version | 1.0 |` to `| Version | 1.47 |`

- [ ] **Step 3: Remove stray artifacts**

```bash
rm -f ports/grep/gmon.out ports/grep/grep-1.68.lha ports/grep/grep.map
rm -f ports/lua/ported/lua.o
# Remove any other stray artifacts found in Step 1
```

Check all ports:
```bash
find ports/ -name '*.lha' -o -name 'gmon.out' -o -name '*.map' -o -name '*_native' | grep -v templates
find ports/*/ported/ -name '*.o' 2>/dev/null
```

- [ ] **Step 4: Re-run validation**

Run: `bash scripts/check-port-metadata.sh`
Expected: No FAIL. Some WARN for cal/diff missing PORT.md version row (acceptable — older format).

- [ ] **Step 5: Commit**

```bash
git add ports/grep/PORT.md ports/cut/PORT.md ports/sed/PORT.md
git rm ports/grep/gmon.out ports/grep/grep-1.68.lha ports/grep/grep.map ports/lua/ported/lua.o
# git rm any other stray artifacts found
git commit -m "fix: resolve port metadata violations caught by check-port-metadata"
```

---

### Task 5: Wire into pre-commit hook

**Files:**
- Modify: `.githooks/pre-commit`

**Context:** Must come AFTER Task 4 (fix violations), otherwise the hook blocks its own commit.

- [ ] **Step 1: Add check to pre-commit**

After the existing `make check-docs` line (~line 10), add:

```bash
echo "=== Pre-commit: checking port metadata consistency ==="
make check-port-metadata
```

- [ ] **Step 2: Commit**

```bash
git add .githooks/pre-commit
git commit -m "ci: add check-port-metadata to pre-commit hook"
```

---

### Task 6: Wire into CI

**Files:**
- Modify: `.github/workflows/ci.yml`

- [ ] **Step 1: Add CI step**

After the `Check test coverage standard` step (~line 82), add:

```yaml
      - name: Check port metadata consistency
        run: make check-port-metadata
```

- [ ] **Step 2: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "ci: add check-port-metadata to GitHub Actions workflow"
```

---

### Task 7: Update documentation

**Files:**
- Modify: `CLAUDE.md` — add `make check-port-metadata` to Build Instructions
- Modify: `README.md` — add to make targets table

- [ ] **Step 1: Update CLAUDE.md**

In the Build Instructions section, add after the `make check-fix-propagation` line:

```
make check-port-metadata  # Validate port metadata consistency (version, files, PORTS.md)
```

In the Safety Hooks section or Git Hooks section, mention that pre-commit now also runs `check-port-metadata`.

- [ ] **Step 2: Update README.md**

Add `check-port-metadata` to the make targets table in README.md.

- [ ] **Step 3: Verify all checks pass**

Run:
```bash
make check-docs          # Existing doc consistency
make check-port-metadata  # New metadata check
make check-test-coverage  # Existing test coverage
```

All must pass.

- [ ] **Step 4: Commit**

```bash
git add CLAUDE.md README.md
git commit -m "docs: document check-port-metadata in CLAUDE.md and README.md"
```

---

### Task 8: Final verification

- [ ] **Step 1: Run full validation suite**

```bash
make check-docs
make check-agents
make check-port-metadata
make check-test-coverage
```

All must pass with exit 0.

- [ ] **Step 2: Run synthetic fixture test**

```bash
PORTS_DIR=tests/fixtures bash scripts/check-port-metadata.sh; echo "EXIT: $?"
```

Must exit non-zero and catch all deliberate errors.

- [ ] **Step 3: Verify pre-commit hook works**

```bash
git stash  # If there are unstaged changes
# Make a trivial change to test the hook
echo "" >> CLAUDE.md
git add CLAUDE.md
git commit -m "test: verify pre-commit hook"
# If it passes, the hook ran check-port-metadata successfully
git reset HEAD~1  # Undo test commit
git checkout CLAUDE.md
git stash pop  # Restore stashed changes if any
```
