# Makefile for patch (amiport)
TARGET      = patch
SOURCES     = ported/patch.c ported/pch.c ported/inp.c ported/util.c ported/ed.c ported/backupfile.c ported/mkpath.c
VERSION     = 1.78
REVISION    = 1
DESCRIPTION = Apply diff patches to source files  # Max 40 chars, ASCII only (Aminet requirement)
AUTHOR      = OpenBSD contributors

include ../common.mk

CFLAGS     += -std=gnu99 -I../../lib/posix-emu/include
LDFLAGS    += -L../../lib/posix-emu -lamiport-emu
VAMOS_STACK = 256

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# Basic smoke test: apply a simple unified diff
	@echo "hello" > test-patch-orig.txt
	@echo "--- test-patch-orig.txt" > test-patch-diff.txt
	@echo "+++ test-patch-orig.txt" >> test-patch-diff.txt
	@echo "@@ -1 +1 @@" >> test-patch-diff.txt
	@echo "-hello" >> test-patch-diff.txt
	@echo "+goodbye" >> test-patch-diff.txt
	@../../toolchain/scripts/vamos -s $(VAMOS_STACK) -C "" ./$(TARGET) test-patch-orig.txt test-patch-diff.txt && echo "PASS: basic patch apply" || echo "FAIL: basic patch apply"
	@cat test-patch-orig.txt
	@rm -f test-patch-orig.txt test-patch-diff.txt test-patch-orig.txt.orig
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	@echo "Multi-file port — native compare not supported"
