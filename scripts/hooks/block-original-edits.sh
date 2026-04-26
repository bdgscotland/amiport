#!/bin/bash
# PreToolUse hook: Block edits to ports/*/original/ directories
# These contain upstream source and should never be modified by agents.
#
# Exception: Games (SDL2 ports) often need deep upstream patches for AmigaOS
# integration (platform entry points, event loop tweaks, saveload format, etc.)
# A port can opt out of this rule by creating a file named `.allow-original-edits`
# in the port root (e.g. ports/openttd/.allow-original-edits). When the marker
# exists, edits to that port's original/ tree are allowed but logged to stderr
# so they're visible in the session transcript.

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // .tool_input.path // empty')

if echo "$FILE_PATH" | grep -q '/original/'; then
  # Extract the port root (ports/<name>/) from the file path
  PORT_ROOT=$(echo "$FILE_PATH" | sed -nE 's|^(.*/ports/[^/]+)/.*$|\1|p')
  if [ -n "$PORT_ROOT" ] && [ -f "$PORT_ROOT/.allow-original-edits" ]; then
    echo "ALLOWED (marker present): $FILE_PATH under $PORT_ROOT/.allow-original-edits" >&2
    exit 0
  fi
  echo "BLOCKED: Cannot modify files in original/ directory — upstream source is read-only. Edit files in ported/ instead." >&2
  if [ -n "$PORT_ROOT" ]; then
    echo "         (To allow: create '$PORT_ROOT/.allow-original-edits' — use for game ports that need deep upstream patches.)" >&2
  fi
  exit 2
fi
exit 0
