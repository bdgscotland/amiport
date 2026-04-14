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

# ----------------------------------------------------------------------
# Dual-flavor 68020 build (Track A, 2026-04-14)
#
# Mirror the pattern from lib/zlib/Makefile (commit 4a8a362). All four
# CFLAGS variants get a -020 twin derived via $(subst -m68000,-m68020),
# so future changes to the base flags automatically propagate. Same
# -O levels in both flavors -- the win is in the CPU instruction set,
# not the optimizer (see lib/zlib/Makefile commit message for the
# full rationale).
#
# Per .claude/rules/library-pipeline.md + the "Libraries MUST Use
# -m68000" pitfall: libgit2.a (000 flavor) remains the default, is
# the only one linked by the Stage 5 test suite, and is what every
# current amiport port sees. libgit2-020.a is opt-in for amigit-020
# and any future 68020+ consumer.
# ----------------------------------------------------------------------
CFLAGS_020              = $(subst -m68000,-m68020,$(CFLAGS))
HOTPATH_CFLAGS_020      = $(subst -m68000,-m68020,$(HOTPATH_CFLAGS))
PCRE_CFLAGS_020         = $(subst -m68000,-m68020,$(PCRE_CFLAGS))
XDIFF_CFLAGS_020        = $(subst -m68000,-m68020,$(XDIFF_CFLAGS))

TARGET     = libgit2.a
TARGET_020 = libgit2-020.a

# ========================================================================
# Source file lists -- directories are pre-pruned by the Stage 2 copy
# (streams/, transports/, win32/, unix/process.c, clone.c/fetch.c/remote.c
# etc. intentionally excluded). $(wildcard) here reflects that pruning.
# ========================================================================

LIBGIT2_SRCS  = $(wildcard src/libgit2/*.c)
# Transports subtree: PDR-012 Phase 1 un-pruned the smart transport
# trio (smart.c / smart_pkt.c / smart_protocol.c). The rest of the
# transports/ dir stays pruned (http.c, ssh_*.c, local.c, etc.).
# Listed explicitly rather than via wildcard so accidental upstream
# file additions do not silently enter the build.
TRANSPORT_SRCS = src/libgit2/transports/smart.c \
                 src/libgit2/transports/smart_pkt.c \
                 src/libgit2/transports/smart_protocol.c
UTIL_SRCS     = $(wildcard src/util/*.c) \
                $(wildcard src/util/allocators/*.c) \
                $(wildcard src/util/hash/*.c) \
                $(wildcard src/util/hash/sha1dc/*.c) \
                $(wildcard src/util/hash/rfc6234/*.c) \
                $(wildcard src/util/unix/*.c)
XDIFF_SRCS    = $(wildcard src/xdiff/*.c)
PCRE_SRCS     = $(wildcard src/pcre/*.c)

LIBGIT2_OBJS  = $(LIBGIT2_SRCS:.c=.o)
TRANSPORT_OBJS = $(TRANSPORT_SRCS:.c=.o)
UTIL_OBJS     = $(UTIL_SRCS:.c=.o)
XDIFF_OBJS    = $(XDIFF_SRCS:.c=.o)
PCRE_OBJS     = $(PCRE_SRCS:.c=.o)

ALL_OBJS = $(LIBGIT2_OBJS) $(TRANSPORT_OBJS) $(UTIL_OBJS) $(XDIFF_OBJS) $(PCRE_OBJS)

# Dual-flavor 020 object lists -- same source tree, .020.o suffix.
LIBGIT2_OBJS_020   = $(LIBGIT2_SRCS:.c=.020.o)
TRANSPORT_OBJS_020 = $(TRANSPORT_SRCS:.c=.020.o)
UTIL_OBJS_020      = $(UTIL_SRCS:.c=.020.o)
XDIFF_OBJS_020     = $(XDIFF_SRCS:.c=.020.o)
PCRE_OBJS_020      = $(PCRE_SRCS:.c=.020.o)

ALL_OBJS_020 = $(LIBGIT2_OBJS_020) $(TRANSPORT_OBJS_020) $(UTIL_OBJS_020) $(XDIFF_OBJS_020) $(PCRE_OBJS_020)

# ========================================================================
# Build rules
# ========================================================================

# 'all' still means "build the 000 flavor only". 020 is opt-in via
# 'make all-020' or 'make dual'.
all:     $(TARGET)
all-020: $(TARGET_020)
dual:    $(TARGET) $(TARGET_020)

$(TARGET): $(ALL_OBJS)
	$(AR) rcs $@ $(ALL_OBJS)
	$(RANLIB) $@

$(TARGET_020): $(ALL_OBJS_020)
	$(AR) rcs $@ $(ALL_OBJS_020)
	$(RANLIB) $@

# ------------------------------------------------------------------
# 000 flavor pattern rules -- unchanged from the pre-dual-flavor era.
# ------------------------------------------------------------------

src/libgit2/%.o: src/libgit2/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/libgit2/transports/%.o: src/libgit2/transports/%.c
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

# ------------------------------------------------------------------
# 020 flavor pattern rules -- mirror the 000 rules with .020.o suffix
# and the -020 CFLAGS variants.
# ------------------------------------------------------------------

src/libgit2/%.020.o: src/libgit2/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/libgit2/transports/%.020.o: src/libgit2/transports/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/util/%.020.o: src/util/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/util/allocators/%.020.o: src/util/allocators/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/util/hash/%.020.o: src/util/hash/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/util/hash/sha1dc/%.020.o: src/util/hash/sha1dc/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/util/hash/rfc6234/%.020.o: src/util/hash/rfc6234/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/util/unix/%.020.o: src/util/unix/%.c
	$(CC) $(CFLAGS_020) -c $< -o $@

src/xdiff/%.020.o: src/xdiff/%.c
	$(CC) $(XDIFF_CFLAGS_020) -c $< -o $@

src/pcre/%.020.o: src/pcre/%.c
	$(CC) $(PCRE_CFLAGS_020) -c $< -o $@

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
XDIFF_HOTPATH_CFLAGS     = $(subst -O0,-O1,$(XDIFF_CFLAGS)) -fno-strict-aliasing
XDIFF_HOTPATH_CFLAGS_020 = $(subst -m68000,-m68020,$(XDIFF_HOTPATH_CFLAGS))

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

# ------------------------------------------------------------------
# 020 flavor hot-path promotions -- mirror every override above.
# Each file here was audited at the 000 Stage 7 perf-optimizer pass
# (2026-04-13, PERF-REPORT.md) and verified safe for -O1. The audit
# is CPU-family-independent -- the struct-return/large-locals/inline-
# asm/pointer-casting concerns don't change between -m68000 and
# -m68020, so the 020 flavor inherits the same safety verdict.
# ------------------------------------------------------------------

src/util/hash/sha1dc/sha1.020.o: src/util/hash/sha1dc/sha1.c
	$(CC) $(HOTPATH_CFLAGS_020) -c $< -o $@

src/util/hash/sha1dc/ubc_check.020.o: src/util/hash/sha1dc/ubc_check.c
	$(CC) $(HOTPATH_CFLAGS_020) -c $< -o $@

src/xdiff/xdiffi.020.o: src/xdiff/xdiffi.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS_020) -c $< -o $@

src/xdiff/xprepare.020.o: src/xdiff/xprepare.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS_020) -c $< -o $@

src/xdiff/xutils.020.o: src/xdiff/xutils.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS_020) -c $< -o $@

src/xdiff/xhistogram.020.o: src/xdiff/xhistogram.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS_020) -c $< -o $@

src/xdiff/xmerge.020.o: src/xdiff/xmerge.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS_020) -c $< -o $@

src/xdiff/xpatience.020.o: src/xdiff/xpatience.c
	$(CC) $(XDIFF_HOTPATH_CFLAGS_020) -c $< -o $@

src/util/wildmatch.020.o: src/util/wildmatch.c
	$(CC) $(HOTPATH_CFLAGS_020) -c $< -o $@

clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS_020) $(TARGET) $(TARGET_020)

.PHONY: all all-020 dual clean
