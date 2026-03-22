#!/usr/bin/env bash
set -euo pipefail

# check-fix-propagation.sh — Scan ported source files for known crash patterns
# from docs/references/crash-patterns.md. Informational only (always exits 0).

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORTS_DIR="$REPO_ROOT/ports"

warn_count=0
port_count=0

is_suppressed() {
    local port_dir="$1"
    local pattern="$2"
    local ignore_file="$port_dir/.fix-propagation-ignore"
    if [[ -f "$ignore_file" ]]; then
        grep -qx "$pattern" "$ignore_file" 2>/dev/null && return 0
    fi
    return 1
}

warn() {
    echo "WARN: $1 — $2"
    warn_count=$((warn_count + 1))
}

for port_dir in "$PORTS_DIR"/*/; do
    port_name="$(basename "$port_dir")"
    ported_dir="$port_dir/ported"

    # Skip ports without a ported/ directory and the templates directory
    [[ -d "$ported_dir" ]] || continue
    [[ "$port_name" == "templates" ]] && continue

    port_has_warning=0

    for src_file in "$ported_dir"/*.c; do
        [[ -f "$src_file" ]] || continue
        file_label="$port_name/$(basename "$src_file")"
        contents="$(cat "$src_file")"

        # 1. exit() without _exit() — crash-patterns #9 (libnix exit hang)
        if ! is_suppressed "$port_dir" "exit_hang"; then
            if echo "$contents" | grep -q 'exit(' && ! echo "$contents" | grep -q '_exit('; then
                warn "$file_label" "uses exit() without _exit() — libnix exit hang (crash-patterns #9)"
                port_has_warning=1
            fi
        fi

        # 2. Missing __stack cookie — crash-patterns #7
        if ! is_suppressed "$port_dir" "stack_cookie"; then
            if ! echo "$contents" | grep -q '__stack'; then
                warn "$file_label" "missing __stack cookie — stack overflow risk (crash-patterns #7)"
                port_has_warning=1
            fi
        fi

        # 3. vsnprintf(NULL — crash-patterns #5
        if ! is_suppressed "$port_dir" "vsnprintf_null"; then
            if echo "$contents" | grep -q 'vsnprintf(NULL'; then
                warn "$file_label" "vsnprintf(NULL, ...) crashes on libnix (crash-patterns #5)"
                port_has_warning=1
            fi
        fi

        # 4. exit(1) — non-Amiga exit code
        if ! is_suppressed "$port_dir" "exit_code_1"; then
            if echo "$contents" | grep -q 'exit(1)'; then
                warn "$file_label" "exit(1) is invisible to Amiga shells — use exit(10) for RETURN_ERROR"
                port_has_warning=1
            fi
        fi
    done

    if [[ $port_has_warning -eq 1 ]]; then
        port_count=$((port_count + 1))
    fi
done

echo ""
if [[ $warn_count -gt 0 ]]; then
    echo "Found $warn_count warning(s) across $port_count port(s)"
else
    echo "No known crash patterns found"
fi

exit 0
