# lib/libhubbub/Makefile -- NetSurf HTML5 tokeniser + tree builder
#
# Upstream: https://github.com/netsurf-browser/libhubbub @ commit 6651b8c
# (v0.3.8). Copyright 2009-2020 J-M Bell + Daniel Silverstone, MIT-licensed
# (see COPYING).
#
# 30 .c files / ~8K LOC + ~5K LOC headers + 11647-line generated entities.inc
# (committed; regenerated from upstream `perl build/make-entities.pl` if
# the HTML5 entity list ever needs refreshing).
#
# Subsystems:
#   - charset/    : charset detection (BOM + meta tag heuristics)
#   - parser.c    : top-level parser glue
#   - tokeniser/  : HTML5 tokeniser state machine + named entity bsearch
#   - treebuilder/: HTML5 tree construction (21 mode-specific files)
#   - utils/      : errors, string ops
#
# Source-analyzer audit verdict: CLEAN (2026-05-02). No source patches.
#
# Required by: ports/netsurf (Phase 1 final consumer); future libdom +
#              future libsvgtiny.
#
# CPU TARGET: -m68040 -m68881 (NetSurf-Vampire dep stack convention; see
# lib/libwapcaplet/Makefile header for rationale).
#
# DEPENDS ON: lib/libparserutils (already shipped this branch).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR            = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB        = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# -O1 promoted whole-archive 2026-05-02 after perf-optimizer audit:
# all 30 TUs are -O1-safe per crash-patterns #16 (no struct-by-value
# returns >8 bytes, no soft-float pulls, no large stack locals, no
# alignment traps). bebbo-gcc 13.3 (the NetSurf toolchain) has stable
# -O1 codegen. Estimated ~1.5x speedup on the parse phase (tokeniser
# byte-loop + in_body mode dispatch dominate). Same whole-archive
# pattern as lib/libwapcaplet + lib/libparserutils.
# -fno-strict-aliasing defensive (prevents aliasing-based misopts on
# the pointer-heavy byte walkers in tokeniser + treebuilder + the
# void*-cast tree-handler callbacks).
# -m68040 -m68881 matches NetSurf consumer ABI.
# -std=c99 mandatory: source uses bool, uint*_t, static inline.
# -DNDEBUG strips assert() and debug fprintf paths.
# -D_DEFAULT_SOURCE matches upstream Makefile.
# -I../../lib/libparserutils/include for the inputstream/codec/types
#   headers used by parser.c, tokeniser.c, charset/detect.c.
CFLAGS  = -O1 -fno-strict-aliasing -noixemul -m68040 -m68881 -std=c99 -Wall -Wextra
CFLAGS += -DNDEBUG -D_DEFAULT_SOURCE
CFLAGS += -Iinclude -Isrc
CFLAGS += -I../../lib/libparserutils/include

TARGET = libhubbub.a

OBJS = \
    src/charset/detect.o \
    src/parser.o \
    src/tokeniser/entities.o \
    src/tokeniser/tokeniser.o \
    src/treebuilder/after_after_body.o \
    src/treebuilder/after_after_frameset.o \
    src/treebuilder/after_body.o \
    src/treebuilder/after_frameset.o \
    src/treebuilder/after_head.o \
    src/treebuilder/before_head.o \
    src/treebuilder/before_html.o \
    src/treebuilder/element-type.o \
    src/treebuilder/generic_rcdata.o \
    src/treebuilder/in_body.o \
    src/treebuilder/in_caption.o \
    src/treebuilder/in_cell.o \
    src/treebuilder/in_column_group.o \
    src/treebuilder/in_foreign_content.o \
    src/treebuilder/in_frameset.o \
    src/treebuilder/in_head.o \
    src/treebuilder/in_head_noscript.o \
    src/treebuilder/in_row.o \
    src/treebuilder/in_select.o \
    src/treebuilder/in_select_in_table.o \
    src/treebuilder/in_table.o \
    src/treebuilder/in_table_body.o \
    src/treebuilder/initial.o \
    src/treebuilder/treebuilder.o \
    src/utils/errors.o \
    src/utils/string.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
