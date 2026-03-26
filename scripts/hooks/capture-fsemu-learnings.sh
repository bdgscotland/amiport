#!/bin/bash
# PostToolUse hook: When FS-UAE tests have failures, remind to capture learnings.
# Fires on Bash commands — checks if the output contains FS-UAE test failure markers.

# Only check Bash commands that look like test-fsemu runs
COMMAND="${TOOL_INPUT_command:-}"
case "$COMMAND" in
    *test-fsemu*) ;;
    *) exit 0 ;;
esac

# Check if the output contains failure markers
OUTPUT="${TOOL_OUTPUT:-}"
if echo "$OUTPUT" | grep -q "FAILED\|failed: [1-9]"; then
    echo "FS-UAE test failure detected. After fixing, run /capture-learning to document what went wrong and prevent it in future ports."
fi

exit 0
