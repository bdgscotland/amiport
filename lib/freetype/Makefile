# Makefile for FreeType 2.13.3 (AmigaOS 68k, SDL_ttf-only subset)
#
# Minimal build: truetype + sfnt + autofit + smooth + raster + pshinter
# + psaux + psnames + base. No PostScript fonts, no caching, no
# compression, no SVG. Produces libfreetype.a
#
# Consumer: libSDL_ttf (Aminet SDL_ttf-2.0.9-68k) — 12 FT_* symbols.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O0 default for safety on 68k -- bebbo-gcc 6.5.0b has codegen bugs at
# -O1/-O2 for struct-by-value returns > 8 bytes (crash-patterns #16).
# -m68000 because vamos emulates 68000; libraries must not contain
# 68020+ instructions or tests crash with ALERT: code=00068020
# (see known-pitfalls "Libraries MUST Use -m68000").
#
# Per-file -O1 -fno-strict-aliasing below for hot-path files
# (smooth, raster, truetype, ftbitmap, ftbbox) -- perf-optimizer
# verdict (2026-04-16): all functions scalar, no struct-by-value
# returns > 8 bytes, safe for -O1. These are the rasterization and
# TT-bytecode-interpreter inner loops -- downstream SDL_ttf glyph
# rendering hits them per-character per-frame. Test suite must
# re-pass on vamos after any change to these rules.
#
# ftbase and autofit remain at -O0 pending a dedicated struct-return
# audit (they're large TUs that include ftstream.c / ftobjs.c /
# aflatin.c respectively); promotion deferred to a future revision.
CFLAGS = -O0 -noixemul -m68000 -std=gnu99 -Wall -Wno-unused-parameter \
         -DFT2_BUILD_LIBRARY \
         -DFT_USE_TT -DFT_USE_AUTOFIT -DFT_USE_SMOOTH \
         -DFT_USE_RASTER -DFT_USE_PSAUX -DFT_USE_PSHINT \
         -Iamiga-include -Iinclude -Isrc

# Per-file -O1 for hot-path files. `-fno-strict-aliasing` is
# mandatory -- FreeType heavily type-puns FT_Byte*/FT_UShort*/FT_ULong*
# during glyph stream parsing.
HOTPATH_CFLAGS = $(CFLAGS) -O1 -fno-strict-aliasing

TARGET = libfreetype.a

# FreeType uses single-object compilation: each module .c #includes
# all its sub-files. We compile ONE .c per module.
MODULE_SRCS = \
    src/base/ftbase.c \
    src/base/ftinit.c \
    src/base/ftsystem.c \
    src/base/ftdebug.c \
    src/base/ftglyph.c \
    src/base/ftbitmap.c \
    src/base/ftbbox.c \
    src/base/ftmm.c \
    src/truetype/truetype.c \
    src/sfnt/sfnt.c \
    src/autofit/autofit.c \
    src/smooth/smooth.c \
    src/raster/raster.c \
    src/pshinter/pshinter.c \
    src/psaux/psaux.c \
    src/psnames/psnames.c

OBJS = $(MODULE_SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Hot-path files compiled at -O1. Order matters -- these override the
# pattern rule above. Any -O1-related regression should be isolated by
# reverting individual files to the pattern rule (which uses -O0).
src/smooth/smooth.o: src/smooth/smooth.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/raster/raster.o: src/raster/raster.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/truetype/truetype.o: src/truetype/truetype.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/base/ftbitmap.o: src/base/ftbitmap.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/base/ftbbox.o: src/base/ftbbox.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
