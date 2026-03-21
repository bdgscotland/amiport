#!/bin/bash
# PreToolUse hook: Block file creation in the project root during porting.
# Only flags files that look like build/test artifacts — not legitimate root files.
INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty')

# No file path — skip
[ -z "$FILE_PATH" ] && exit 0

# Resolve the project root from git
PROJECT_ROOT=$(git -C "$(dirname "$FILE_PATH")" rev-parse --show-toplevel 2>/dev/null)
[ -z "$PROJECT_ROOT" ] && exit 0

# Get the directory of the target file
FILE_DIR=$(dirname "$FILE_PATH")

# Only check files written directly to the project root
[ "$FILE_DIR" != "$PROJECT_ROOT" ] && exit 0

# Allow known legitimate root files
BASENAME=$(basename "$FILE_PATH")
case "$BASENAME" in
  CLAUDE.md|README.md|PORTS.md|Makefile|.gitignore|.gitattributes|LICENSE|CHANGELOG.md|VERSION)
    exit 0
    ;;
  *.json|*.yml|*.yaml|*.toml)
    # Config files are fine
    exit 0
    ;;
esac

# Block — exit 2 tells PreToolUse to reject the tool call
echo "BLOCKED: Cannot create '$BASENAME' in project root. Build/test artifacts must go inside ports/<name>/ or the appropriate subdirectory. See test-hygiene rules." >&2
exit 2
