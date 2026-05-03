# lib/libjpeg/Makefile -- IJG libjpeg 9f
#
# Upstream: https://www.ijg.org/files/jpegsrc.v9f.tar.gz (libjpeg 9f),
# IJG license (essentially zlib-style; see LICENSE / UPSTREAM-README).
#
# 44 hand-written .c files vendored from upstream's LIBSOURCES list,
# minus jfdctflt.c and jidctflt.c (the FLOAT DCT variants, excluded
# per the amiport DCT_FLOAT_SUPPORTED-disable patch -- see
# src/jconfig.h and src/jmorecfg.h headers for rationale). Plus
# jmemnobs.c (the no-backing-store memory manager -- simplest of the
# 5 jmem*.c options upstream provides; appropriate for in-memory
# JPEG decode without temp files).
#
# Source-analyzer audit (informal):
# - Pure C89-ish (uses `const` and prototypes; HAVE_PROTOTYPES set in
#   our jconfig.h)
# - Zero non-ASCII bytes (verified)
# - DCT_FLOAT_SUPPORTED disabled at compile time (see src/jmorecfg.h
#   patch); float DCT methods JDCT_FLOAT are completely excluded from
#   the compiled archive. Verify post-build with:
#     m68k-amigaos-nm libjpeg.a | grep -E '__div(sf|df)3|__floatsisf'
# - setjmp-based error model (matches libpng pattern; consumer wraps
#   every entry point in setjmp(jmp_buf))
# - Caller-supplied data source/destination via jpeg_stdio_src or
#   jpeg_mem_src (libjpeg's own helpers using FILE* / memory)
#
# CONSUMER REQUIREMENTS:
# - Link: -ljpeg
# - Consumer manages all jpeg_decompress_struct + jpeg_error_mgr
#   lifecycle; consumer wraps every entry in setjmp for error recovery
# - DO NOT request JDCT_FLOAT method (will fail at runtime since the
#   float DCT functions are not linked); use JDCT_ISLOW (default) or
#   JDCT_IFAST
# - jmemnobs holds the entire decompressed image in heap memory;
#   limit DEFAULT_MAX_MEM in src/jconfig.h is 16 MB (sufficient for
#   typical web JPEGs)
#
# Required by: ports/netsurf (Phase 1 final consumer; <img src="X.jpg">
# decode).
#
# CPU TARGET: -m68040 -m68881 (NetSurf-Vampire dep stack convention).
#
# DEPENDS ON: nothing (no zlib, no external deps; pure C).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# Default -O0 -fno-strict-aliasing per crash-patterns #16. Per-file
# -O1 promotion applied below for 5 hot-path files after Stage 7
# perf-optimizer audit (PERF-REPORT.md): jdhuff.c (Huffman decode),
# jidctint.c (accurate integer IDCT), jidctfst.c (fast integer IDCT),
# jdcolor.c (YCbCr->RGB color conversion), jdsample.c (chroma upsample).
# All audited as -O1 safe (zero struct returns >8 bytes, zero
# soft-float, zero 64-bit math, conservative local sizes).
CFLAGS  = -O0 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
CFLAGS += -DNDEBUG
# jconfig.h + patched jmorecfg.h live in src/, must be found before
# any system version. Public headers in include/ are copied verbatim
# (jpeglib.h + the same patched jmorecfg.h).
CFLAGS += -Isrc -Iinclude

# Hot-path -O1 flags (Stage 7 promoted, see PERF-REPORT.md)
HOTPATH_CFLAGS = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99
HOTPATH_CFLAGS += -DNDEBUG -Isrc -Iinclude

TARGET = libjpeg.a

# 36 LIBSOURCES from upstream Makefile.unix (minus jfdctflt + jidctflt)
# + jmemnobs.c. Total 37 .c files but headers are not source so we
# only list .c files that get compiled.
OBJS  = src/jaricom.o
OBJS += src/jcapimin.o
OBJS += src/jcapistd.o
OBJS += src/jcarith.o
OBJS += src/jccoefct.o
OBJS += src/jccolor.o
OBJS += src/jcdctmgr.o
OBJS += src/jchuff.o
OBJS += src/jcinit.o
OBJS += src/jcmainct.o
OBJS += src/jcmarker.o
OBJS += src/jcmaster.o
OBJS += src/jcomapi.o
OBJS += src/jcparam.o
OBJS += src/jcprepct.o
OBJS += src/jcsample.o
OBJS += src/jctrans.o
OBJS += src/jdapimin.o
OBJS += src/jdapistd.o
OBJS += src/jdarith.o
OBJS += src/jdatadst.o
OBJS += src/jdatasrc.o
OBJS += src/jdcoefct.o
OBJS += src/jdcolor.o
OBJS += src/jddctmgr.o
OBJS += src/jdhuff.o
OBJS += src/jdinput.o
OBJS += src/jdmainct.o
OBJS += src/jdmarker.o
OBJS += src/jdmaster.o
OBJS += src/jdmerge.o
OBJS += src/jdpostct.o
OBJS += src/jdsample.o
OBJS += src/jdtrans.o
OBJS += src/jerror.o
OBJS += src/jfdctfst.o
OBJS += src/jfdctint.o
OBJS += src/jidctfst.o
OBJS += src/jidctint.o
OBJS += src/jquant1.o
OBJS += src/jquant2.o
OBJS += src/jutils.o
OBJS += src/jmemmgr.o
OBJS += src/jmemnobs.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Hot-path overrides: per-file -O1 for Huffman decode + IDCT + color
# convert + chroma upsample. Estimated ~1.6x decode speedup on 68040.
src/jdhuff.o: src/jdhuff.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

src/jidctint.o: src/jidctint.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

src/jidctfst.o: src/jidctfst.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

src/jdcolor.o: src/jdcolor.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

src/jdsample.o: src/jdsample.c
	$(CC) $(HOTPATH_CFLAGS) -c -o $@ $<

# Common header dependencies (every .o depends on the public + internal
# headers; not exhaustive but catches the important ones)
$(OBJS): include/jpeglib.h include/jmorecfg.h \
         src/jconfig.h src/jmorecfg.h src/jpegint.h src/jerror.h \
         src/jinclude.h src/jmemsys.h src/jdct.h src/jversion.h

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
