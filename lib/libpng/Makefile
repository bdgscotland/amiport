# lib/libpng/Makefile -- PNG image decoder library
#
# Upstream: https://github.com/glennrp/libpng @ branch libpng16 commit
# 8c62c3b (libpng 1.6.x), zlib license (see LICENSE).
#
# 15 hand-written .c files (~33K LOC) implementing PNG read + write +
# transformations. Plus 4 internal headers (pngdebug.h, pnginfo.h,
# pngpriv.h, pngstruct.h) and 2 public headers (png.h, pngconf.h)
# placed in include/. The generated pnglibconf.h lives in src/ since
# it's effectively private (consumers don't reference it directly --
# they go through png.h / pngconf.h which conditionally include it).
#
# Source-analyzer audit verdict (2026-05-02): CLEAN with one CRITICAL
# config change applied to src/pnglibconf.h:
#   - PNG_FIXED_POINT_SUPPORTED -- KEPT
#   - PNG_FLOATING_ARITHMETIC_SUPPORTED -- DISABLED (avoid soft-float)
#   - PNG_FLOATING_POINT_SUPPORTED -- DISABLED (avoid soft-float)
#
# Why the float-disable matters: the floating-point gamma path uses
# pow() / floor() / frexp() which would either pull soft-float symbols
# (if compiled without -m68881) OR call FPU transcendentals that
# FS-UAE's 68882 emulation can't handle (per the existing FS-UAE
# 68882 transcendental-gap pitfall family). The fixed-point gamma
# path uses pure integer math via png_fixed_point (= png_int_32) and
# is sufficient for NetSurf's typical PNG decode workload (sRGB
# images without explicit gAMA chunks, or images where the consumer
# accepts integer-precision gamma).
#
# After the patch:
# - Zero `pow` / `floor` / `frexp` calls survive in the compiled archive
# - Zero soft-float symbol pulls (verify with m68k-amigaos-nm post-build)
# - All API functions that need a gamma value have *_fixed variants
#   (png_set_gAMA_fixed, png_get_gAMA_fixed, etc.) which consumers
#   call with 1/100000 fixed-point units
#
# CONSUMER REQUIREMENTS:
# - Link order: -lpng -lz (libpng calls into zlib for IDAT inflate/deflate)
# - Consumer provides custom png_malloc / png_free + I/O callbacks via
#   png_create_read_struct() and png_set_read_fn() at decode time
# - Consumer must call png_destroy_read_struct() / png_destroy_write_struct()
#   for every successfully created PNG to avoid leaks on AmigaOS -noixemul
# - Consumer that needs gamma correction must use *_fixed variants
#   (no float APIs available)
#
# Required by: ports/netsurf (Phase 1 final consumer; <img src="X.png"> decode).
#
# CPU TARGET: -m68040 -m68881 (NetSurf-Vampire dep stack convention; the
# -m68881 ensures GCC inlines FPU instructions for any future float math
# rather than emitting soft-float symbol calls).
#
# DEPENDS ON: lib/zlib/ (for IDAT inflate/deflate). Link with -lz AFTER
# -lpng (libpng calls into zlib).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# Default -O0 -fno-strict-aliasing per crash-patterns #16. Per-file -O1
# promoted on the read-side hot path after Stage 7 perf-optimizer audit
# (see PERF-REPORT.md): pngread.c + pngrtran.c + pngrutil.c -- the three
# scanline-loop dominant files. All three audited as safe (zero struct
# returns >8 bytes, zero soft-float, zero 64-bit math).
CFLAGS  = -O0 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -DPNG_NO_CONSOLE_IO
# pnglibconf.h is in src/, so libpng's own files find it via -Isrc.
# pngconf.h does the conditional #include for both internal and external
# consumers via <pnglibconf.h>.
CFLAGS += -Iinclude -Isrc
CFLAGS += -I../zlib/include

# Hot-path -O1 flags (Stage 7 promoted, see PERF-REPORT.md)
HOTPATH_CFLAGS = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
HOTPATH_CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE -DPNG_NO_CONSOLE_IO
HOTPATH_CFLAGS += -Iinclude -Isrc -I../zlib/include

TARGET = libpng.a

OBJS  = src/png.o
OBJS += src/pngerror.o
OBJS += src/pngget.o
OBJS += src/pngmem.o
OBJS += src/pngpread.o
OBJS += src/pngread.o
OBJS += src/pngrio.o
OBJS += src/pngrtran.o
OBJS += src/pngrutil.o
OBJS += src/pngset.o
OBJS += src/pngtrans.o
OBJS += src/pngwio.o
OBJS += src/pngwrite.o
OBJS += src/pngwtran.o
OBJS += src/pngwutil.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Hot-path overrides: per-file -O1 for the scanline-loop dominant files.
src/pngread.o: src/pngread.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

src/pngrtran.o: src/pngrtran.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

src/pngrutil.o: src/pngrutil.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

# Common header dependencies
$(OBJS): include/png.h include/pngconf.h \
         src/pnglibconf.h src/pngpriv.h src/pngstruct.h src/pnginfo.h \
         src/pngdebug.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
