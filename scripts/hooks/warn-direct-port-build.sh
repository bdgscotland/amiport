#!/bin/bash
# warn-direct-port-build.sh — PreToolUse hook for Bash
#
# Warns when running make/build commands targeting ports/ directly
# instead of using the build-manager agent. Library builds (lib/) are OK.

INPUT=$(cat)
COMMAND=$(echo "$INPUT" | jq -r '.tool_input.command // empty')

# Skip if no command
[ -z "$COMMAND" ] && exit 0

# Check for make commands targeting ports/
if echo "$COMMAND" | grep -qE '(make\s+.*-C\s+.*ports/|make\s+.*TARGET=ports/|cd\s+ports/.*/.*make)'; then
    # Allow 'make test' smoke tests (just running the binary)
    if echo "$COMMAND" | grep -qE 'make\s+(-C\s+\S+\s+)?test\b'; then
        exit 0
    fi
    # Allow 'make clean'
    if echo "$COMMAND" | grep -qE 'make\s+(-C\s+\S+\s+)?clean\b'; then
        exit 0
    fi
    echo '{"hookSpecificOutput":{"hookEventName":"PreToolUse","additionalContext":"WARNING: Direct make on a port directory detected. Use the build-manager agent for port builds: Agent(subagent_type=\"build-manager\", prompt=\"Rebuild ports/<name> ...\"). Direct make is acceptable for library builds (lib/) and smoke tests (make test), but full port builds should go through the agent pipeline."}}'
fi
