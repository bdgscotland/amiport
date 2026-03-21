# Makefile for tail (amiport)
include ../common.mk

TARGET      = tail
SOURCES     = ported/tail.c ported/forward.c ported/reverse.c ported/read.c ported/misc.c
VERSION     = 1.24
DESCRIPTION = Display the last part of a file
AUTHOR      = Edward Sze-Tyan Wang / UC Berkeley

# Override if not util/cli (e.g., util/misc, text/edit)
# AMINET_CAT = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# TODO: Add test commands using vamos
	@# Example: echo "input" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -flags
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/tail.c original/forward.c original/reverse.c original/read.c original/misc.c
	@# TODO: Add comparison commands
	@rm -f $(TARGET)_native
