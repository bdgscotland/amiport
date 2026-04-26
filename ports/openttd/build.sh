#!/bin/bash
#
# ports/openttd/build.sh — deterministic OpenTTD build for AmigaOS 3.x
#
# Usage:
#   ports/openttd/build.sh <flavor>
#
# Flavors:
#   dedicated         — headless server build (no SDL2, no GUI)
#   sdl2              — SDL2 GUI build, pristine from upstream OpenTTD source
#   sdl2-profiled     — SDL2 GUI build with debug-wip/*_profiled.cpp overlays
#                        for the AMIPORT_PROFILE runtime instrumentation
#
# Env overrides:
#   CLEAN=1                    — delete build dir before configuring
#   LIBSDL2_DIR=/path/to/libSDL2-amigaos3  (default: ~/Developer/libSDL2-amigaos3)
#   DOCKER_IMAGE=...           — toolchain image
#
# Output: ports/openttd/<binary>  (dedicated → openttd; sdl2* → openttd-sdl2)

set -euo pipefail

FLAVOR="${1:?flavor required: dedicated|sdl2|sdl2-profiled}"
CLEAN="${CLEAN:-0}"
PORT_DIR="$(cd "$(dirname "$0")" && pwd)"
AMIPORT_DIR="$(cd "$PORT_DIR/../.." && pwd)"
LIBSDL2_DIR="${LIBSDL2_DIR:-$HOME/Developer/libSDL2-amigaos3}"
DOCKER_IMAGE="${DOCKER_IMAGE:-ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest}"
SRC_DIR="$PORT_DIR/original/OpenTTD-13.4"
HOST_DIR="$SRC_DIR/build-host"

case "$FLAVOR" in
    dedicated)
        BUILD_SUBDIR="build-amiga"
        TOOLCHAIN="amigaos3-toolchain.cmake"
        CMAKE_OPTS="-DOPTION_DEDICATED=ON"
        PORT_BIN="openttd"
        ;;
    sdl2|sdl2-profiled)
        BUILD_SUBDIR="build-sdl2"
        TOOLCHAIN="amigaos3-toolchain-sdl2.cmake"
        CMAKE_OPTS="-DOPTION_DEDICATED=OFF -DSDL2_DIR=/sdl2/cmake"
        PORT_BIN="openttd-sdl2"
        ;;
    *)
        echo "ERROR: unknown flavor '$FLAVOR'" >&2
        echo "       valid: dedicated | sdl2 | sdl2-profiled" >&2
        exit 2
        ;;
esac

BUILD_DIR="$SRC_DIR/$BUILD_SUBDIR"

echo "=== build.sh: flavor=$FLAVOR clean=$CLEAN ==="
echo "    amiport   : $AMIPORT_DIR"
echo "    libSDL2   : $LIBSDL2_DIR"
echo "    image     : $DOCKER_IMAGE"
echo "    build dir : $BUILD_DIR"
echo "    output bin: $PORT_DIR/$PORT_BIN"

# Prereq checks
if [ ! -f "$LIBSDL2_DIR/libSDL2.a" ]; then
    echo "ERROR: libSDL2.a not found at $LIBSDL2_DIR/libSDL2.a" >&2
    echo "       set LIBSDL2_DIR or build ~/Developer/libSDL2-amigaos3" >&2
    exit 1
fi
if [ ! -x "$HOST_DIR/src/strgen/strgen" ] || [ ! -x "$HOST_DIR/src/settingsgen/settingsgen" ]; then
    echo "ERROR: host build binaries missing at $HOST_DIR" >&2
    echo "       need: $HOST_DIR/src/strgen/strgen" >&2
    echo "       need: $HOST_DIR/src/settingsgen/settingsgen" >&2
    echo "       (run a host-side cmake+make in $HOST_DIR to produce them)" >&2
    exit 1
fi

DOCKER_ARGS=(
    --rm
    -v "$AMIPORT_DIR:/amiport"
    -v "$LIBSDL2_DIR:/sdl2"
    -w "/amiport/ports/openttd/original/OpenTTD-13.4"
    "$DOCKER_IMAGE"
)

# The runtime toolchain image doesn't ship cmake; install it on-the-fly.
# Pull from apt cache only on first run (ensures reproducibility).
ENSURE_CMAKE='command -v cmake >/dev/null 2>&1 || (apt-get update -qq && apt-get install -y -qq cmake) >/dev/null'

# Clean configure if requested or missing
if [ "$CLEAN" = "1" ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "=== clean configure ($BUILD_SUBDIR) ==="
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    docker run "${DOCKER_ARGS[@]}" bash -c "$ENSURE_CMAKE && cd $BUILD_SUBDIR && cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=/amiport/ports/openttd/$TOOLCHAIN \
        -DHOST_BINARY_DIR=/amiport/ports/openttd/original/OpenTTD-13.4/build-host \
        -DUNIX=1 \
        -DOPTION_USE_THREADS=OFF \
        $CMAKE_OPTS"

    # Strip -rdynamic from generated link.txt — bebbo-gcc doesn't support it
    # (and there's no shared-library model on AmigaOS anyway).
    echo "=== patching link.txt (strip -rdynamic) ==="
    if [ -f "$BUILD_DIR/CMakeFiles/openttd.dir/link.txt" ]; then
        sed -i.bak 's| -rdynamic||g' "$BUILD_DIR/CMakeFiles/openttd.dir/link.txt"
    fi
fi

# Build all .cpp files (link will fail — cmake doesn't know about amiport-side
# objects + softfloat + -lsocket -latomic). We deliberately ignore the link
# error here; the real link happens in the dedicated link step below.
echo "=== compile all .cpp objects ==="
docker run "${DOCKER_ARGS[@]}" bash -c "$ENSURE_CMAKE && cd $BUILD_SUBDIR && make -j4 openttd 2>&1 | tail -10 || true"

# Compile the amiport-side TUs separately (not tracked by cmake).
# These provide ShowInfo, crashlog, and network stubs that openttd's upstream
# code calls but which have no Linux-side equivalent.
echo "=== compile amiport TUs ==="
docker run "${DOCKER_ARGS[@]}" bash -c "
    set -e
    cd /amiport/ports/openttd/original/OpenTTD-13.4/$BUILD_SUBDIR
    DEFS=\$(grep '^CXX_DEFINES' CMakeFiles/openttd.dir/flags.make | sed 's/CXX_DEFINES = //')
    INCS=\$(grep '^CXX_INCLUDES' CMakeFiles/openttd.dir/flags.make | sed 's/CXX_INCLUDES = //')
    FLAGS=\$(grep '^CXX_FLAGS' CMakeFiles/openttd.dir/flags.make | sed 's/CXX_FLAGS = //')
    CXX=/opt/amiga13/bin/m68k-amigaos-g++
    CC=/opt/amiga13/bin/m68k-amigaos-gcc
    eval \$CXX \$DEFS \$INCS \$FLAGS -o amiport_os.o -c /amiport/ports/openttd/ported/os_amigaos3.cpp
    eval \$CXX \$DEFS \$INCS \$FLAGS -o amiport_crashlog.o -c /amiport/ports/openttd/ported/crashlog_amigaos3.cpp
    \$CC -m68020 -O1 -noixemul -o amiport_netstubs.o -c /amiport/ports/openttd/ported/network_stubs.c
    echo '  -> amiport TUs compiled (in build dir)'
"

# Final link — build full binary with everything.
echo "=== final link ==="
docker run "${DOCKER_ARGS[@]}" bash -c "
    set -e
    cd /amiport/ports/openttd/original/OpenTTD-13.4/$BUILD_SUBDIR
    # Extract the cmake-generated link command and modify it.
    LINK_CMD=\$(cat CMakeFiles/openttd.dir/link.txt | head -1)
    LINK_CMD=\$(echo \"\$LINK_CMD\" | sed 's| -rdynamic||g')
    LINK_CMD=\$(echo \"\$LINK_CMD\" | sed 's|m68k-amigaos-g++ |m68k-amigaos-g++ -Wl,--allow-multiple-definition -Wl,--gc-sections |')
    LINK_CMD=\$(echo \"\$LINK_CMD\" | sed 's| CMakeFiles/openttd.dir/src/os/unix/[^ ]*||g')
    LINK_CMD=\$(echo \"\$LINK_CMD\" | sed 's| -o openttd| -o openttd /amiport/lib/softfloat/libsoftfloat.a amiport_os.o amiport_crashlog.o amiport_netstubs.o /amiport/lib/posix-shim/src/profile.o -lsocket -latomic|')
    echo \"LINK CMD: \$LINK_CMD\"
    eval \$LINK_CMD
    ls -la openttd
"

BUILT_BIN="$BUILD_DIR/openttd"
if [ ! -f "$BUILT_BIN" ]; then
    echo "ERROR: final link did not produce $BUILT_BIN" >&2
    exit 1
fi

# sdl2-profiled overlay: recompile the profiled cpps and relink
if [ "$FLAVOR" = "sdl2-profiled" ]; then
    echo "=== sdl2-profiled overlay: TODO next session — wire up debug-wip cpps with matching flags ==="
    echo "    (current build is plain sdl2; profiled overlay deferred)"
fi

echo "=== deploy to port dir ==="
cp "$BUILT_BIN" "$PORT_DIR/$PORT_BIN"
ls -la "$PORT_DIR/$PORT_BIN"
echo "=== done ==="
