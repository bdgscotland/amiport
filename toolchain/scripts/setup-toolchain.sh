#!/bin/bash
#
# setup-toolchain.sh — Set up the Amiga cross-compilation toolchain
#
# Checks for available toolchains (Docker or native) and sets up
# the preferred one.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
DOCKER_DIR="$SCRIPT_DIR/../docker"

echo "=== amiport toolchain setup ==="
echo ""

# Check for Docker
if command -v docker &> /dev/null; then
    echo "[OK] Docker found: $(docker --version)"
    HAS_DOCKER=1
else
    echo "[--] Docker not found"
    HAS_DOCKER=0
fi

# Check for native bebbo-gcc
if command -v m68k-amigaos-gcc &> /dev/null; then
    echo "[OK] Native m68k-amigaos-gcc found: $(m68k-amigaos-gcc --version | head -1)"
    HAS_NATIVE_GCC=1
else
    echo "[--] Native m68k-amigaos-gcc not found"
    HAS_NATIVE_GCC=0
fi

# Check for native VBCC
if command -v vc &> /dev/null; then
    echo "[OK] Native VBCC found"
    HAS_NATIVE_VBCC=1
else
    echo "[--] Native VBCC not found"
    HAS_NATIVE_VBCC=0
fi

echo ""

# Docker setup (preferred)
if [ "$HAS_DOCKER" = "1" ]; then
    echo "Building Docker toolchain (bebbo-gcc)..."
    echo "This will take several minutes on first run."
    echo ""

    docker build -t amiport/bebbo-gcc -f "$DOCKER_DIR/Dockerfile.bebbo-gcc" "$DOCKER_DIR"

    echo ""
    echo "[OK] Docker toolchain ready!"
    echo ""
    echo "Usage:"
    echo "  docker run -v \"\$(pwd)\":/work amiport/bebbo-gcc m68k-amigaos-gcc -o prog prog.c"
    echo ""
    echo "Or use: make build TARGET=examples/wc"
    exit 0
fi

# Native setup fallback
if [ "$HAS_NATIVE_GCC" = "1" ] || [ "$HAS_NATIVE_VBCC" = "1" ]; then
    echo "Using native toolchain."
    exit 0
fi

# No toolchain available
echo "ERROR: No cross-compilation toolchain found."
echo ""
echo "Options:"
echo "  1. Install Docker and run this script again (recommended)"
echo "  2. Install bebbo's amiga-gcc natively:"
echo "     https://github.com/bebbo/amiga-gcc"
echo "  3. Install VBCC natively:"
echo "     http://sun.hasenbraten.de/vbcc/"
echo ""
exit 1
