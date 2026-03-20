#!/bin/bash
#
# check-aminet.sh — Check if our ports appear on Aminet
#
# Searches aminet.net for each port and reports whether it's published.
# Updates PORTS.md tracking table.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=== Checking Aminet for amiport publications ==="
echo ""

for dir in "$PROJECT_DIR"/ports/*/; do
    name=$(basename "$dir")
    [ -f "$dir/Makefile" ] || continue

    printf "  %-15s " "$name"

    # Search Aminet for our package
    result=$(curl -s "https://aminet.net/search?query=${name}&path[]=util" 2>/dev/null || echo "")

    if echo "$result" | grep -qi "amiport\|bowring" 2>/dev/null; then
        echo "PUBLISHED"
        # Try to extract the URL
        url=$(echo "$result" | grep -oi "href=\"/package/[^\"]*${name}[^\"]*\"" | head -1 | sed 's/href="//;s/"//' || true)
        if [ -n "$url" ]; then
            echo "              https://aminet.net$url"
        fi
    elif echo "$result" | grep -qi "$name" 2>/dev/null; then
        echo "FOUND (but may not be ours — check manually)"
    else
        echo "NOT FOUND"
    fi
done

echo ""
echo "Note: New uploads take up to 24 hours to appear after moderator review."
