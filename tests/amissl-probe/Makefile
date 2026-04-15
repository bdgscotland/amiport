# Makefile for AmiSSL library load diagnostic probe.
#
# PDR-012 Phase 3 session 3. Builds a single-TU standalone Amiga
# binary that does nothing but OpenLibrary() tests. Zero amiport
# dependencies (no libamiport, no libgit2, no zlib) so the
# resulting binary isolates the OpenLibrary question from amigit's
# memory + link-order environment.
#
# Target: -m68020 to match the amigit binary and the AmiSSL 5.27
# OS3 68020-40 backend flavor the FS-UAE harness stages.
#
# Usage:
#   make                  -- build probe
#   make install-emu      -- build + copy to build/amiga/ for FS-UAE
#   make clean

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CFLAGS = -O0 -noixemul -m68020 -Wall

TARGET = probe

.PHONY: all build install-emu clean

all: build

build: $(TARGET)

$(TARGET): probe.c
	$(CC) $(CFLAGS) -o $@ probe.c

install-emu: $(TARGET)
	cp $(TARGET) ../../build/amiga/amissl-probe

clean:
	rm -f $(TARGET)
