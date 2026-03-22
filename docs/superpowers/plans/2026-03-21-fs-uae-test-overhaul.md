# FS-UAE Test Infrastructure Overhaul — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the FS-UAE test pipeline bulletproof — comprehensive test coverage, self-diagnosing hangs, self-healing fixes, and CI enforcement across all 7 production ports.

**Architecture:** Three parallel workstreams: (1) roll known crash-pattern fixes across all ports via code-transformer, (2) create a new test-designer agent that generates comprehensive test suites from source analysis, (3) add timeout diagnosis and self-healing to test-fsemu.sh. After all three complete, validate with FS-UAE, then add CI gates.

**Tech Stack:** Bash (test scripts, CI gates), ARexx (test harness — already exists), Claude agents (test-designer, code-transformer), Make (build targets)

**Spec:** `~/.gstack/projects/bdgscotland-amiport/ceo-plans/2026-03-21-fs-uae-test-overhaul.md`

---

## File Structure

### New files
| File | Responsibility |
|------|---------------|
| `.claude/agents/test-designer.md` | Agent definition for test suite generation |
| `ports/common-test-data/test-empty.txt` | Shared test input: 0-byte file |
| `ports/common-test-data/test-oneline.txt` | Shared test input: single line |
| `ports/common-test-data/test-multiline.txt` | Shared test input: 10 lines |
| `ports/common-test-data/test-longline.txt` | Shared test input: >1024 char line |
| `ports/common-test-data/test-binary.dat` | Shared test input: non-UTF8 content |
| `ports/common-test-data/test-special-chars.txt` | Shared test input: tabs, special chars |
| `scripts/check-test-coverage.sh` | CI gate: validates test suite completeness |
| `scripts/check-fix-propagation.sh` | CI scanner: detects known crash patterns |

### Modified files
| File | Change |
|------|--------|
| `scripts/test-fsemu.sh` | Add timeout diagnosis + self-healing dispatch |
| `scripts/install-emu.sh` | Copy common-test-data to WORK: |
| `.claude/skills/transform-source/references/transformation-rules.md` | Add crash-patterns rules (section 9) |
| `.claude/agents/code-transformer.md` | Add crash-patterns.md as mandatory reading |
| `.claude/skills/port-project/SKILL.md` | Add Stage 5b test-designer dispatch |
| `ports/common.mk` | Add CATEGORY variable |
| `ports/*/Makefile` | Add CATEGORY per port |
| `Makefile` | Add check-test-coverage and check-fix-propagation targets |
| `.github/workflows/ci.yml` | Add coverage gate + propagation scanner |
| `CLAUDE.md` | Add test-designer agent to table |
| `README.md` | Add test-designer agent + new make targets |
| `docs/architecture.md` | Add test-designer to pipeline diagram |
| `docs/porting-guide.md` | Add test-designer to workflow |
| `TODOS.md` | Add deferred FS-UAE CI item |

### Regenerated files (by test-designer agent)
| File | Change |
|------|--------|
| `ports/*/test-fsemu-cases.txt` | Replaced with comprehensive test suites |
| `ports/*/test-<name>-*.txt` | New test input files per port |

---

## Task 1: Create common test data files

**Files:**
- Create: `ports/common-test-data/test-empty.txt`
- Create: `ports/common-test-data/test-oneline.txt`
- Create: `ports/common-test-data/test-multiline.txt`
- Create: `ports/common-test-data/test-longline.txt`
- Create: `ports/common-test-data/test-binary.dat`
- Create: `ports/common-test-data/test-special-chars.txt`

- [ ] **Step 1: Create the directory and empty file**

```bash
mkdir -p ports/common-test-data
touch ports/common-test-data/test-empty.txt
```

- [ ] **Step 2: Create test-oneline.txt**

Content: `hello world` (single line, newline-terminated)

- [ ] **Step 3: Create test-multiline.txt**

10 lines of varied content — mix of words, numbers, blank lines, repeated patterns (useful for grep/sed/cut testing):
```
hello world
foo bar baz
12345
UPPERCASE LINE

hello world
line with    tabs
special: !@#$%
the quick brown fox
last line
```

- [ ] **Step 4: Create test-longline.txt**

Single line of 1100 'A' characters followed by the word "MARKER" then newline. This tests buffer boundaries (1024-byte buffers are common in ported code).

```bash
printf '%0.sA' {1..1100} > ports/common-test-data/test-longline.txt
printf 'MARKER\n' >> ports/common-test-data/test-longline.txt
```

- [ ] **Step 5: Create test-binary.dat**

8 bytes of non-UTF8 binary content:
```bash
printf '\x00\x01\x02\xff\xfe\xfd\x80\x7f' > ports/common-test-data/test-binary.dat
```

- [ ] **Step 6: Create test-special-chars.txt**

```
line	with	tabs
line with trailing spaces
"quoted string"
line with 'single quotes'
backslash\\path
```

- [ ] **Step 7: Commit**

```bash
git add ports/common-test-data/
git commit -m "feat: add shared test input files for FS-UAE testing"
```

---

## Task 2: Extend install-emu.sh to copy common test data

**Files:**
- Modify: `scripts/install-emu.sh`

- [ ] **Step 1: Add common-test-data copy block**

After the "Install built examples" loop (line 41) and before the `INSTALLED` check (line 43), add:

```bash
# Install common test data files
COMMON_DATA="$PROJECT_DIR/ports/common-test-data"
if [ -d "$COMMON_DATA" ]; then
    for datafile in "$COMMON_DATA"/*; do
        if [ -f "$datafile" ]; then
            cp "$datafile" "$EMU_DIR/"
        fi
    done
    echo "  [OK] common test data"
fi
```

- [ ] **Step 2: Verify install-emu.sh runs without errors**

```bash
make install-emu
```

Expected: output includes `[OK] common test data` alongside port binaries.

- [ ] **Step 3: Verify files appear in build/amiga/**

```bash
ls build/amiga/test-*.txt build/amiga/test-binary.dat
```

Expected: all 6 common test data files present.

- [ ] **Step 4: Commit**

```bash
git add scripts/install-emu.sh
git commit -m "feat: install-emu.sh copies common test data to WORK:"
```

---

## Task 3: Add CATEGORY variable to port Makefiles

The CI coverage gate needs to know each port's category to enforce the correct test minimum (CLI=8, scripting=10).

**Files:**
- Modify: `ports/common.mk`
- Modify: `ports/cal/Makefile`, `ports/cut/Makefile`, `ports/diff/Makefile`, `ports/grep/Makefile`, `ports/sed/Makefile`, `ports/tail/Makefile`, `ports/lua/Makefile`

- [ ] **Step 1: Add CATEGORY default to common.mk**

After line 28 (`AMINET_CAT ?= util/cli`), add:

```makefile
# Port category (1=CLI, 2=Scripting, 3=Console, 4=Network, 5=GUI)
CATEGORY ?= 1
```

- [ ] **Step 2: Set CATEGORY=2 in lua's Makefile**

In `ports/lua/Makefile`, add after the existing variable definitions:

```makefile
CATEGORY    = 2
```

All other ports default to CATEGORY=1 (CLI) via common.mk, which is correct.

- [ ] **Step 3: Verify CATEGORY is accessible from Make**

```bash
make -C ports/cal -p 2>/dev/null | grep CATEGORY
make -C ports/lua -p 2>/dev/null | grep CATEGORY
```

Expected: `CATEGORY = 1` for cal, `CATEGORY = 2` for lua.

- [ ] **Step 4: Commit**

```bash
git add ports/common.mk ports/lua/Makefile
git commit -m "feat: add CATEGORY variable to port Makefiles for CI coverage gate"
```

---

## Task 4: Create check-test-coverage.sh script

**Files:**
- Create: `scripts/check-test-coverage.sh`

- [ ] **Step 1: Write the script**

```bash
#!/bin/bash
#
# check-test-coverage.sh — Validate FS-UAE test suite completeness
#
# Checks every port's test-fsemu-cases.txt against the minimum coverage
# standard from docs/test-coverage-standard.md.
#
# Usage: make check-test-coverage
# Exit: 0 if all pass, 1 if any fail

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

FAIL=0
CHECKED=0

echo "=== Checking test coverage ==="

for dir in "$PROJECT_DIR"/ports/*/; do
    name=$(basename "$dir")
    cases="$dir/test-fsemu-cases.txt"
    makefile="$dir/Makefile"

    # Skip if no Makefile (not a real port)
    [ -f "$makefile" ] || continue

    CHECKED=$((CHECKED + 1))

    # Get category from Makefile (default 1)
    category=$(make -C "$dir" -p 2>/dev/null | grep '^CATEGORY' | head -1 | awk '{print $3}')
    category=${category:-1}

    # Set minimum test count by category
    case "$category" in
        1) min_tests=8 ;;
        2) min_tests=10 ;;
        3) min_tests=8 ;;
        4) min_tests=8 ;;
        *) min_tests=8 ;;
    esac

    # Check if test cases file exists
    if [ ! -f "$cases" ]; then
        echo "  FAIL: $name — no test-fsemu-cases.txt"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Count tests
    test_count=$(grep -c '^TEST:' "$cases" 2>/dev/null || echo 0)

    # Check for EXPECT_RC assertions
    has_rc0=$(grep -c 'EXPECT_RC: 0' "$cases" 2>/dev/null || echo 0)
    has_rc5=$(grep -c 'EXPECT_RC: 5' "$cases" 2>/dev/null || echo 0)
    has_rc10=$(grep -c 'EXPECT_RC: 10' "$cases" 2>/dev/null || echo 0)

    # Evaluate
    errors=""
    if [ "$test_count" -lt "$min_tests" ]; then
        errors="${errors}has $test_count tests (minimum: $min_tests); "
    fi
    if [ "$has_rc0" -eq 0 ] && [ "$has_rc5" -eq 0 ]; then
        errors="${errors}no success path test (EXPECT_RC: 0 or 5); "
    fi
    if [ "$has_rc10" -eq 0 ]; then
        errors="${errors}no error path test (EXPECT_RC: 10); "
    fi

    if [ -n "$errors" ]; then
        echo "  FAIL: $name — $errors"
        FAIL=$((FAIL + 1))
    else
        echo "  PASS: $name ($test_count tests, RC: 0=$has_rc0 5=$has_rc5 10=$has_rc10)"
    fi
done

echo ""
if [ $FAIL -gt 0 ]; then
    echo "FAIL: $FAIL of $CHECKED ports failed test coverage check"
    exit 1
else
    echo "PASS: all $CHECKED ports meet test coverage standard"
    exit 0
fi
```

- [ ] **Step 2: Make executable**

```bash
chmod +x scripts/check-test-coverage.sh
```

- [ ] **Step 3: Verify it correctly fails on current (inadequate) test suites**

```bash
bash scripts/check-test-coverage.sh
```

Expected: all 7 ports FAIL (none have EXPECT_RC assertions, most have < 8 tests).

- [ ] **Step 4: Commit**

```bash
git add scripts/check-test-coverage.sh
git commit -m "feat: check-test-coverage.sh validates FS-UAE test suite completeness"
```

---

## Task 5: Create check-fix-propagation.sh script

**Files:**
- Create: `scripts/check-fix-propagation.sh`

- [ ] **Step 1: Write the script**

```bash
#!/bin/bash
#
# check-fix-propagation.sh — Scan ports for known crash patterns
#
# Detects statically-greppable crash patterns from crash-patterns.md.
# Outputs warnings (informational, non-blocking).
#
# Usage: make check-fix-propagation
# Exit: always 0 (warnings only)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

WARNINGS=0
CHECKED=0

echo "=== Checking fix propagation ==="

for dir in "$PROJECT_DIR"/ports/*/; do
    name=$(basename "$dir")
    ported="$dir/ported"
    ignore_file="$dir/.fix-propagation-ignore"

    [ -d "$ported" ] || continue
    CHECKED=$((CHECKED + 1))

    # Load ignore patterns if present
    ignore_patterns=""
    if [ -f "$ignore_file" ]; then
        ignore_patterns=$(cat "$ignore_file")
    fi

    for src in "$ported"/*.c; do
        [ -f "$src" ] || continue
        base=$(basename "$src")

        # Check: exit() without _exit() — crash-patterns #9
        has_exit=$(grep -c 'exit(' "$src" 2>/dev/null || echo 0)
        has__exit=$(grep -c '_exit(' "$src" 2>/dev/null || echo 0)
        if [ "$has_exit" -gt 0 ] && [ "$has__exit" -eq 0 ]; then
            if ! echo "$ignore_patterns" | grep -q "exit_hang" 2>/dev/null; then
                echo "  WARN: $name/$base — has exit() but no _exit() (crash-patterns #9)"
                WARNINGS=$((WARNINGS + 1))
            fi
        fi

        # Check: missing __stack cookie — crash-patterns #7
        if ! grep -q '__stack' "$src" 2>/dev/null; then
            if ! echo "$ignore_patterns" | grep -q "stack_cookie" 2>/dev/null; then
                echo "  WARN: $name/$base — missing __stack cookie (crash-patterns #7)"
                WARNINGS=$((WARNINGS + 1))
            fi
        fi

        # Check: vsnprintf(NULL — crash-patterns #5
        if grep -q 'vsnprintf(NULL' "$src" 2>/dev/null; then
            if ! echo "$ignore_patterns" | grep -q "vsnprintf_null" 2>/dev/null; then
                echo "  WARN: $name/$base — vsnprintf(NULL) call (crash-patterns #5)"
                WARNINGS=$((WARNINGS + 1))
            fi
        fi

        # Check: exit(1) — non-Amiga exit code
        if grep -q 'exit(1)' "$src" 2>/dev/null; then
            if ! echo "$ignore_patterns" | grep -q "exit_code_1" 2>/dev/null; then
                echo "  WARN: $name/$base — exit(1) is invisible to Amiga scripts (use exit(10))"
                WARNINGS=$((WARNINGS + 1))
            fi
        fi
    done
done

echo ""
if [ $WARNINGS -gt 0 ]; then
    echo "Found $WARNINGS warning(s) across $CHECKED ports"
    echo "Suppress with ports/<name>/.fix-propagation-ignore (one pattern per line: exit_hang, stack_cookie, vsnprintf_null, exit_code_1)"
else
    echo "No known crash patterns found in $CHECKED ports"
fi
exit 0
```

- [ ] **Step 2: Make executable and verify**

```bash
chmod +x scripts/check-fix-propagation.sh
bash scripts/check-fix-propagation.sh
```

Expected: warnings for ports missing `_exit()` and possibly `__stack` cookies. Exit code 0 (non-blocking).

- [ ] **Step 3: Commit**

```bash
git add scripts/check-fix-propagation.sh
git commit -m "feat: check-fix-propagation.sh scans ports for known crash patterns"
```

---

## Task 6: Add new Make targets

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Add targets to Makefile**

Add before the `help` target's echo block, in the `.PHONY` line:

Add `check-test-coverage check-fix-propagation` to the `.PHONY` list.

Add the targets themselves (near the existing `check-docs` and `check-agents` targets):

```makefile
check-test-coverage:
	@bash scripts/check-test-coverage.sh

check-fix-propagation:
	@bash scripts/check-fix-propagation.sh
```

Also add to the `help` target echo block:

```
	@echo "  check-test-coverage  Validate FS-UAE test suite completeness"
	@echo "  check-fix-propagation  Scan ports for known crash patterns"
```

- [ ] **Step 2: Verify targets work**

```bash
make check-fix-propagation
make check-test-coverage || true  # Expected to fail on current suites
```

- [ ] **Step 3: Commit**

```bash
git add Makefile
git commit -m "feat: add check-test-coverage and check-fix-propagation Make targets"
```

---

## Task 7: Add crash-patterns rules to transformation-rules.md

**Files:**
- Modify: `.claude/skills/transform-source/references/transformation-rules.md`

- [ ] **Step 1: Append new section at end of file (after line 657)**

```markdown

## 9. Crash-Pattern Prevention Rules

Rules derived from `docs/references/crash-patterns.md`. Apply these AFTER all other
transformations to prevent known AmigaOS-specific bugs.

```c
/* RULE: Replace exit() with _exit() at the final return point of main()
 * (crash-patterns #9 — libnix exit() hangs on AmigaOS console)
 *
 * libnix's exit() calls atexit handlers that deadlock when closing
 * console-connected stdio streams. _exit() bypasses atexit and goes
 * straight to AmigaDOS Exit().
 *
 * IMPORTANT: Only change exit() at the final return point of main().
 * Do NOT change exit() in error paths, err()/errx() calls, or helper
 * functions — those still need atexit cleanup.
 */

// Before (at end of main()):
exit(0);
// After:
fflush(stdout); /* amiport: flush before _exit — no atexit handlers */
_exit(0); /* amiport: _exit to avoid libnix exit() hang (crash-patterns #9) */

// Before (at end of main() with variable):
exit(rval);
// After:
fflush(stdout);
_exit(rval); /* amiport: _exit to avoid libnix exit() hang (crash-patterns #9) */
```

```c
/* RULE: Add __stack cookie if missing (crash-patterns #7)
 *
 * Amiga default stack is 4KB. vamos defaults to 8KB and ignores __stack.
 * Most ported programs need 32KB+. Recursive programs (find, diff) need 64KB+.
 * Add near the top of the main source file, at file scope.
 */

// If no __stack declaration exists, add:
long __stack = 32768; /* amiport: stack cookie — Amiga default 4KB is too small */
// For recursive programs (grep -r, find, diff):
long __stack = 65536; /* amiport: stack cookie — extra for recursion */
```

```c
/* RULE: Replace vsnprintf(NULL, 0, ...) with probe buffer
 * (crash-patterns #5 — libnix vsnprintf crashes on NULL destination)
 *
 * C99 allows vsnprintf(NULL, 0, fmt, ap) to measure buffer size.
 * libnix does NOT support this — it writes to address zero.
 * Use a probe buffer instead.
 */

// Before:
int len = vsnprintf(NULL, 0, fmt, ap);
// After:
char probe[1024];
int len = vsnprintf(probe, sizeof(probe), fmt, ap);
/* amiport: probe buffer — libnix vsnprintf crashes on NULL (crash-patterns #5) */
```
```

- [ ] **Step 2: Verify the file is valid markdown**

```bash
wc -l .claude/skills/transform-source/references/transformation-rules.md
```

Expected: ~720 lines (657 original + ~60 new).

- [ ] **Step 3: Commit**

```bash
git add .claude/skills/transform-source/references/transformation-rules.md
git commit -m "feat: add crash-pattern prevention rules to transformation-rules.md"
```

---

## Task 8: Update code-transformer agent to read crash-patterns.md

**Files:**
- Modify: `.claude/agents/code-transformer.md`

- [ ] **Step 1: Add crash-patterns.md as mandatory reading**

In the code-transformer agent file, find the `## Reference Materials` or equivalent section listing documents the agent should consult. Add:

```markdown
- `docs/references/crash-patterns.md` — **Mandatory.** Known AmigaOS-specific crash patterns. Apply prevention rules from Section 9 of transformation-rules.md during every transformation.
```

If no reference section exists, add one after the Principles section:

```markdown
## Mandatory References

Before transforming any source, consult these documents:
- `.claude/skills/transform-source/references/transformation-rules.md` — All transformation rules including crash-pattern prevention (Section 9)
- `docs/references/crash-patterns.md` — Known crash patterns. Check if any apply to the code being transformed.
- `docs/references/newlib-availability.md` — What functions are available in libnix
```

- [ ] **Step 2: Commit**

```bash
git add .claude/agents/code-transformer.md
git commit -m "feat: code-transformer reads crash-patterns.md as mandatory reference"
```

---

## Task 9: Create test-designer agent

**Files:**
- Create: `.claude/agents/test-designer.md`

- [ ] **Step 1: Write the agent definition**

```markdown
---
name: test-designer
model: sonnet
description: Designs comprehensive FS-UAE test suites for ported programs. Analyzes source code, flags, exit codes, and error paths to generate test-fsemu-cases.txt files meeting the project's test coverage standard.
allowed-tools: Bash, Read, Write, Glob, Grep, WebFetch
---

You are a test suite designer for AmigaOS-ported programs. You analyze ported source code and generate comprehensive test suites that run inside FS-UAE via the ARexx test harness.

## Your Job

Given a port directory (`ports/<name>/`), produce:
1. A complete `test-fsemu-cases.txt` with 8+ tests (CLI) or 10+ (scripting)
2. All required test input files (`test-<name>-*.txt`)
3. A coverage report to stdout

## Test Case Format

Each test in `test-fsemu-cases.txt` uses this format:

```
TEST: description (what behavior is being tested)
CMD: WORK:<program> <args> [WORK:<inputfile>]
EXPECT: expected first-line output (exact match)
EXPECT_RC: expected Amiga return code (0, 5, 10, or 20)
```

Assertion types (can combine EXPECT + EXPECT_RC on same test):
- `EXPECT:` — exact match of first line of stdout
- `EXPECT_CONTAINS:` — substring match anywhere in output
- `EXPECT_RC:` — expected Amiga return code

Important: `WORK:` is the FS-UAE volume where binaries and test files are mounted. All paths in CMD must use `WORK:` prefix.

ARexx `ADDRESS COMMAND` does NOT support stdin piping. If a program reads from stdin, create a pre-built input file and pass it as a file argument instead.

## Source Analysis Methodology

Follow `docs/test-coverage-standard.md` "Deriving Test Cases" section:

1. **Flags:** Grep for `getopt`, option parsing, or flag variables → one test per flag
2. **Exit codes:** Grep for `exit(`, `_exit(`, `err(`, `errx(` → one test per distinct exit code
3. **Error messages:** Grep for `fprintf(stderr`, `warn(`, `warnx(` → one test per error path
4. **Edge cases:** Check `docs/references/crash-patterns.md` for applicable Amiga-specific issues
5. **Upstream tests:** If `ports/<name>/upstream-tests/` exists, adapt the top 10 most relevant cases (ranked by: flag-exercising > error path > edge case). Exclude tests requiring piping, multi-process, locale, or signals.

## Required Test Categories

Every test suite MUST include:

1. **Functional tests** — at least one per documented flag/option
2. **Error path tests** — at least one test with `EXPECT_RC: 10` (bad args, missing file, etc.)
3. **Exit code tests** — at least one each: `EXPECT_RC: 0` (success) and `EXPECT_RC: 5` (warning, if applicable) and `EXPECT_RC: 10` (error)
4. **Edge case tests** — empty file, long lines, binary file detection (where applicable)
5. **Amiga-specific tests** — verify Amiga path handling works (WORK: volume paths)

## Piping Detection

If the source reads from stdin when no file argument is given (grep for `read(STDIN_FILENO`, `fgets(.*stdin`, `getline`, `scanf`, `getchar` without preceding `fopen`), create a pre-built input file and pass it as a file argument. Comment the test: `# Uses input file instead of piping (ARexx limitation)`.

## Shared Test Data

Standard test input files are available at `WORK:` from `ports/common-test-data/`:
- `test-empty.txt` — 0 bytes
- `test-oneline.txt` — "hello world"
- `test-multiline.txt` — 10 lines of varied content
- `test-longline.txt` — >1024 char line with "MARKER" at end
- `test-binary.dat` — binary content
- `test-special-chars.txt` — tabs, quotes, backslashes

Use these for generic edge case tests. Create port-specific files as `ports/<name>/test-<name>-<purpose>.txt` when the shared files don't cover the need.

## Post-Generation Validation

After generating test-fsemu-cases.txt, verify:
1. Every `WORK:test-*.txt` or `WORK:test-*.dat` reference has a corresponding file in either `ports/<name>/` or `ports/common-test-data/`
2. Test count meets the minimum for the port's category
3. At least one test has `EXPECT_RC: 0` or `EXPECT_RC: 5`
4. At least one test has `EXPECT_RC: 10`
5. No tests use stdin piping (no `|` or `<` in CMD lines)

## Coverage Report

Print to stdout at the end:
```
=== Test Coverage Report: <name> ===
Category: <N> (CLI/Scripting/Console/Network)
Tests: <count> (minimum: <min>)
Functional: <count> tests
Error path: <count> tests (EXPECT_RC: 10)
Exit codes: RC0=<n> RC5=<n> RC10=<n>
Edge cases: <count> tests
Amiga-specific: <count> tests
Input files created: <count>
Shared data referenced: <count>
VERDICT: PASS / FAIL (reason)
```

## Reference Documents

- `docs/test-coverage-standard.md` — Required test categories and minimums
- `docs/references/crash-patterns.md` — Amiga-specific edge cases to test
- `toolchain/templates/test-runner.rexx` — The ARexx harness that runs the tests (understand its assertion logic)
```

- [ ] **Step 2: Verify agent frontmatter passes check**

```bash
make check-agents
```

Expected: test-designer passes frontmatter validation.

- [ ] **Step 3: Commit**

```bash
git add .claude/agents/test-designer.md
git commit -m "feat: test-designer agent for comprehensive FS-UAE test suite generation"
```

---

## Task 10: Add timeout diagnosis to test-fsemu.sh

**Files:**
- Modify: `scripts/test-fsemu.sh`

- [ ] **Step 1: Read the current parse_results and run_emulator functions**

Read `scripts/test-fsemu.sh` lines 290-400 to understand the current timeout and results handling.

- [ ] **Step 2: Add diagnose_timeout function**

Add this function before `parse_results()` (around line 368):

```bash
# ============================================================
# Diagnose timeout failures
# ============================================================
diagnose_timeout() {
    local port_dir="$1"
    local tap_file="$RESULTS_DIR/tap-output.txt"
    local sentinel="$RESULTS_DIR/tests-complete"
    local elapsed="$2"

    echo ""
    echo "=== Timeout Diagnosis ==="

    # Check 1: Did ARexx harness start at all?
    if [ ! -f "$tap_file" ] || [ ! -s "$tap_file" ]; then
        echo -e "${RED}DIAGNOSIS: ARexx harness did not start${NC}"
        echo "  RexxMast may not be running, or test-runner.rexx failed to launch."
        echo "  Check: Is SYS:System/RexxMast in the Startup-Sequence?"
        echo "  Check: Is test-runner.rexx copied to WORK:?"
        return 1
    fi

    # Check 2: Did TAP header get written but no results?
    local has_header=$(grep -c '^1\.\.' "$tap_file" 2>/dev/null || echo 0)
    local result_count=$(grep -cE '^ok |^not ok ' "$tap_file" 2>/dev/null || echo 0)
    local total=$(head -1 "$tap_file" | grep -oE '[0-9]+$' || echo 0)

    if [ "$has_header" -gt 0 ] && [ "$result_count" -eq 0 ]; then
        # Distinguish hang from immediate crash by elapsed time
        if [ "$elapsed" -lt 15 ]; then
            echo -e "${RED}DIAGNOSIS: Binary crashed immediately (FS-UAE exited in ${elapsed}s)${NC}"
            echo "  The program likely segfaulted on startup."
            echo "  Run with --debug to capture Enforcer hits."
        else
            echo -e "${YELLOW}DIAGNOSIS: First test command hung (timeout after ${elapsed}s)${NC}"
            echo "  Likely cause: libnix exit() hang (crash-patterns #9)"
            echo "  The ARexx harness started ($total tests parsed) but the first"
            echo "  command never returned."
            check_exit_hang_pattern "$port_dir"
        fi
        return 1
    fi

    if [ "$has_header" -gt 0 ] && [ "$result_count" -gt 0 ] && [ "$result_count" -lt "$total" ]; then
        local next_test=$((result_count + 1))
        echo -e "${YELLOW}DIAGNOSIS: Test $next_test of $total hung${NC}"
        echo "  Tests 1-$result_count completed. Test $next_test never returned."
        echo "  Check the CMD for test $next_test in test-fsemu-cases.txt."
        return 1
    fi

    # Check 3: Tests completed but UAEQuit failed?
    if [ -f "$sentinel" ]; then
        echo -e "${YELLOW}DIAGNOSIS: Tests completed but UAEQuit failed${NC}"
        echo "  The sentinel file exists — all tests ran. UAEQuit did not shut down FS-UAE."
        echo "  This is not a test failure — results are valid."
        return 0
    fi

    echo -e "${RED}DIAGNOSIS: Unknown timeout cause${NC}"
    echo "  TAP file exists but no clear failure pattern."
    echo "  FS-UAE log: $RESULTS_DIR/fsuae.log"
    return 1
}

# ============================================================
# Check if port has exit() hang pattern
# ============================================================
check_exit_hang_pattern() {
    local port_dir="$1"
    local ported_dir="$port_dir/ported"

    [ -d "$ported_dir" ] || return 1

    local has_exit=false
    local has__exit=false

    for src in "$ported_dir"/*.c; do
        [ -f "$src" ] || continue
        grep -q 'exit(' "$src" 2>/dev/null && has_exit=true
        grep -q '_exit(' "$src" 2>/dev/null && has__exit=true
    done

    if [ "$has_exit" = true ] && [ "$has__exit" = false ]; then
        echo ""
        echo -e "${YELLOW}  EXIT HANG PATTERN DETECTED:${NC}"
        echo "  Source has exit() but no _exit(). This matches crash-patterns #9."
        echo "  Fix: replace exit() at the end of main() with fflush(stdout); _exit();"
        echo ""

        # Self-healing: check if we should auto-fix
        if [ "${AMIPORT_SELF_HEAL_DEPTH:-0}" -eq 0 ]; then
            echo "  Self-healing available. Set AMIPORT_SELF_HEAL=1 to auto-fix and retry."
            # The calling code in main() handles the actual dispatch
            return 2  # Special return code: exit hang detected, self-heal possible
        else
            echo "  Self-healing already attempted (depth=$AMIPORT_SELF_HEAL_DEPTH). Escalating."
            return 1
        fi
    fi

    return 1
}
```

- [ ] **Step 3: Wire diagnosis into the main function's timeout path**

In the `main()` function, find the section after `run_emulator` (around line 640). Replace the timeout handling:

Find the block that starts with:
```bash
    if [ -f "$RESULTS_DIR/tests-complete" ]; then
```

Before that block, add elapsed time tracking:

```bash
    # Calculate elapsed time for diagnosis
    local elapsed=$(( $(date +%s) - START_TIME ))
```

Then update the `elif [ $emu_exit -ne 0 ]; then` block to call diagnosis:

```bash
    elif [ $emu_exit -ne 0 ]; then
        diagnose_timeout "$port_dir" "$elapsed"
        local diag_exit=$?
        parse_results 2>/dev/null || true
        generate_report "$port_dir"
        exit 1
```

- [ ] **Step 4: Test the diagnosis on cal (which has the exit hang)**

```bash
make test-fsemu TARGET=ports/cal 2>&1 | tail -20
```

Expected: Instead of just "TIMEOUT", should now show:
```
=== Timeout Diagnosis ===
DIAGNOSIS: First test command hung (timeout after 60s)
  Likely cause: libnix exit() hang (crash-patterns #9)
  ...
  EXIT HANG PATTERN DETECTED:
  Source has exit() but no _exit().
```

- [ ] **Step 5: Commit**

```bash
git add scripts/test-fsemu.sh
git commit -m "feat: timeout diagnosis in test-fsemu.sh — hang vs crash detection"
```

---

## Task 11: Roll crash-pattern fixes across all 7 ports (WS1)

This task dispatches the code-transformer agent sequentially for each port. **Must be done by dispatching the code-transformer agent** — direct edits to `ported/*.c` are blocked by `enforce-agents.sh`.

**Files:**
- Modify (via code-transformer): `ports/*/ported/*.c` for all 7 ports

- [ ] **Step 1: Audit current state — which ports need which fixes**

```bash
bash scripts/check-fix-propagation.sh
```

Note which ports have which warnings. This is the fix list.

- [ ] **Step 2: For each port (sequentially: cal, cut, diff, grep, lua, sed, tail), dispatch code-transformer**

Dispatch the `code-transformer` agent with this prompt template (substitute port name):

```
Apply crash-pattern prevention fixes to ports/<name>/ported/:

1. If the final exit() call in main() is exit(0) or exit(rval), replace with:
   fflush(stdout);
   _exit(0);  /* amiport: _exit to avoid libnix exit() hang (crash-patterns #9) */
   Do NOT change exit() calls in error paths, err()/errx() calls, or helper functions.

2. If no `long __stack` declaration exists at file scope, add:
   long __stack = 32768;  /* amiport: stack cookie — Amiga default 4KB is too small */
   (Use 65536 for diff, grep, sed which may recurse deeply)

3. If any vsnprintf(NULL, 0, ...) call exists, replace with probe buffer pattern per crash-patterns #5.

4. If any exit(1) or exit(EXIT_FAILURE) exists, replace with exit(10) per Amiga RETURN_ERROR convention.

Read docs/references/crash-patterns.md for full context. Read the source first to understand the program structure before making changes.
```

- [ ] **Step 3: After each port, rebuild and run vamos smoke test**

```bash
make build TARGET=ports/<name>
make test TARGET=ports/<name>
```

- [ ] **Step 4: Re-run fix propagation scanner to verify fixes were applied**

```bash
bash scripts/check-fix-propagation.sh
```

Expected: fewer warnings than Step 1. Ports with only `err()/errx()` exit calls may still show warnings (acceptable false positives).

- [ ] **Step 5: Commit all port fixes together**

```bash
git add ports/*/ported/
git commit -m "fix: roll crash-pattern fixes across all 7 ports (_exit, __stack, exit codes)"
```

---

## Task 12: Generate comprehensive test suites for all 7 ports (WS2)

Dispatch the test-designer agent for each port. Start with cal as a smoke test, then do the remaining 6.

- [ ] **Step 1: Dispatch test-designer for cal first**

Dispatch the `test-designer` agent with:

```
Generate a comprehensive FS-UAE test suite for ports/cal/.

Read the ported source at ports/cal/ported/cal.c.
Read docs/test-coverage-standard.md for the required test categories.
Read docs/references/crash-patterns.md for Amiga-specific edge cases.

Produce:
1. ports/cal/test-fsemu-cases.txt with 8+ tests covering: functional (flags), error paths, exit codes, edge cases, Amiga-specific
2. Any needed test input files as ports/cal/test-cal-*.txt
3. Print coverage report to stdout

Cal is a Category 1 (CLI) port. It displays calendars. Key flags to test: -y (full year), -j (Julian days), month/year arguments, no-args (current month).
Error cases: invalid month, invalid year, bad arguments.
```

- [ ] **Step 2: Verify cal's new test suite passes check-test-coverage**

```bash
bash scripts/check-test-coverage.sh 2>&1 | grep cal
```

Expected: `PASS: cal (8+ tests, RC: 0=N 5=0 10=N)`

- [ ] **Step 3: Run test-fsemu on cal to validate tests actually work**

```bash
make test-fsemu TARGET=ports/cal
```

Expected: cal should now pass (WS1 fixed the exit hang, WS2 generated real tests).

- [ ] **Step 4: Dispatch test-designer for remaining 6 ports**

Dispatch test-designer for each of: cut, diff, grep, lua, sed, tail. These can run in parallel as separate agent dispatches since they write to different port directories.

For each port, provide context in the prompt about what the tool does and its key flags/error conditions. Include the port category (lua = Category 2, all others = Category 1).

- [ ] **Step 5: Verify all 7 ports pass check-test-coverage**

```bash
bash scripts/check-test-coverage.sh
```

Expected: all 7 ports PASS.

- [ ] **Step 6: Commit all generated test suites**

```bash
git add ports/*/test-fsemu-cases.txt ports/*/test-*-*.txt
git commit -m "feat: comprehensive FS-UAE test suites for all 7 ports (test-designer agent)"
```

---

## Task 13: Validate FS-UAE testing end-to-end

Run `make test-fsemu` on all 7 ports sequentially to validate everything works.

- [ ] **Step 1: Test cal (simplest, validates pipeline)**

```bash
make test-fsemu TARGET=ports/cal
```

Expected: all tests pass, no timeout, clean exit.

- [ ] **Step 2: Test remaining 6 ports sequentially**

```bash
for port in cut diff grep lua sed tail; do
    echo "=== Testing $port ==="
    make test-fsemu TARGET=ports/$port || echo "FAILED: $port"
done
```

Expected: all pass. If any hang, the timeout diagnosis should identify the cause. If the self-healing detects an exit hang pattern, it reports it (self-healing is not yet wired to auto-dispatch — see Task 14).

- [ ] **Step 3: Fix any test failures**

If tests fail (wrong output, not hang/crash), update the test cases or fix the port source. This is iterative.

- [ ] **Step 4: Commit any fixes**

```bash
git add -A ports/
git commit -m "fix: FS-UAE test adjustments after end-to-end validation"
```

---

## Task 14: Wire self-healing dispatch into test-fsemu.sh

Now that WS2 test suites exist (Task 12) and WS3 diagnosis is in place (Task 10), it's safe to add self-healing.

**Files:**
- Modify: `scripts/test-fsemu.sh`

- [ ] **Step 1: Add self-healing logic to the main function**

In the `main()` function, after the `diagnose_timeout` call, add self-healing dispatch logic. When `diagnose_timeout` returns exit code 2 (exit hang detected), and `AMIPORT_SELF_HEAL_DEPTH` is 0:

```bash
        # Self-healing: if exit hang detected and not already retrying
        if [ "$diag_exit" -eq 2 ] && [ "${AMIPORT_SELF_HEAL:-0}" -eq 1 ]; then
            echo ""
            echo -e "${YELLOW}=== Self-Healing: Attempting _exit() fix ===${NC}"
            echo "  Stashing current state..."
            git stash push -m "self-heal-attempt-$(basename "$port_dir")" -- "$port_dir/ported/" 2>/dev/null || true

            echo "  NOTE: Self-healing requires dispatching code-transformer agent."
            echo "  In automated mode, this would dispatch the agent. For now, reporting:"
            echo "  FIX NEEDED: Replace exit() at end of main() with fflush(stdout); _exit();"
            echo "  in $(ls "$port_dir"/ported/*.c)"
            echo ""
            echo "  After fixing, re-run: make test-fsemu TARGET=$(echo "$port_dir" | sed "s|$PROJECT_DIR/||")"

            # Restore stash (fix was not applied automatically)
            git stash pop 2>/dev/null || true
        fi
```

Note: Full automatic self-healing (dispatching code-transformer from a bash script) requires Claude agent orchestration which happens at the pipeline level (port-project skill), not from a standalone bash script. The script reports the diagnosis and recommended fix.

- [ ] **Step 2: Test self-healing detection**

Create a temporary port with a known exit hang to test detection:

Actually, this should already be covered by Task 10's validation. If cal still had the exit hang, the diagnosis would have flagged it. Since WS1 (Task 11) fixed it, test by temporarily reverting cal's fix:

Skip this substep — self-healing detection was validated in Task 10.

- [ ] **Step 3: Commit**

```bash
git add scripts/test-fsemu.sh
git commit -m "feat: self-healing diagnosis in test-fsemu.sh reports exit hang fix instructions"
```

---

## Task 15: Add CI gates

**Files:**
- Modify: `.github/workflows/ci.yml`

- [ ] **Step 1: Add check-test-coverage step**

After the "Test production ports (vamos)" step (line 79), add:

```yaml
      - name: Check test coverage standard
        run: make check-test-coverage

      - name: Check fix propagation (informational)
        continue-on-error: true
        run: make check-fix-propagation
```

- [ ] **Step 2: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "feat: CI gates for test coverage and fix propagation"
```

---

## Task 16: Update port-project SKILL.md with test-designer dispatch

**Files:**
- Modify: `.claude/skills/port-project/SKILL.md`

- [ ] **Step 1: Add Stage 5b test-designer dispatch**

Find the Stage 5 section. After Stage 5a (vamos testing), add:

```markdown
**5b. Test suite generation (MANDATORY):**
Dispatch the `test-designer` agent to generate a comprehensive `test-fsemu-cases.txt`:

```
subagent_type: "test-designer"
prompt: "Generate a comprehensive FS-UAE test suite for ports/<name>/. Read the ported source, analyze flags/exit codes/error paths, and produce test-fsemu-cases.txt with 8+ tests (CLI) or 10+ tests (scripting). Include test input files as needed. Port category: <N>."
```

The test-designer reads the ported source, man pages, and upstream tests to generate a complete test suite meeting `docs/test-coverage-standard.md`. It runs BEFORE `make test-fsemu`.
```

- [ ] **Step 2: Add test-designer to the agent dispatch list**

In the "Agents dispatched" section, add:

```markdown
5. `test-designer` → comprehensive test suite generation (Stage 5b)
```

Renumber subsequent entries.

- [ ] **Step 3: Commit**

```bash
git add .claude/skills/port-project/SKILL.md
git commit -m "feat: add test-designer dispatch to port-project pipeline Stage 5b"
```

---

## Task 17: Update all project documentation

Per project rules, all affected docs must be updated. This task covers all documentation changes.

**Files:**
- Modify: `CLAUDE.md`, `README.md`, `docs/architecture.md`, `docs/porting-guide.md`, `TODOS.md`

- [ ] **Step 1: Update CLAUDE.md**

Add `test-designer` to the agent table:

```markdown
| `test-designer` | Stage 5b — comprehensive FS-UAE test suite generation |
```

Add new make targets to the Build Instructions section:

```markdown
make check-test-coverage  # Validate FS-UAE test suite completeness
make check-fix-propagation # Scan ports for known crash patterns (informational)
```

- [ ] **Step 2: Update README.md**

Add test-designer to the agents table. Add new make targets to the usage section.

- [ ] **Step 3: Update docs/architecture.md**

Add test-designer to the pipeline diagram. Update the agent list.

- [ ] **Step 4: Update docs/porting-guide.md**

Add the test-designer step to the porting workflow.

- [ ] **Step 5: Add deferred TODO to TODOS.md**

Add under the "FS-UAE Testing" section:

```markdown
### FS-UAE Integration Testing in CI

**What:** Run `make test-fsemu` in GitHub Actions for every push.

**Why:** Currently only vamos tests run in CI. FS-UAE tests catch bugs that vamos misses (exit() hang, console I/O issues, real AmigaOS library behavior).

**Details:** Requires FS-UAE installed in CI runner, Kickstart 3.1 ROM (licensing issue), headless mode (FS-UAE 4.0+ or Xvfb wrapper). May need a custom Docker image with FS-UAE + ROM. Consider running only on main branch (not PRs) to limit ROM exposure.

**Depends on:** All work in the FS-UAE test overhaul shipping first.

**Priority:** P2

**Effort:** L (human: ~1 week / CC: ~1 hour)
```

- [ ] **Step 6: Commit all documentation updates**

```bash
git add CLAUDE.md README.md docs/architecture.md docs/porting-guide.md TODOS.md
git commit -m "docs: update all docs for test-designer agent, CI gates, and test infrastructure"
```

---

## Task 18: Final validation and cleanup

- [ ] **Step 1: Run all CI checks locally**

```bash
make check-docs
make check-agents
make check-test-coverage
make check-fix-propagation
```

Expected: all pass (check-fix-propagation may show informational warnings for legitimate exit() usage in error paths).

- [ ] **Step 2: Run vamos tests to verify nothing broken**

```bash
make test-shim
make test-emu
make test-ports
```

- [ ] **Step 3: Verify no stray files in project root**

```bash
git status
```

Expected: only tracked changes in ports/, scripts/, .claude/, docs/, Makefile, .github/.

- [ ] **Step 4: Run FS-UAE on cal one final time**

```bash
make test-fsemu TARGET=ports/cal
```

Expected: clean pass with comprehensive test suite.

- [ ] **Step 5: Final commit if any cleanup needed**

```bash
git status  # Check for any uncommitted changes
```
