#!/bin/bash
set -e
cd /tmp
EXTRA="${1:-}"
echo "=== Compiling repro.cpp with EXTRA=$EXTRA ==="
/opt/amiga13/bin/m68k-amigaos-g++ \
    -std=c++17 -m68040 -m68881 -O0 -noixemul \
    -D__libnix__ \
    $EXTRA \
    -o repro repro.cpp 2>&1 | tail -5
/opt/amiga13/bin/m68k-amigaos-strip --strip-debug /tmp/repro 2>&1
ls -la /tmp/repro
