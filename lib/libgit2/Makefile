# Makefile for libgit2 1.8.5 (AmigaOS 68k via bebbo-gcc)
#
# Produces libgit2.a -- the embedded git library. Ported for amiport
# PDR-010 to support the future `ports/amigit/` CLI. Network transports,
# threading, iconv, mmap, and platform crypto backends are all disabled.
#
# Upstream: https://github.com/libgit2/libgit2 (v1.8.5)
# License:  GNU GPL v2 with Linking Exception
#
# Dependencies:
#   - lib/zlib/libz.a  (PDR-010 Phase 1, already shipping)
#   - amiport posix-shim (for pread, pwrite, realpath, lstat, etc.)
#
# Pipeline: see .claude/rules/library-pipeline.md for the mandatory
# stages. This Makefile is Stage 2 output; do NOT edit without source-
# analyzer, memory-checker, and perf-optimizer revalidation.
#
# Source-analyzer verdict: CAVEATS (5 targeted upstream patches applied,
# documented in PATCHES.md). Build flags below reflect PDR-010 disable
# matrix + source-analyzer recommendations.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC     = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR     = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O0 default per library-pipeline rule and crash-patterns #16 (bebbo-gcc
# 6.5.0b struct-by-value return corruption at -O1/-O2). Per-file -O1
# promotion is deferred to Stage 7 (perf-optimizer) after the full test
# suite passes on FS-UAE. Do NOT promote individual files until that
# rebuild+retest cycle has verified correctness.
#
# -m68000 is mandatory -- vamos emulates 68000; libraries with 68020+
# instructions crash tests with "ALERT: code=00068020" even if the test
# binary itself targets 68020.
#
# -std=gnu99 because libgit2 is heavy C99 (designated initializers,
# `bool`, for-init declarations). libnix is a C89 runtime but gnu99
# language extensions are supported (ADR-022).
#
# -include amigaos_compat.h force-injects the amiport shim macros
# (pread -> amiport_pread, realpath -> amiport_realpath, etc.) into
# every translation unit so libgit2 source stays unchanged.

CFLAGS = -std=gnu99 -noixemul -m68000 -O0 \
         -Wall -Wno-unused-parameter -Wno-unused-function \
         -Wno-pointer-sign -Wno-sign-compare \
         -include src/util/amigaos_compat.h \
         -Isrc/util -Isrc/libgit2 -Iinclude \
         -Isrc/xdiff -Isrc/pcre \
         -I../zlib/include \
         -DNO_MMAP \
         -DGIT_IO_SELECT \
         -DSHA1DC_FORCE_BIGENDIAN=1 \
         -DHAVE_CONFIG_H \
         -DLINK_SIZE=2 \
         -DGIT_LEGACY_HASH

# HOTPATH_CFLAGS: -O1 with -fno-strict-aliasing for audited scalar-only files.
# -fno-strict-aliasing is mandatory -- xdiff and SHA1DC cast pointers freely
# (char* <-> long*, uint8_t* <-> uint32_t*, chastore pointers). Without it,
# the compiler may reorder stores through unrelated types, producing silent
# data corruption. Promoted files verified: no struct-by-value returns > 8
# bytes, no large stack locals above the 512-byte threshold. See PERF-REPORT.md.
HOTPATH_CFLAGS = $(subst -O0,-O1,$(CFLAGS)) -fno-strict-aliasing

# PCRE files need an extra define to signal the bundled-minimal build
PCRE_CFLAGS = $(CFLAGS) -DPCRE_STATIC -DSUPPORT_PCRE8 -Isrc/pcre

# xdiff header is inside src/xdiff/
XDIFF_CFLAGS = $(CFLAGS) -Isrc/xdiff

TARGET = libgit2.a

# ========================================================================
# Source file lists -- directories are pre-pruned by the Stage 2 copy
# (streams/, transports/, win32/, unix/process.c, clone.c/fetch.c/remote.c
# etc. intentionally excluded). $(wildcard) here reflects that pruning.
# ========================================================================

LIBGIT2_SRCS  = $(wildcard src/libgit2/*.c)
UTIL_SRCS     = $(wildcard src/util/*.c) \
                $(wildcard src/util/allocators/*.c) \
                $(wildcard src/util/hash/*.c) \
                $(wildcard src/util/hash/sha1dc/*.c) \
                $(wildcard src/util/hash/rfc6234/*.c) \
                $(wildcard src/util/unix/*.c)
XDIFF_SRCS    = $(wildcard src/xdiff/*.c)
PCRE_SRCS     = $(wildcard src/pcre/*.c)

LIBGIT2_OBJS  = $(LIBGIT2_SRCS:.c=.o)
UTIL_OBJS     = $(UTIL_SRCS:.c=.o)
XDIFF_OBJS    = $(XDIFF_SRCS:.c=.o)
PCRE_OBJS     = $(PCRE_SRCS:.c=.o)

ALL_OBJS = $(LIBGIT2_OBJS) $(UTIL_OBJS) $(XDIFF_OBJS) $(PCRE_OBJS)

# ========================================================================
# Build rules
# ========================================================================

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(AR) rcs $@ $(ALL_OBJS)
	$(RANLIB) $@

# Standard rules (libgit2 core + util)
src/libgit2/%.o: src/libgit2/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/util/%.o: src/util/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/util/allocators/%.o: src/util/allocators/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/util/hash/%.o: src/util/hash/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/util/hash/sha1dc/%.o: src/util/hash/sha1dc/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/util/hash/rfc6234/%.o: src/util/hash/rfc6234/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/util/unix/%.o: src/util/unix/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# xdiff uses its own include layout
src/xdiff/%.o: src/xdiff/%.c
	$(CC) $(XDIFF_CFLAGS) -c $< -o $@

# PCRE uses its own config.h and needs extra defines
src/pcre/%.o: src/pcre/%.c
	$(CC) $(PCRE_CFLAGS) -c $< -o $@

# ========================================================================
# Per-file -O1 promotions (HOTPATH_CFLAGS) -- audited by perf-optimizer
# Stage 7, 2026-04-13. See PERF-REPORT.md for full audit rationale.
# Each file verified: no struct-by-value return > 8 bytes, no large stack
# locals > 512 bytes, no inline-asm, -fno-strict-aliasing covers pointer
# casting. Do NOT add files here without a full per-TU audit.
# ========================================================================

# SHA1DC: 80-step unrolled compression; register allocation at -O1 is the
# dominant win (a,b,c,d,e state variables stay in regs across all 80 steps).
src/util/hash/sha1dc/sha1.o: src/util/hash/sha1dc/sha1.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/util/hash/sha1dc/ubc_check.o: src/util/hash/sha1dc/ubc_check.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

# xdiff core: Myers O(ND) algorithm, line hashing, context preparation.
# -fno-strict-aliasing covers chastore's (char*) <-> (long*) casts.
XDIFF_HOTPATH_CFLAGS = $(subst -O0,-O1,$(XDIFF_CFLAGS)) -fno-strict-aliasing

src/xdiff/xdiffi.o: src/xdiff/xdiffi.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS) -c $< -o $@

src/xdiff/xprepare.o: src/xdiff/xprepare.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS) -c $< -o $@

src/xdiff/xutils.o: src/xdiff/xutils.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS) -c $< -o $@

src/xdiff/xhistogram.o: src/xdiff/xhistogram.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS) -c $< -o $@

src/xdiff/xmerge.o: src/xdiff/xmerge.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS) -c $< -o $@

src/xdiff/xpatience.o: src/xdiff/xpatience.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS) -c $< -o $@

# wildmatch: .gitignore / pathspec matching; pure table-driven char lookup.
src/util/wildmatch.o: src/util/wildmatch.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_OBJS) $(TARGET)

.PHONY: all clean
