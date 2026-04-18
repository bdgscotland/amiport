#!/bin/bash
# Build the softfloat-test isolation binary.
# Uses the GCC 13.3 toolchain Docker image (same as openttd) so the C++17
# + libstdc++ surface and the lib/softfloat link are exactly the same as
# what OpenTTD uses.
#
# Output: build/amiga/softfloat-test/softfloat-test
set -euo pipefail

cd "$(dirname "$0")/../.."  # → project root

mkdir -p build/amiga/softfloat-test

# Build lib/softfloat first if not present
if [ ! -f lib/softfloat/libsoftfloat.a ]; then
    echo "=== Building lib/softfloat (one-time) ==="
    make -C lib/softfloat
fi

echo "=== Compiling softfloat-test.cpp ==="
docker run --rm \
    -v "$(pwd):/amiport" \
    -w /amiport \
    ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    /opt/amiga13/bin/m68k-amigaos-g++ \
        -std=c++17 -m68020 -O1 -noixemul \
        -fno-strict-aliasing \
        -D__libnix__ \
        -DFMT_HEADER_ONLY -DFMT_EXCEPTIONS=0 \
        -I/amiport/lib/softfloat/include \
        -I/amiport/ports/openttd/original/OpenTTD-13.4/src/3rdparty \
        -c ports/softfloat-test/softfloat-test.cpp \
        -o build/amiga/softfloat-test/softfloat-test.o

echo "=== Compiling stubs.c ==="
docker run --rm \
    -v "$(pwd):/amiport" \
    -w /amiport \
    ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    /opt/amiga13/bin/m68k-amigaos-gcc \
        -m68020 -O0 -noixemul \
        -c ports/softfloat-test/stubs.c \
        -o build/amiga/softfloat-test/stubs.o

echo "=== Linking softfloat-test ==="
# Critical: -lsoftfloat MUST appear before -lm so __divsf3 etc. resolve
# from libsoftfloat.a (and not from libnix's mathieee*-routing versions).
docker run --rm \
    -v "$(pwd):/amiport" \
    -w /amiport \
    ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    /opt/amiga13/bin/m68k-amigaos-g++ \
        -m68020 -noixemul \
        -Wl,--allow-multiple-definition \
        build/amiga/softfloat-test/softfloat-test.o \
        build/amiga/softfloat-test/stubs.o \
        -L/amiport/lib/softfloat -lsoftfloat \
        -lstdc++ -lm \
        -o build/amiga/softfloat-test/softfloat-test

echo "=== Stripping debug info ==="
docker run --rm \
    -v "$(pwd):/amiport" \
    -w /amiport \
    ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    /opt/amiga13/bin/m68k-amigaos-strip --strip-debug \
        build/amiga/softfloat-test/softfloat-test

ls -la build/amiga/softfloat-test/softfloat-test
echo "=== Done ==="
