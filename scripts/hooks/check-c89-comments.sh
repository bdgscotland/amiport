#!/usr/bin/env bash
# PostToolUse hook for agents that edit C code: catch C++ style comments
# These are the #1 C89 violation agents introduce. bebbo-gcc -ansi rejects them.
#
# Input: TOOL_INPUT contains the tool parameters as JSON
# Only checks .c and .h files in ports/ or lib/

FILE="${TOOL_INPUT_file_path:-}"

# Only check C source files
case "$FILE" in
    *.c|*.h) ;;
    *) exit 0 ;;
esac

# Only check port and library code
case "$FILE" in
    */ports/*|*/lib/*) ;;
    *) exit 0 ;;
esac

# Check for C++ style comments (// at start of line or after whitespace)
# Exclude URLs (http:// https://) and $VER strings
HITS=$(grep -n '^\s*//' "$FILE" 2>/dev/null | grep -v 'http://' | grep -v 'https://' | head -5)

if [ -n "$HITS" ]; then
    echo "C89 WARNING: C++ style comments (//) found in $FILE:"
    echo "$HITS"
    echo ""
    echo "Use /* */ comments instead — bebbo-gcc with -ansi rejects //."
    # Don't block — just warn so the agent can fix
    exit 0
fi

exit 0
