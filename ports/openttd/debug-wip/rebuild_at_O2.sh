#!/bin/bash
# Full clean rebuild of OpenTTD-SDL2 at -O2 (Session 4 of PDR-015).
# Runs INSIDE the bebbo-gcc 13.3 docker container.
# Assumes flags.make currently has -O1; sed-replaces in place.
# After full rebuild succeeds, the caller must run recompile_openttd_sdl2.sh
# for profiled overrides + final link.
set -e

BUILD_DIR=/amiport/ports/openttd/original/OpenTTD-13.4/build-sdl2
FLAGS_FILE=$BUILD_DIR/CMakeFiles/openttd.dir/flags.make
LOG=/tmp/openttd-O2-build.log

cd $BUILD_DIR

echo "=== [0/4] Install cmake (not in toolchain image) ==="
if ! command -v cmake >/dev/null 2>&1; then
    apt-get update -qq && apt-get install -y -qq cmake >/dev/null 2>&1
    echo "  cmake installed: $(cmake --version | head -1)"
else
    echo "  cmake already present"
fi

echo "=== [1/4] Patch flags.make: -O1 -> -O2 + align with PROF_BASE ==="
if grep -q -- '-O2' $FLAGS_FILE; then
    echo "  flags.make already has -O2"
else
    sed -i 's/-O1/-O2/g' $FLAGS_FILE
    echo "  flags.make: -O1 -> -O2"
fi
# At -O2, fmt template instantiations diverge between TUs that differ in -D and -I.
# The profiled .cpp files use PROF_BASE = $FLAGS -DAMIPORT_PROFILE -I/amiport/lib/posix-shim/include -I/sdl2/include
# To match, add the same to global CXX_FLAGS so all 417 TUs see the same fmt headers + macros.
if ! grep -q -- '-DAMIPORT_PROFILE' $FLAGS_FILE; then
    sed -i 's|^CXX_FLAGS = |CXX_FLAGS = -DAMIPORT_PROFILE -I/amiport/lib/posix-shim/include |' $FLAGS_FILE
    echo "  flags.make: added -DAMIPORT_PROFILE -I/amiport/lib/posix-shim/include to CXX_FLAGS"
fi
grep '^CXX_FLAGS' $FLAGS_FILE | head -c 250; echo "..."

echo ""
echo "=== [2/4] Wipe all .o files (force full rebuild) ==="
find CMakeFiles/openttd.dir -name '*.o' -delete
echo "  done"

echo ""
echo "=== [3/4] make openttd -j4 (LONG: 30-60 min, log to $LOG) ==="
echo "  Started: $(date)"
# Capture full log; tail to caller for visibility. Don't fail-stop on link
# (link will fail because we don't have profiled overrides + AmigaOS support
# .o files yet -- the recompile script handles that). We only care about
# whether all .cpp files compile cleanly at -O2.
make openttd -j4 > $LOG 2>&1 || {
    EC=$?
    echo "  make exited rc=$EC (link failures expected; check compile errors below)"
}

echo ""
echo "=== [4/4] Compile error summary ==="
COMPILE_ERRORS=$(grep -E '^/amiport.*: (error|fatal error):' $LOG | head -30 || true)
if [ -z "$COMPILE_ERRORS" ]; then
    echo "  NO COMPILE ERRORS -- all .cpp built cleanly at -O2"
    echo "  Object count: $(find CMakeFiles/openttd.dir -name '*.o' | wc -l | tr -d ' ')"
else
    echo "  COMPILE ERRORS FOUND:"
    echo "$COMPILE_ERRORS"
    echo ""
    echo "  Files that failed:"
    grep -E '^/amiport.*: (error|fatal error):' $LOG | sed 's|:.*||' | sort -u | head -20
fi

echo ""
echo "  Finished: $(date)"
echo "  Full log: $LOG"
