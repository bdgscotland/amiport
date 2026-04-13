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

include ../common.mk
VERSION     = 0.1
REVISION    = 1
DESCRIPTION = Local git for AmigaOS via libgit2
AUTHOR      = amiport

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
          ported/amigit_libgit2_stubs.o

# Override SOURCES so common.mk's default single-file rule is bypassed.
SOURCES = $(OBJECTS:.o=.c)

# CFLAGS: start from common.mk defaults, then:
# - replace -O2 with -O0 to match lib/libgit2 baseline (crash-patterns #16)
# - add -std=gnu99 for libgit2's C99-heavy headers
# - force-include amigaos_compat.h so the macro namespace matches the
#   built libgit2.a (tests/libgit2/Makefile uses the same pattern)
# - add include paths for libgit2 public headers and internal util/
# - suppress noisy warnings from libgit2 headers
CFLAGS := $(subst -O2,-O0,$(CFLAGS)) \
          -std=gnu99 \
          -Wno-unused-parameter -Wno-unused-function \
          -I../../lib/libgit2/include \
          -I../../lib/libgit2/src/util \
          -include ../../lib/libgit2/src/util/amigaos_compat.h

# LDFLAGS: libgit2 link chain. Order matters: libgit2 -> libz ->
# libamiport (already from common.mk) -> libm. -lm is mandatory for
# libgit2's khash soft-float load-factor math even though amigit has
# no explicit floating point (known-pitfalls: libgit2 khash).
LDFLAGS := -L../../lib/libgit2 -lgit2 \
           -L../../lib/zlib -lz \
           $(LDFLAGS) \
           -lm

# vamos stack and memory: match tests/libgit2/Makefile.
# libgit2's tree/pack walk is deeply recursive; 256 KB covers it.
# 4 MB RAM covers the binary (~1 MB) + stack + libgit2 workspace.
VAMOS_STACK = 256
VAMOS_MEM   = 4096

# Override common.mk's default $(TARGET) rule with our per-file build.
$(TARGET): $(OBJECTS) $(SHIM_LIB) ../../lib/libgit2/libgit2.a ../../lib/zlib/libz.a
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

ported/%.o: ported/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Make sure the upstream libraries get built when they're stale.
../../lib/libgit2/libgit2.a:
	$(MAKE) -C ../../lib/libgit2

../../lib/zlib/libz.a:
	$(MAKE) -C ../../lib/zlib

# Smoke test: vamos runs `amigit version` and verifies exit 0 plus
# output contains "libgit2". Real testing happens in test-fsemu-cases.txt
# via FS-UAE; this target is just a quick pulse-check.
test: $(TARGET)
	@echo "=== Testing $(TARGET) (vamos smoke) ==="
	$(TOOLCHAIN_BIN)/vamos -q -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET) version
	@echo "=== $(TARGET) smoke passed ==="

compare: $(TARGET)
	@echo "amigit has no upstream to compare against (see ports/amigit/PORT.md)"
	@true

clean:
	rm -f ported/*.o $(TARGET) $(TARGET).map
