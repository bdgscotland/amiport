#!/bin/bash
#
# check-aminet.sh — Check if our ports appear on Aminet
#
# Searches aminet.net for each port by checking the .readme file
# for our uploader identity (from .env). This distinguishes our
# uploads from other packages with the same name.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Load identity
if [ -f "$PROJECT_DIR/.env" ]; then
    . "$PROJECT_DIR/.env"
fi

SEARCH_TERM="${AMINET_NAME:-amiport}"

echo "=== Checking Aminet for amiport publications ==="
echo "    (searching for: \"$SEARCH_TERM\")"
echo ""

for dir in "$PROJECT_DIR"/ports/*/; do
    name=$(basename "$dir")
    [ -f "$dir/Makefile" ] || continue

    printf "  %-15s " "$name"

    # Check if the specific readme exists on Aminet
    # Aminet readmes are at: aminet.net/package/util/cli/cal-1.0
    # Try to fetch the readme and look for our identity
    readme_url="https://aminet.net/util/cli/${name}.readme"
    readme_content=$(curl -s -L "$readme_url" 2>/dev/null || echo "")

    if echo "$readme_content" | grep -qi "$SEARCH_TERM" 2>/dev/null; then
        echo "PUBLISHED (ours)"
        # Extract the package page URL
        echo "              https://aminet.net/package/util/cli/${name}"
    elif echo "$readme_content" | grep -qi "Short:" 2>/dev/null; then
        # A readme exists but it's not ours
        echo "EXISTS (not ours — different uploader)"
    else
        # Try searching with version suffix
        version=$(grep "^VERSION" "$dir/Makefile" 2>/dev/null | head -1 | sed 's/^VERSION[[:space:]]*=[[:space:]]*//' || echo "1.0")
        readme_url2="https://aminet.net/util/cli/${name}-${version}.readme"
        readme_content2=$(curl -s -L "$readme_url2" 2>/dev/null || echo "")

        if echo "$readme_content2" | grep -qi "$SEARCH_TERM" 2>/dev/null; then
            echo "PUBLISHED (ours)"
            echo "              https://aminet.net/package/util/cli/${name}-${version}"
        elif echo "$readme_content2" | grep -qi "Short:" 2>/dev/null; then
            echo "EXISTS (not ours)"
        else
            echo "NOT ON AMINET"
        fi
    fi
done

echo ""
echo "Note: New uploads take up to 24 hours to appear after moderator review."
