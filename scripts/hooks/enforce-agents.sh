#!/bin/bash
# PreToolUse hook: Enforce agent usage for porting work
#
# Blocks direct edits to ported/ C source files unless the edit is coming
# from within an agent dispatch (code-transformer, debug-agent, etc.)
#
# The hook checks for a sentinel environment variable AMIPORT_AGENT_ACTIVE
# which is set by the port-project skill when dispatching agents.
# When Claude tries to manually edit ported/*.c without going through
# the pipeline agents, this hook blocks it with guidance.
#
# Note: This is a safety net, not a perfect gate. Claude can't set env vars
# itself, but subagents inherit the parent's environment. The real enforcement
# comes from the CLAUDE.md rules + this hook catching obvious violations.

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

# Allow if an agent is active (subagent dispatch sets this)
if [ "${AMIPORT_AGENT_ACTIVE:-}" = "1" ]; then
  exit 0
fi

# Block with guidance
cat >&2 << 'MSG'
BLOCKED: Direct edits to ported/ source files are not allowed.

Use the pipeline agents instead:
  - To transform source: dispatch the code-transformer agent or use /transform-source
  - To fix a crash: dispatch the debug-agent or use /debug-amiga
  - To port from scratch: use /port-project

The agents apply consistent transformation rules, document changes with
/* amiport: */ comments, run stub value impact analysis, and update the
crash patterns knowledge base.

If you need to make a manual fix (e.g., a one-line tweak the agent can't handle),
ask the user to approve the edit first.
MSG
exit 2
