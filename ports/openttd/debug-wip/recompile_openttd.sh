#!/bin/bash
set -e
BUILD_DIR=/amiport/ports/openttd/original/OpenTTD-13.4/build-dedicated
cd $BUILD_DIR

CXX=/opt/amiga13/bin/m68k-amigaos-g++
CC=/opt/amiga13/bin/m68k-amigaos-gcc

DEFS=$(grep "^CXX_DEFINES" CMakeFiles/openttd.dir/flags.make | sed 's/CXX_DEFINES = //')
INCS=$(grep "^CXX_INCLUDES" CMakeFiles/openttd.dir/flags.make | sed 's/CXX_INCLUDES = //')
FLAGS=$(grep "^CXX_FLAGS" CMakeFiles/openttd.dir/flags.make | sed 's/CXX_FLAGS = //')
# Override -O0 with -O1 (workaround for bebbo-gcc 13.3 std::string operator+ bug at -O0)
FLAGS=$(echo "$FLAGS" | sed 's/-O0/-O1/g')

echo "BUILD_DIR=$BUILD_DIR"
echo "===="

echo "=== Recompiling openttd.cpp ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o CMakeFiles/openttd.dir/src/openttd.cpp.o \
    -c /amiport/ports/openttd/original/OpenTTD-13.4/src/openttd.cpp 2>&1 | tail -3

echo "=== Recompiling fileio.cpp ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o CMakeFiles/openttd.dir/src/fileio.cpp.o \
    -c /amiport/ports/openttd/original/OpenTTD-13.4/src/fileio.cpp 2>&1 | tail -3

echo "=== Recompiling gfxinit.cpp (instantiates BaseGraphics::AddFile) ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o CMakeFiles/openttd.dir/src/gfxinit.cpp.o \
    -c /amiport/ports/openttd/original/OpenTTD-13.4/src/gfxinit.cpp 2>&1 | tail -3

echo "=== Recompiling ini_load.cpp ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o CMakeFiles/openttd.dir/src/ini_load.cpp.o \
    -c /amiport/ports/openttd/original/OpenTTD-13.4/src/ini_load.cpp 2>&1 | tail -3

echo "=== Recompiling music.cpp + sound.cpp ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o CMakeFiles/openttd.dir/src/music.cpp.o \
    -c /amiport/ports/openttd/original/OpenTTD-13.4/src/music.cpp 2>&1 | tail -3
eval $CXX $DEFS $INCS $FLAGS \
    -o CMakeFiles/openttd.dir/src/sound.cpp.o \
    -c /amiport/ports/openttd/original/OpenTTD-13.4/src/sound.cpp 2>&1 | tail -3

echo "=== Recompiling os_amigaos3.cpp ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o /tmp/os_amigaos3.o \
    -c /amiport/ports/openttd/ported/os_amigaos3.cpp 2>&1 | tail -3

echo "=== Recompiling crashlog_amigaos3.cpp ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o /tmp/crashlog_amigaos3.o \
    -c /amiport/ports/openttd/ported/crashlog_amigaos3.cpp 2>&1 | tail -3

echo "=== Recompiling softfloat_stubs.c ==="
$CC -m68020 -O0 -noixemul \
    -o /tmp/softfloat_stubs.o \
    -c /amiport/ports/openttd/ported/softfloat_stubs.c 2>&1 | tail -3

echo "=== Recompiling network_stubs.c ==="
$CC -m68020 -O0 -noixemul \
    -o /tmp/network_stubs.o \
    -c /amiport/ports/openttd/ported/network_stubs.c 2>&1 | tail -3

echo "=== Linking openttd ==="
LINK_CMD=$(cat CMakeFiles/openttd.dir/link.txt | head -1)
LINK_CMD=$(echo "$LINK_CMD" | sed 's| CMakeFiles/openttd.dir/src/os/unix/[^ ]*||g')
LINK_CMD=$(echo "$LINK_CMD" | sed 's| -rdynamic||')
LINK_CMD=$(echo "$LINK_CMD" | sed 's|m68k-amigaos-g++ |m68k-amigaos-g++ -Wl,--allow-multiple-definition -Wl,--no-gc-sections |')
LINK_CMD=$(echo "$LINK_CMD" | sed 's| -o openttd| -o /tmp/openttd /tmp/os_amigaos3.o /tmp/crashlog_amigaos3.o /tmp/softfloat_stubs.o /tmp/network_stubs.o|')
LINK_CMD="$LINK_CMD -lsocket -latomic"

eval $LINK_CMD 2>&1 | tail -10
ls -la /tmp/openttd 2>&1
