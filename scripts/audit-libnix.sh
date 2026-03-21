#!/bin/bash
# audit-libnix.sh — Verify newlib-availability.md against actual toolchain libraries
#
# Extracts function symbols from the libnix .a files inside the Docker toolchain
# and compares against docs/references/newlib-availability.md to find drift:
#   - Functions we think are Missing but libnix actually provides (false negatives)
#   - Functions we think are Available but are actually missing (false positives)
#
# Usage: ./scripts/audit-libnix.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
REF_DOC="$ROOT/docs/references/newlib-availability.md"
TOOLCHAIN_SCRIPT="$ROOT/toolchain/scripts/m68k-amigaos-gcc"

# Colors
if [ -t 1 ] && command -v tput >/dev/null 2>&1 && [ "$(tput colors 2>/dev/null || echo 0)" -ge 8 ]; then
    BOLD=$(tput bold); RED=$(tput setaf 1); GREEN=$(tput setaf 2); YELLOW=$(tput setaf 3); RESET=$(tput sgr0)
else
    BOLD=""; RED=""; GREEN=""; YELLOW=""; RESET=""
fi

echo "${BOLD}=== libnix Audit: Docker .a vs newlib-availability.md ===${RESET}"
echo ""

# Check prerequisites
if [ ! -f "$REF_DOC" ]; then
    echo "${RED}ERROR: $REF_DOC not found${RESET}" >&2
    exit 1
fi

if ! command -v docker >/dev/null 2>&1; then
    echo "${RED}ERROR: Docker not found. The toolchain runs inside Docker.${RESET}" >&2
    exit 1
fi

# Extract function symbols from libnix inside Docker
# The toolchain image has libnix at /opt/amiga/m68k-amigaos/lib/
echo "Extracting symbols from Docker toolchain image..."

DOCKER_IMAGE="ghcr.io/bdgscotland/amiport-toolchain:latest"
LIBNIX_PATHS="/opt/amiga/m68k-amigaos/lib/libnix/libc.a /opt/amiga/m68k-amigaos/lib/libnix/libm.a"

# Run nm inside Docker and extract defined (T/t) function symbols
DOCKER_SYMBOLS=$(docker run --rm "$DOCKER_IMAGE" sh -c \
    "for lib in $LIBNIX_PATHS; do [ -f \"\$lib\" ] && nm \"\$lib\" 2>/dev/null; done" \
    | grep ' [Tt] ' \
    | awk '{print $3}' \
    | sed 's/^_//' \
    | sort -u) || {
    echo "${RED}ERROR: Failed to extract symbols from Docker image.${RESET}" >&2
    echo "Is the toolchain image available? Try: make setup-toolchain" >&2
    exit 1
}

SYMBOL_COUNT=$(echo "$DOCKER_SYMBOLS" | wc -l | tr -d ' ')
echo "Found ${BOLD}$SYMBOL_COUNT${RESET} symbols in libnix"
echo ""

# Parse newlib-availability.md for function statuses
# Extract lines matching "| `funcname` | Status |" pattern
AVAILABLE_FUNCS=()
MISSING_FUNCS=()
PARTIAL_FUNCS=()

while IFS= read -r line; do
    # Match table rows: | `funcname` | Available/Missing/Partial/Use Shim |
    if echo "$line" | grep -qE '^\| `[a-z_]+' ; then
        func=$(echo "$line" | sed -E 's/^\| `([a-z_]+[^`]*)`.*$/\1/' | tr -d '`' | awk '{print $1}')
        status=$(echo "$line" | awk -F'|' '{print $3}' | xargs)
        case "$status" in
            Available*) AVAILABLE_FUNCS+=("$func") ;;
            Missing*)   MISSING_FUNCS+=("$func") ;;
            Partial*)   PARTIAL_FUNCS+=("$func") ;;
            "Use Shim"*) ;; # Intentionally shimmed, skip
        esac
    fi
done < "$REF_DOC"

echo "Reference doc: ${#AVAILABLE_FUNCS[@]} Available, ${#MISSING_FUNCS[@]} Missing, ${#PARTIAL_FUNCS[@]} Partial"
echo ""

# Check: functions marked Missing that libnix actually provides
echo "${BOLD}=== False Negatives (marked Missing but found in libnix) ===${RESET}"
FALSE_NEG=0
for func in "${MISSING_FUNCS[@]}"; do
    if echo "$DOCKER_SYMBOLS" | grep -qw "$func"; then
        echo "  ${GREEN}FOUND${RESET}: $func — libnix has it, but we think it's Missing"
        FALSE_NEG=$((FALSE_NEG + 1))
    fi
done
[ "$FALSE_NEG" -eq 0 ] && echo "  (none — reference doc is accurate)"
echo ""

# Check: functions marked Available that libnix does NOT provide
echo "${BOLD}=== False Positives (marked Available but NOT found in libnix) ===${RESET}"
FALSE_POS=0
for func in "${AVAILABLE_FUNCS[@]}"; do
    if ! echo "$DOCKER_SYMBOLS" | grep -qw "$func"; then
        echo "  ${RED}MISSING${RESET}: $func — we think it's Available, but libnix doesn't have it"
        FALSE_POS=$((FALSE_POS + 1))
    fi
done
[ "$FALSE_POS" -eq 0 ] && echo "  (none — reference doc is accurate)"
echo ""

# Check: Partial functions
echo "${BOLD}=== Partial Functions (verify manually) ===${RESET}"
for func in "${PARTIAL_FUNCS[@]}"; do
    if echo "$DOCKER_SYMBOLS" | grep -qw "$func"; then
        echo "  ${YELLOW}PRESENT${RESET}: $func — symbol exists (caveats may still apply)"
    else
        echo "  ${RED}ABSENT${RESET}: $func — marked Partial but symbol not found"
    fi
done
echo ""

# Summary
echo "${BOLD}=== Summary ===${RESET}"
echo "False negatives (Missing but available): $FALSE_NEG"
echo "False positives (Available but missing): $FALSE_POS"
echo "Docker image: $DOCKER_IMAGE"
echo ""

if [ "$FALSE_NEG" -gt 0 ] || [ "$FALSE_POS" -gt 0 ]; then
    echo "${YELLOW}Drift detected. Update docs/references/newlib-availability.md${RESET}"
    exit 2
else
    echo "${GREEN}Reference doc matches toolchain. No drift.${RESET}"
fi
