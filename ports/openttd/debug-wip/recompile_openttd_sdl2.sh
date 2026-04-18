#!/bin/bash
# Recompile + relink the GUI / SDL2 OpenTTD build.
# Mirrors recompile_openttd.sh but for build-sdl2/.
set -e
BUILD_DIR=/amiport/ports/openttd/original/OpenTTD-13.4/build-sdl2
cd $BUILD_DIR

CXX=/opt/amiga13/bin/m68k-amigaos-g++
CC=/opt/amiga13/bin/m68k-amigaos-gcc

DEFS=$(grep "^CXX_DEFINES" CMakeFiles/openttd.dir/flags.make | sed 's/CXX_DEFINES = //')
INCS=$(grep "^CXX_INCLUDES" CMakeFiles/openttd.dir/flags.make | sed 's/CXX_INCLUDES = //')
FLAGS=$(grep "^CXX_FLAGS" CMakeFiles/openttd.dir/flags.make | sed 's/CXX_FLAGS = //')
# Override -O0 with -O1 (workaround for bebbo-gcc 13.3 std::string operator+ bug)
FLAGS=$(echo "$FLAGS" | sed 's/-O0/-O1/g')

echo "BUILD_DIR=$BUILD_DIR"
echo "===="

echo "=== Rebuilding files at -O1 to dodge bebbo-gcc 13.3 -O0 std::string op+ bug ==="
# These files use std::string operator+ in load-bearing init paths.
# At -O0 they corrupt the returned string (Guru #80000004 / #80000006).
# CMake's default for our toolchain is -O0; override with the FLAGS sed.
for src in openttd.cpp fileio.cpp gfxinit.cpp ini_load.cpp music.cpp sound.cpp \
           video/dedicated_v.cpp video/sdl2_v.cpp video/sdl2_default_v.cpp \
           genworld.cpp; do
    obj="CMakeFiles/openttd.dir/src/${src}.o"
    src_full="/amiport/ports/openttd/original/OpenTTD-13.4/src/${src}"
    if [ -f "$src_full" ]; then
        echo "  -> $src"
        eval $CXX $DEFS $INCS $FLAGS -o "$obj" -c "$src_full" 2>&1 | tail -2 || true
    fi
done

echo "=== Recompiling AmigaOS support files (os_amigaos3, crashlog, network_stubs) ==="
eval $CXX $DEFS $INCS $FLAGS \
    -o /tmp/os_amigaos3_sdl2.o \
    -c /amiport/ports/openttd/ported/os_amigaos3.cpp 2>&1 | tail -3
eval $CXX $DEFS $INCS $FLAGS \
    -o /tmp/crashlog_amigaos3_sdl2.o \
    -c /amiport/ports/openttd/ported/crashlog_amigaos3.cpp 2>&1 | tail -3
$CC -m68020 -O1 -noixemul \
    -o /tmp/network_stubs_sdl2.o \
    -c /amiport/ports/openttd/ported/network_stubs.c 2>&1 | tail -3

echo "=== Recompiling md5_aligned (alignment fix from dedicated build) ==="
eval $CXX $DEFS $INCS $FLAGS \
    -I/amiport/ports/openttd/original/OpenTTD-13.4/src/3rdparty/md5 \
    -o CMakeFiles/openttd.dir/src/3rdparty/md5/md5.cpp.o \
    -c /amiport/ports/openttd/ported/md5_aligned.cpp 2>&1 | tail -3

echo "=== Linking openttd-sdl2 ==="
LINK_CMD=$(cat CMakeFiles/openttd.dir/link.txt | head -1)
LINK_CMD=$(echo "$LINK_CMD" | sed 's| -rdynamic||')
LINK_CMD=$(echo "$LINK_CMD" | sed 's|m68k-amigaos-g++ |m68k-amigaos-g++ -Wl,--allow-multiple-definition -Wl,--no-gc-sections |')
LINK_CMD=$(echo "$LINK_CMD" | sed 's| CMakeFiles/openttd.dir/src/os/unix/[^ ]*||g')
LINK_CMD=$(echo "$LINK_CMD" | sed 's| -o openttd| -o /tmp/openttd-sdl2 /amiport/lib/softfloat/libsoftfloat.a /tmp/os_amigaos3_sdl2.o /tmp/crashlog_amigaos3_sdl2.o /tmp/network_stubs_sdl2.o|')
LINK_CMD="$LINK_CMD -lsocket -latomic"

eval $LINK_CMD 2>&1 | tail -8
ls -la /tmp/openttd-sdl2 2>&1
