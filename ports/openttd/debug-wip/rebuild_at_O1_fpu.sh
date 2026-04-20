#!/bin/bash
# Full clean rebuild of build-amiga at -O1 (to keep -m68881 FPU codegen but
# avoid the bebbo-gcc 13.3 std::string operator+ -O0 ABI bug).
# Runs INSIDE the bebbo-gcc 13.3 Docker container.
set -e

BUILD_DIR=/amiport/ports/openttd/original/OpenTTD-13.4/build-amiga
FLAGS_FILE=$BUILD_DIR/CMakeFiles/openttd.dir/flags.make
LOG=/tmp/openttd-O1-build.log

cd $BUILD_DIR

echo "=== [0/4] Install cmake ==="
if ! command -v cmake >/dev/null 2>&1; then
    apt-get update -qq && apt-get install -y -qq cmake >/dev/null 2>&1
fi

echo "=== [1/4] Patch flags.make: -O0 -> -O1 ==="
if grep -q -- '-O0' $FLAGS_FILE; then
    sed -i 's/-O0/-O1/g' $FLAGS_FILE
    echo "  -O0 -> -O1 applied"
else
    echo "  flags.make does NOT have -O0 (already changed?)"
fi
grep '^CXX_FLAGS' $FLAGS_FILE | head -c 300; echo "..."

echo ""
echo "=== [2/4] Wipe .o files ==="
find CMakeFiles/openttd.dir -name '*.o' -delete
echo "  done"

echo ""
echo "=== [3/4] make openttd -j4 (LONG: 30-60 min) ==="
echo "  Started: $(date)"
make openttd -j4 > $LOG 2>&1 || {
    EC=$?
    echo "  make exited rc=$EC"
}

echo ""
echo "=== [4/4] result ==="
ls -la openttd 2>&1 || echo "  (no openttd binary produced)"
COMPILE_ERRORS=$(grep -E '^/amiport.*: (error|fatal error):' $LOG | head -10 || true)
if [ -z "$COMPILE_ERRORS" ]; then
    echo "  no compile errors"
    echo "  object count: $(find CMakeFiles/openttd.dir -name '*.o' | wc -l | tr -d ' ')"
else
    echo "  compile errors:"
    echo "$COMPILE_ERRORS"
fi

echo "  Finished: $(date)"
echo "  Full log: $LOG"
