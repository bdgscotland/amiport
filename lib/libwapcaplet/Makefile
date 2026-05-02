# lib/libwapcaplet/Makefile -- NetSurf string interning library
#
# Upstream: https://github.com/netsurf-browser/libwapcaplet @ c7c128d (v0.4.3)
# Required by: libcss, libdom, libhubbub, libsvgtiny, libnsutils, libnspsl
#              and ultimately ports/netsurf (NetSurf Vampire Phase 1).
#
# Pure C99 single TU. ~290 LOC. No POSIX surface beyond
# malloc/free/memcpy/memset/strncmp/assert (all libnix Tier 1).
# Source-analyzer audit verdict: CLEAN (2026-05-02, no patches required).
#
# CPU TARGET POLICY (deviation from project convention):
#   The NetSurf Vampire dep stack (this lib, libcss, libdom, libhubbub,
#   etc.) is built -m68040 -m68881 to match the ports/netsurf/ consumer
#   ABI exactly. The existing project rule "Libraries MUST Use -m68000"
#   (.claude/rules/known-pitfalls.md) is intentionally overridden here:
#   these libs are NetSurf-Vampire-targeted only and ship into a
#   -m68040/-m68881 binary that requires Apollo / 68040+ with FPU.
#   Reusability for lower-CPU future ports is consciously sacrificed for
#   matching ABI + full project perf gain (FPU inline, MOVE16, wider
#   addressing). Other amiport libs (lib/zlib, lib/oniguruma, etc.) keep
#   their -m68000 baseline; this is a per-NetSurf-stack exception.
#
# vamos test compatibility: vamos can run -m68040 binaries with
#   -C 68040 (per crash-patterns "vamos Needs -C 68020 for 68020+
#   Binaries"). Hardware FPU instructions may or may not run cleanly
#   under vamos -- if not, library tests fall back to FS-UAE A3000+040.
#
# -O1 promoted whole-TU 2026-05-02 after perf-optimizer audit:
#   - Single TU, no structs returned by value (audit-clear vs crash-patterns #16)
#   - No float math, no alignment traps
#   - bebbo-gcc 13.3 (the NetSurf toolchain) has stable -O1 codegen
#   - Hot path is FNV-1a hash compute (called once per intern), -O1 keeps
#     z/str/len in registers for ~40-45% per-intern speedup
#   - Re-tested 35/35 pass on vamos -C 68040 after the promotion
# Stays at -O0 for any future TU added to this lib unless re-audited.
# -std=c99 mandatory: source uses bool, uint32_t, static inline, restrict.
# -noixemul: standard amiport convention (no ixemul, libnix only).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

CFLAGS  = -O1 -noixemul -m68040 -m68881 -std=c99 -Wall
CFLAGS += -Iinclude

TARGET  = libwapcaplet.a
OBJS    = src/libwapcaplet.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

src/libwapcaplet.o: src/libwapcaplet.c include/libwapcaplet/libwapcaplet.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
