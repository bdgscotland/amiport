#!/bin/bash
#
# check-aminet.sh — Check if our ports appear on Aminet and track downloads
#
# Searches aminet.net for each port by checking the package page.
# Reports publication status and download counts.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Load identity
if [ -f "$PROJECT_DIR/.env" ]; then
    . "$PROJECT_DIR/.env"
fi

SEARCH_TERM="${AMINET_NAME:-Duncan Bowring}"

echo "=== Checking Aminet for amiport publications ==="
echo "    (searching for: \"$SEARCH_TERM\")"
echo ""

total_downloads=0
published=0
not_published=0

for dir in "$PROJECT_DIR"/ports/*/; do
    name=$(basename "$dir")
    [ -f "$dir/Makefile" ] || continue

    # Get version and category from Makefile
    version=$(grep "^VERSION" "$dir/Makefile" 2>/dev/null | head -1 | sed 's/^VERSION[[:space:]]*=[[:space:]]*//' || echo "1.0")
    aminet_cat=$(grep "^AMINET_CAT" "$dir/Makefile" 2>/dev/null | head -1 | sed 's/^AMINET_CAT[[:space:]]*=[[:space:]]*//' || echo "util/cli")

    printf "  %-15s " "$name"

    # Try versioned package name first, then plain name
    found=false
    for pkg_name in "${name}-${version}" "${name}"; do
        for cat in "$aminet_cat" "util/cli" "dev/lang" "util/misc"; do
            page_url="https://aminet.net/package/${cat}/${pkg_name}"
            page_content=$(curl -s -L --max-time 10 "$page_url" 2>/dev/null || echo "")

            if echo "$page_content" | grep -qi "$SEARCH_TERM" 2>/dev/null; then
                # Extract download count
                downloads=$(echo "$page_content" | grep -oP 'Downloads:\s*\K[0-9]+' 2>/dev/null || echo "?")
                if [ "$downloads" = "?" ]; then
                    # Try alternate pattern
                    downloads=$(echo "$page_content" | grep -oE '[0-9]+ downloads' 2>/dev/null | grep -oE '[0-9]+' || echo "?")
                fi

                echo "LIVE  ${cat}/${pkg_name}  (${downloads} downloads)"
                if [ "$downloads" != "?" ]; then
                    total_downloads=$((total_downloads + downloads))
                fi
                published=$((published + 1))
                found=true
                break 2
            fi
        done
    done

    if [ "$found" = false ]; then
        echo "NOT ON AMINET"
        not_published=$((not_published + 1))
    fi
done

echo ""
echo "=== Summary ==="
echo "  Published:     $published"
echo "  Not published: $not_published"
echo "  Total downloads: $total_downloads"
echo ""
echo "Note: New uploads take up to 24 hours to appear after moderator review."
