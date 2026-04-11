#!/usr/bin/env bash
# PostToolUse hook for Edit|Write: auto-sync data/catalog.json to site/data/catalog.json
# Fires after any edit to the project-level catalog, keeping the site copy in sync.
# This prevents the pre-commit hook from blocking commits due to catalog drift.

FILE="${TOOL_INPUT_file_path:-}"

# Only trigger on the project-level catalog (not the site copy)
case "$FILE" in
    */data/catalog.json)
        # Skip if this IS the site copy
        echo "$FILE" | grep -q "site/data/catalog.json" && exit 0
        ;;
    *) exit 0 ;;
esac

# Sync to site
if [ -f "data/catalog.json" ] && [ -d "site/data" ]; then
    cp data/catalog.json site/data/catalog.json
    echo "Auto-synced data/catalog.json → site/data/catalog.json"
fi

exit 0
