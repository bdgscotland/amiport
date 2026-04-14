# Makefile for amigit (amiport)
#
# amigit is an amiport-native local-only git CLI built on libgit2.a.
# See docs/pdr/010-amigit-on-libgit2.md and docs/pdr/010a-amigit-cli-spec.md.
#
# Phase 3a (this Makefile's current state): proof-of-life binary with
# `version` and `init` commands only. Phases 3b and 3c will grow the
# command set. The pattern for adding a new command: create
# ported/cmd_<name>.c, add to OBJECTS, wire into the dispatch table
# in ported/amigit.c.

# TARGET must be defined before include so common.mk's all: $(TARGET) expands correctly
TARGET      = amigit

# VERSION/REVISION must be set BEFORE including common.mk -- common.mk
# computes DISPLAY_VERSION via `ifeq ($(REVISION),1)` at include time,
# so a `REVISION = 2` after the include is too late and DISPLAY_VERSION
# stays at "0.1" instead of "0.1-2". Discovered while bumping to 0.1-2.
VERSION     = 0.1
REVISION    = 6
DESCRIPTION = Local git CLI for AmigaOS 3.x (preview)
AUTHOR      = Duncan Bowring (amiport, on libgit2)
AMINET_CAT  = dev/misc

include ../common.mk

# Port category: 1 (CLI). No color, no paging, no interactive input.
# See PDR-010a for the category rationale.
CATEGORY    = 1

# Per-file compilation: amigit is a multi-file port because each
# subcommand gets its own translation unit for testability and so a
# future refactor can promote individual TUs to -O1 without affecting
# the others. amigit_libgit2_stubs.c holds the link-time stubs shared
# with tests/libgit2/test_libgit2 (strnlen, difftime, select, etc.).

OBJECTS = ported/amigit.o \
          ported/cmd_init.o \
          ported/cmd_version.o \
          ported/cmd_status.o \
          ported/cmd_log.o \
          ported/cmd_show.o \
          ported/cmd_diff.o \
          ported/cmd_add.o \
          ported/cmd_commit.o \
          ported/cmd_checkout.o \
          ported/cmd_branch.o \
          ported/cmd_tag.o \
          ported/cmd_ls_remote.o \
          ported/transport_https.o \
          ported/amigit_libgit2_stubs.o

# Override SOURCES so common.mk's default single-file rule is bypassed.
SOURCES = $(OBJECTS:.o=.c)

# CFLAGS: start from common.mk defaults, then:
# - replace -O2 with -O1 for amigit's own 13 port TUs only. libgit2.a and
#   libz.a remain -O0 (built separately under lib/). perf-optimizer
#   audited all port TUs SAFE for -O1 on 2026-04-13: no struct-by-value
#   returns > 8 bytes (crash-patterns #16), no recursion, no float div
#   in amigit's own code. Re-verified by the full 82-test FS-UAE suite
#   after the 0.2 -O1 flip.
# - replace -m68000 with -m68020 -- amigit targets 68020+ only (no A500
#   support). The realistic audience for a git client is accelerated
#   Amigas (Vampire V2/V4, A1200/030+, A3000/A4000, FS-UAE). A plain
#   68000 A500 is not viable for git workloads. See memory:
#   project_amigit_68020_target.md (2026-04-13).
# - add -std=gnu99 for libgit2's C99-heavy headers
# - force-include amigaos_compat.h so the macro namespace matches the
#   built libgit2.a (tests/libgit2/Makefile uses the same pattern)
# - add include paths for libgit2 public headers and internal util/
# - suppress noisy warnings from libgit2 headers
CFLAGS := $(subst -O2,-O1,$(subst -m68000,-m68020,$(CFLAGS))) \
          -std=gnu99 \
          -Wno-unused-parameter -Wno-unused-function \
          -I../../lib/libgit2/include \
          -I../../lib/libgit2/src/util \
          -include ../../lib/libgit2/src/util/amigaos_compat.h

# LDFLAGS: libgit2 link chain. Order matters: libgit2 -> libz ->
# libamiport -> libm. -lm is mandatory for libgit2's khash soft-float
# load-factor math even though amigit has no explicit floating point
# (known-pitfalls: libgit2 khash).
#
# 0.1-6 (Track A, 2026-04-14): link against the -m68020 library
# flavors (libgit2-020.a, libz-020.a, libamiport-020.a) from
# commits 4a8a362 and 0dcb0a6. Before this, amigit 0.1-5 was a
# 68020 binary linked against 68000 libraries -- exactly the
# "cosmetic -m68020 at -O0" pitfall from known-pitfalls.md. The
# binary's own 13 TUs were already 68020-O1 but the hot work
# (libgit2 pack/diff/xdiff, zlib inflate, posix-shim fd ops) was
# running on 68000 instructions. This commit fixes that.
#
# Note: we do NOT replace -lamiport (from common.mk's $(LDFLAGS))
# via text substitution because common.mk bakes in a specific
# library search order. Instead we override SHIM_LIB and prepend
# the -020 library search path so the linker picks up the 020
# variants first.
LDFLAGS := -L../../lib/libgit2 -lgit2-020 \
           -L../../lib/zlib -lz-020 \
           -L../../lib/posix-shim -lamiport-020 \
           $(filter-out -lamiport,$(LDFLAGS)) \
           -lm

# Point the library-dependency edges at the 020 variants so make
# rebuilds them automatically when they're stale.
SHIM_LIB = ../../lib/posix-shim/libamiport-020.a

# vamos stack and memory: match tests/libgit2/Makefile.
# libgit2's tree/pack walk is deeply recursive; 256 KB covers it.
# 4 MB RAM covers the binary (~1 MB) + stack + libgit2 workspace.
# vamos defaults to 68000 but amigit is -m68020, so tell vamos to
# emulate a 68020 CPU (known-pitfalls: "vamos Needs -C 68020").
VAMOS_STACK = 256
VAMOS_MEM   = 4096
VAMOS_CPU   = 68020

# Override common.mk's default $(TARGET) rule with our per-file build.
# Depend on the -020 library archives so make rebuilds them when stale.
$(TARGET): $(OBJECTS) $(SHIM_LIB) ../../lib/libgit2/libgit2-020.a ../../lib/zlib/libz-020.a
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

ported/%.o: ported/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Make sure the 020 library variants get built when they're stale.
../../lib/libgit2/libgit2-020.a:
	$(MAKE) -C ../../lib/libgit2 all-020

../../lib/zlib/libz-020.a:
	$(MAKE) -C ../../lib/zlib all-020

../../lib/posix-shim/libamiport-020.a:
	$(MAKE) -C ../../lib/posix-shim all-020

# Smoke test: vamos runs `amigit version` and verifies exit 0 plus
# output contains "libgit2". Real testing happens in test-fsemu-cases.txt
# via FS-UAE; this target is just a quick pulse-check.
test: $(TARGET)
	@echo "=== Testing $(TARGET) (vamos smoke) ==="
	VAMOS_MEM=$(VAMOS_MEM) $(TOOLCHAIN_BIN)/vamos -C $(VAMOS_CPU) -s $(VAMOS_STACK) -- ./$(TARGET) version
	@echo "=== $(TARGET) smoke passed ==="

compare: $(TARGET)
	@echo "amigit has no upstream to compare against (see ports/amigit/PORT.md)"
	@true

# amigit has no upstream source, so override the shared `package` target
# from ../common.mk to skip the `original/` copy (which would otherwise
# refuse to copy the README-only stub the block-original-edits hook
# forbids populating). Structure matches common.mk's package target byte
# for byte except the cp -r original/ line is dropped.
package: $(TARGET)
	@test -s $(TARGET).readme || (echo "ERROR: $(TARGET).readme missing or empty" && exit 1)
	@rm -rf .pkg && mkdir -p .pkg
	$(STRIP) --strip-debug -o .pkg/$(TARGET) $(TARGET)
	@cp $(TARGET).readme .pkg/
	@cp PORT.md .pkg/
	@if [ -f TEST-REPORT.md ]; then cp TEST-REPORT.md .pkg/; fi
	@cp -r ported/ .pkg/ported/
	@if [ -f TEST-REPORT.md ]; then \
		$(LHA_CMD) a ../$(TARGET)-$(PKG_SUFFIX).lha $(TARGET) $(TARGET).readme PORT.md TEST-REPORT.md ported/; \
	else \
		$(LHA_CMD) a ../$(TARGET)-$(PKG_SUFFIX).lha $(TARGET) $(TARGET).readme PORT.md ported/; \
	fi
	@rm -rf .pkg
	@cp $(TARGET).readme $(TARGET)-$(PKG_SUFFIX).readme
	@echo "Package ready:"
	@echo "  $(TARGET)-$(PKG_SUFFIX).lha     — upload to amiport.platesteel.net"
	@echo "  $(TARGET)-$(PKG_SUFFIX).readme  — upload alongside the .lha"

# Machine LHA: stripped binary only, no source/docs, for `amiport install`.
# Installs amigit into C:amigit (AmigaOS convention).
package-machine: $(TARGET)
	@rm -rf .pkg-machine && mkdir -p .pkg-machine/C
	$(STRIP) --strip-debug -o .pkg-machine/C/$(TARGET) $(TARGET)
	docker run --rm -v $(shell cd ../.. && pwd):/work -w /work/ports/$(TARGET)/.pkg-machine amigadev/crosstools:m68k-amigaos lha a ../$(TARGET)-$(PKG_SUFFIX)-machine.lha C/$(TARGET)
	@rm -rf .pkg-machine
	@echo "Machine package: $(TARGET)-$(PKG_SUFFIX)-machine.lha"

clean:
	rm -f ported/*.o $(TARGET) $(TARGET).map
