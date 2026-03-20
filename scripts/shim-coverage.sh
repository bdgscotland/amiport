#!/bin/bash
# amiport shim-coverage — report which amiport_* functions are tested/exercised

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SHIM_INCLUDE="$ROOT/lib/posix-shim/include/amiport"
TEST_DIR="$ROOT/tests/shim"
EXAMPLES_DIR="$ROOT/examples"

# Color support
if [ -t 1 ] && command -v tput >/dev/null 2>&1 && [ "$(tput colors 2>/dev/null || echo 0)" -ge 8 ]; then
    BOLD=$(tput bold); RESET=$(tput sgr0)
else
    BOLD=""; RESET=""
fi

echo "${BOLD}=== amiport shim coverage ===${RESET}"
printf "%-26s %-12s %s\n" "FUNCTION" "UNIT TEST" "EXAMPLE"

TOTAL=0
TESTED=0
EXAMPLED=0

# Extract all amiport_* function declarations from headers
FUNCS=$(grep -rhoE 'amiport_[a-z_]+' "$SHIM_INCLUDE" | sort -u)

for func in $FUNCS; do
    TOTAL=$((TOTAL + 1))

    # Check unit tests
    test_hit="-"
    if [ -d "$TEST_DIR" ] && grep -rql "$func" "$TEST_DIR" 2>/dev/null; then
        test_hit="YES"
        TESTED=$((TESTED + 1))
    fi

    # Check examples — find which example directory uses it
    example_hit="-"
    if [ -d "$EXAMPLES_DIR" ]; then
        match=$(grep -rl "$func" "$EXAMPLES_DIR" 2>/dev/null | head -1 || true)
        if [ -n "$match" ]; then
            # Extract the example name from the path (first directory under examples/)
            example_hit=$(echo "$match" | sed "s|$EXAMPLES_DIR/||" | cut -d/ -f1)
            EXAMPLED=$((EXAMPLED + 1))
        fi
    fi

    printf "%-26s %-12s %s\n" "$func" "$test_hit" "$example_hit"
done

echo ""
if [ "$TOTAL" -gt 0 ]; then
    TEST_PCT=$((TESTED * 100 / TOTAL))
    EXAM_PCT=$((EXAMPLED * 100 / TOTAL))
else
    TEST_PCT=0
    EXAM_PCT=0
fi
echo "${BOLD}Coverage: ${TESTED}/${TOTAL} functions tested (${TEST_PCT}%)${RESET}"
echo "${BOLD}         ${EXAMPLED}/${TOTAL} functions in examples (${EXAM_PCT}%)${RESET}"
