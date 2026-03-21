#!/bin/bash
# Stop hook: Remind Claude to verify work before stopping.
# Checks if stop_hook_active to prevent infinite loops.
INPUT=$(cat)
STOP_HOOK_ACTIVE=$(echo "$INPUT" | jq -r '.stop_hook_active // false')

# Prevent infinite loop — if we already ran, let it stop
if [ "$STOP_HOOK_ACTIVE" = "true" ]; then
  exit 0
fi

# Inject a reminder — systemMessage is the valid output field for Stop hooks
echo '{"systemMessage":"Reminder: verify work before stopping — run tests, check for stray root files, and confirm all docs are updated per CLAUDE.md rules."}'
exit 0
