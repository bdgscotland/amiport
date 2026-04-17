#!/bin/bash
# Automated OpenTTD test runner for FS-UAE
# Launches FS-UAE, runs OpenTTD, captures output, reports result.
#
# Usage: bash ports/openttd/test-openttd.sh [timeout_seconds]

set -e

AMIPORT="$(cd "$(dirname "$0")/../.." && pwd)"
TIMEOUT="${1:-60}"
BUILDDIR="$AMIPORT/build/amiga"
FSCONFIG="$AMIPORT/ports/openttd/openttd-test.fs-uae"

echo "=== OpenTTD FS-UAE Test Runner ==="
echo "Timeout: ${TIMEOUT}s"

# Create AmigaOS startup-sequence that runs OpenTTD and captures output
STARTUP="$BUILDDIR/OpenTTD/test-run"
cat > "$STARTUP" << 'AMIGA'
; Auto-run OpenTTD for testing
Stack 1048576
CD WORK:OpenTTD
WORK:OpenTTD/openttd -b WORK:OpenTTD/baseset -d 1 >T:openttd.log 2>&1
Echo "EXIT_CODE: " NOLINE
Echo $RC
Echo "=== TEST COMPLETE ===" >>T:openttd.log
AMIGA

# Create a User-Startup that auto-runs our test
USERSTARTUP="$AMIPORT/build/system/S/User-Startup"
cp "$USERSTARTUP" "$USERSTARTUP.bak" 2>/dev/null || true
cat > "$USERSTARTUP" << 'AMIGA'
; Auto-test OpenTTD
Wait 3
Execute WORK:OpenTTD/test-run
AMIGA

# Launch FS-UAE in background
echo "Launching FS-UAE..."
fs-uae "$FSCONFIG" &
FSPID=$!

# Wait for timeout or completion
echo "Waiting up to ${TIMEOUT}s for test..."
ELAPSED=0
while [ $ELAPSED -lt $TIMEOUT ]; do
    if ! kill -0 $FSPID 2>/dev/null; then
        echo "FS-UAE exited after ${ELAPSED}s"
        break
    fi
    sleep 5
    ELAPSED=$((ELAPSED + 5))
    # Check if test output appeared
    if [ -f "$BUILDDIR/OpenTTD/T/openttd.log" ] 2>/dev/null; then
        if grep -q "TEST COMPLETE" "$BUILDDIR/OpenTTD/T/openttd.log" 2>/dev/null; then
            echo "Test completed after ${ELAPSED}s"
            break
        fi
    fi
done

# Kill FS-UAE if still running
if kill -0 $FSPID 2>/dev/null; then
    echo "Timeout -- killing FS-UAE"
    kill $FSPID 2>/dev/null
    wait $FSPID 2>/dev/null
fi

# Restore User-Startup
if [ -f "$USERSTARTUP.bak" ]; then
    mv "$USERSTARTUP.bak" "$USERSTARTUP"
else
    echo "; User-Startup" > "$USERSTARTUP"
fi

# Report results
echo ""
echo "=== RESULTS ==="
if [ -f "$BUILDDIR/OpenTTD/T/openttd.log" ] 2>/dev/null; then
    echo "--- OpenTTD output ---"
    cat "$BUILDDIR/OpenTTD/T/openttd.log"
else
    echo "No output captured (crashed before writing log, or T: not accessible)"
    echo "Check FS-UAE console output for Guru Meditation codes"
fi
