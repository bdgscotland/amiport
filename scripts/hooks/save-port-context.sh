#!/bin/bash
# PreCompact hook: Log compaction event and preserve porting context.
# Writes a breadcrumb so post-compaction Claude knows what was happening.
INPUT=$(cat)
TRIGGER=$(echo "$INPUT" | jq -r '.trigger // "unknown"')
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

# Find any in-progress ports (directories with ported/ but no committed binary)
ACTIVE_PORTS=""
PROJECT_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || echo "/Users/duncan/Developer/amiport")
for port_dir in "$PROJECT_ROOT"/ports/*/; do
  [ -d "${port_dir}ported" ] || continue
  PORT_NAME=$(basename "$port_dir")
  # Skip templates
  [ "$PORT_NAME" = "templates" ] && continue
  ACTIVE_PORTS="${ACTIVE_PORTS}${PORT_NAME}, "
done

# Inject context about active ports back into the conversation
if [ -n "$ACTIVE_PORTS" ]; then
  ACTIVE_PORTS="${ACTIVE_PORTS%, }"
  echo "{\"systemMessage\":\"Context compaction ($TRIGGER) at $TIMESTAMP. Active ports: $ACTIVE_PORTS. Check PORT.md in each for current status.\"}"
fi

exit 0
