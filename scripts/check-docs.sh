#!/usr/bin/env bash
set -euo pipefail

# check-docs.sh — CI gate: validate documentation consistency
#
# Check 1: Agent cross-reference (bidirectional)
#   - Every .claude/agents/*.md agent name must appear in CLAUDE.md, README.md,
#     docs/architecture.md, AND .claude/rules/use-pipeline-agents.md
#   - Every agent name in use-pipeline-agents.md must have a matching agent file
#
# Check 2: Skill existence
#   - Every user-facing skill (SKILL.md with argument-hint:) must appear in CLAUDE.md
#
# Check 3: Shim function sync (bidirectional)
#   - Direction A (FAIL): amiport_* in transformation-rules.md must exist in headers
#   - Direction B (WARN): amiport_* in headers should be in transformation-rules.md
#
# Check 4: Duplication regression guard
#   - CLAUDE.md must NOT contain sections that belong in canonical rules files
#
# Exit 0 if all checks pass (WARNs don't cause failure), exit 1 if any FAIL.

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

failed=0
warned=0

# ---------------------------------------------------------------------------
# Check 1: Agent cross-reference (bidirectional)
# ---------------------------------------------------------------------------
echo "=== Check 1: Agent cross-reference ==="

AGENTS_DIR="$REPO_ROOT/.claude/agents"
RULES_FILE="$REPO_ROOT/.claude/rules/use-pipeline-agents.md"
CHECK_DOCS=(
    "$REPO_ROOT/CLAUDE.md"
    "$REPO_ROOT/README.md"
    "$REPO_ROOT/docs/architecture.md"
    "$RULES_FILE"
)

check1_fail=0

# Forward: every agent file must be referenced in all docs
for agent_file in "$AGENTS_DIR"/*.md; do
    agent=$(basename "$agent_file" .md)
    for doc in "${CHECK_DOCS[@]}"; do
        if ! grep -q "$agent" "$doc" 2>/dev/null; then
            echo "FAIL  agent '$agent' not found in $(basename "$doc")"
            check1_fail=1
        fi
    done
done

# Reverse: every agent name in use-pipeline-agents.md must have a matching file
# Pattern: backtick-name-backtick followed by " agent"
while IFS= read -r agent_ref; do
    agent_file="$AGENTS_DIR/${agent_ref}.md"
    if [ ! -f "$agent_file" ]; then
        echo "FAIL  use-pipeline-agents.md references '\`${agent_ref}\` agent' but .claude/agents/${agent_ref}.md does not exist"
        check1_fail=1
    fi
done < <(grep -oE '`[a-z][a-z-]+`[[:space:]]+agent' "$RULES_FILE" 2>/dev/null | grep -oE '`[a-z][a-z-]+`' | tr -d '`' | sort -u)

if [ "$check1_fail" -eq 0 ]; then
    agent_count=$(ls "$AGENTS_DIR"/*.md 2>/dev/null | wc -l | tr -d ' ')
    echo "PASS  all $agent_count agents cross-referenced correctly"
else
    failed=1
fi

echo ""

# ---------------------------------------------------------------------------
# Check 2: Skill existence
# ---------------------------------------------------------------------------
echo "=== Check 2: User-facing skill existence ==="

check2_fail=0

for skill_md in "$REPO_ROOT"/.claude/skills/*/SKILL.md; do
    if grep -q '^argument-hint:' "$skill_md" 2>/dev/null; then
        skill=$(basename "$(dirname "$skill_md")")
        if ! grep -q "$skill" "$REPO_ROOT/CLAUDE.md" 2>/dev/null; then
            echo "FAIL  skill '$skill' has argument-hint: (user-facing) but not found in CLAUDE.md"
            check2_fail=1
        fi
    fi
done

if [ "$check2_fail" -eq 0 ]; then
    skill_count=$(grep -l '^argument-hint:' "$REPO_ROOT"/.claude/skills/*/SKILL.md 2>/dev/null | wc -l | tr -d ' ')
    echo "PASS  all $skill_count user-facing skills referenced in CLAUDE.md"
else
    failed=1
fi

echo ""

# ---------------------------------------------------------------------------
# Check 3: Shim function sync (bidirectional)
# ---------------------------------------------------------------------------
echo "=== Check 3: Shim function sync ==="

TRANSFORM_RULES="$REPO_ROOT/.claude/skills/transform-source/references/transformation-rules.md"
SHIM_HEADERS_DIR="$REPO_ROOT/lib/posix-shim/include/amiport"

# Internal helpers to skip in direction B
INTERNAL_HELPERS="amiport_errno_from_ioerr amiport_expand_argv amiport_check_break amiport_fd_is_valid amiport_fd_to_fh amiport_exit amiport_free_argv amiport_map_errno amiport_glob_has_magic amiport_file_count amiport_file_entry amiport_files amiport_dirent amiport_optarg amiport_opterr amiport_optind amiport_optopt amiport_optreset amiport_glob_t"

# Extract amiport_* (not amiport_emu_*) from transformation-rules.md
rules_fns=$(grep -oE 'amiport_[a-zA-Z_]+' "$TRANSFORM_RULES" 2>/dev/null \
    | grep -v '^amiport_emu_' \
    | sort -u)

# Extract all amiport_* (not amiport_emu_*) from shim headers
header_fns=$(grep -rh -oE 'amiport_[a-zA-Z_]+' "$SHIM_HEADERS_DIR"/ 2>/dev/null \
    | grep -v '^amiport_emu_' \
    | sort -u)

check3_fail=0

# Direction A: rules → headers (FAIL if stale)
while IFS= read -r fn; do
    if ! echo "$header_fns" | grep -qx "$fn"; then
        echo "FAIL  transformation-rules.md references '$fn' but it does not exist in any shim header (stale reference)"
        check3_fail=1
    fi
done <<< "$rules_fns"

if [ "$check3_fail" -eq 0 ]; then
    rules_count=$(echo "$rules_fns" | grep -c . || true)
    echo "PASS  all $rules_count amiport_* functions in transformation-rules.md exist in shim headers"
else
    failed=1
fi

# Direction B: headers → rules (WARN if missing, skip internal helpers)
warn_count=0
while IFS= read -r fn; do
    # Skip internal helpers
    if echo "$INTERNAL_HELPERS" | grep -qw "$fn"; then
        continue
    fi
    if ! echo "$rules_fns" | grep -qx "$fn"; then
        echo "WARN  shim header defines '$fn' but it is not in transformation-rules.md (new function?)"
        warn_count=$((warn_count + 1))
        warned=1
    fi
done <<< "$header_fns"

if [ "$warn_count" -eq 0 ]; then
    header_count=$(echo "$header_fns" | grep -c . || true)
    echo "PASS  all $header_count shim header functions present in transformation-rules.md (or excluded as internal)"
fi

echo ""

# ---------------------------------------------------------------------------
# Check 4: Duplication regression guard
# ---------------------------------------------------------------------------
echo "=== Check 4: Duplication regression guard ==="

CLAUDE_MD="$REPO_ROOT/CLAUDE.md"

# Marker strings that belong in canonical rules files, not CLAUDE.md
MARKERS=(
    "fopen() macro collision"
    "vamos argv pointer"
    "vsnprintf(NULL"
    "pledge/unveil stubs"
    "### Epoch offset"
    "### __nowild for"
    "### vamos ignores __stack"
    "### st_dev/st_ino"
    "ANSI C89"
)

check4_fail=0

for marker in "${MARKERS[@]}"; do
    if grep -qF "$marker" "$CLAUDE_MD" 2>/dev/null; then
        echo "FAIL  CLAUDE.md contains '$marker' — this belongs in .claude/rules/, not CLAUDE.md"
        check4_fail=1
    fi
done

if [ "$check4_fail" -eq 0 ]; then
    echo "PASS  CLAUDE.md contains no duplicated pitfall/coding-standard sections"
else
    failed=1
fi

echo ""

# ---------------------------------------------------------------------------
# Check 5: Crash patterns ↔ known-pitfalls cross-reference
# ---------------------------------------------------------------------------
echo "=== Check 5: Crash patterns ↔ known-pitfalls sync ==="

CRASH_PATTERNS="$REPO_ROOT/docs/references/crash-patterns.md"
KNOWN_PITFALLS="$REPO_ROOT/.claude/rules/known-pitfalls.md"

check5_warn=0

# Extract crash pattern numbers (## N. Title)
if [ -f "$CRASH_PATTERNS" ]; then
    while IFS= read -r title; do
        # Extract number and short identifier
        num=$(echo "$title" | grep -oE '^## [0-9]+' | grep -oE '[0-9]+')
        # Check if crash-patterns #N is mentioned in known-pitfalls
        if [ -n "$num" ] && ! grep -q "crash-patterns.*#$num\|crash.patterns.*#$num\|#$num\b" "$KNOWN_PITFALLS" 2>/dev/null; then
            # Not all crash patterns need a pitfall entry, but warn about recent ones (>= #5)
            if [ "$num" -ge 5 ] 2>/dev/null; then
                echo "WARN  crash-patterns #$num exists but is not referenced in known-pitfalls.md"
                check5_warn=1
                warned=1
            fi
        fi
    done < <(grep -E '^## [0-9]+\.' "$CRASH_PATTERNS" 2>/dev/null)
fi

if [ "$check5_warn" -eq 0 ]; then
    pattern_count=$(grep -cE '^## [0-9]+\.' "$CRASH_PATTERNS" 2>/dev/null || echo 0)
    echo "PASS  all crash patterns (>= #5) referenced in known-pitfalls.md"
fi

echo ""

# ---------------------------------------------------------------------------
# Check 6: Shim exports ↔ porting-guide sync
# ---------------------------------------------------------------------------
echo "=== Check 6: Key shim features in porting-guide ==="

PORTING_GUIDE="$REPO_ROOT/docs/porting-guide.md"

check6_warn=0

# Key shim features that should be mentioned in the porting guide
KEY_FEATURES=(
    "expand_argv:argv expansion"
    "__progname:__progname"
    "check_break:Ctrl-C"
)

for entry in "${KEY_FEATURES[@]}"; do
    feature="${entry%%:*}"
    label="${entry##*:}"
    if ! grep -qi "$label" "$PORTING_GUIDE" 2>/dev/null; then
        echo "WARN  shim provides '$feature' but porting-guide.md does not mention '$label'"
        check6_warn=1
        warned=1
    fi
done

if [ "$check6_warn" -eq 0 ]; then
    echo "PASS  key shim features documented in porting-guide.md"
fi

echo ""

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
if [ "$failed" -ne 0 ] || [ "$warned" -ne 0 ]; then
    if [ "$failed" -ne 0 ] && [ "$warned" -ne 0 ]; then
        echo "RESULT: FAIL (failures and warnings present)"
    elif [ "$failed" -ne 0 ]; then
        echo "RESULT: FAIL"
    else
        echo "RESULT: PASS with warnings"
    fi
else
    echo "RESULT: PASS — all checks clean"
fi

if [ "$failed" -ne 0 ]; then
    exit 1
fi
