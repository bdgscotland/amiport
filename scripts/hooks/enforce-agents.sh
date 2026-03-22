#!/bin/bash
# PreToolUse hook: Remind about agent usage for porting work
#
# Warns when editing ported/ C source files directly, as a reminder to
# use pipeline agents (code-transformer, debug-agent, etc.) instead.
#
# This is warn-only (exit 0), not blocking (exit 2), because:
# - Subagents dispatched via the Agent tool use the same Write/Edit tools
# - There is no reliable env var or process signal to distinguish a
#   code-transformer subagent write from a manual Claude write
# - Blocking would break the pipeline's own agents
#
# The real enforcement comes from CLAUDE.md rules, the agent dispatch table
# in .claude/rules/use-pipeline-agents.md, and the /port-project skill's
# GATE checks.

INPUT=$(cat)
TOOL=$(echo "$INPUT" | jq -r '.tool_name // empty')
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // .tool_input.path // empty')

# Only check Edit and Write tools
case "$TOOL" in
  Edit|Write) ;;
  *) exit 0 ;;
esac

# Only check ported C source files in port directories
if ! echo "$FILE_PATH" | grep -qE 'ports/[^/]+/ported/.*\.(c|h)$'; then
  exit 0
fi

# Warn (but allow) — agents and manual edits both pass through
cat >&2 << 'MSG'
REMINDER: Edits to ported/ source files should use pipeline agents.

Preferred workflow:
  - To transform source: dispatch the code-transformer agent or use /transform-source
  - To fix a crash: dispatch the debug-agent or use /debug-amiga
  - To port from scratch: use /port-project

If this edit is coming from a dispatched agent, carry on.
MSG
exit 0
