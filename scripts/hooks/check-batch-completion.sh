#!/bin/bash
# PreToolUse hook for Bash: fires when staging/committing port files.
# Checks that PORT.md files being committed are not stubs, and warns
# if new port binaries are being committed without LHA packages.
#
# Only fires on git add/commit commands that include port files.

TOOL_INPUT="$TOOL_INPUT_RAW"

# Only check git commit and git add commands
echo "$TOOL_INPUT" | grep -qE "git (commit|add).*ports/" || exit 0

# Check for stub PORT.md in staged files
STAGED=$(git diff --cached --name-only 2>/dev/null || true)
if [ -z "$STAGED" ]; then
    exit 0
fi

# Check PORT.md quality for any staged port PORT.md files
STUB_PORTS=""
for portmd in $(echo "$STAGED" | grep -E '^ports/[^/]+/PORT\.md$'); do
    if [ -f "$portmd" ]; then
        lines=$(wc -l < "$portmd")
        has_placeholder=$(grep -cE 'To be filled|To be documented' "$portmd" 2>/dev/null || true)
        if [ "$lines" -lt 60 ] || [ "$has_placeholder" -gt 0 ]; then
            STUB_PORTS="$STUB_PORTS $portmd($lines lines)"
        fi
    fi
done

if [ -n "$STUB_PORTS" ]; then
    echo "WARNING: Stub PORT.md files staged for commit:$STUB_PORTS"
    echo "Run PORT.md enrichment before committing. See .claude/rules/batch-completion-checklist.md"
fi

# Check for binaries without LHA packages
for binary in $(echo "$STAGED" | grep -E '^ports/[^/]+/[^/]+$' | grep -v '\.' | grep -v '/ported/' | grep -v '/original/'); do
    port_dir=$(dirname "$binary")
    port_name=$(basename "$binary")
    lha_count=$(ls "$port_dir"/*.lha 2>/dev/null | wc -l)
    if [ "$lha_count" -eq 0 ]; then
        echo "WARNING: Binary $binary staged but no LHA package in $port_dir/"
        echo "Run 'make package' before committing. See .claude/rules/batch-completion-checklist.md"
    fi
done

exit 0
