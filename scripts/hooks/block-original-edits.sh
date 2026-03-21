#!/bin/bash
# PreToolUse hook: Block edits to ports/*/original/ directories
# These contain upstream source and should never be modified by agents.
INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // .tool_input.path // empty')

if echo "$FILE_PATH" | grep -q '/original/'; then
  echo "BLOCKED: Cannot modify files in original/ directory — upstream source is read-only. Edit files in ported/ instead." >&2
  exit 2
fi
exit 0
