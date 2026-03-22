# Makefile for wc (amiport)
include ../common.mk

TARGET      = wc
SOURCES     = ported/wc.c
VERSION     = 1.32
# fmt_scaled() uses double arithmetic — need soft-float helpers from libm
LDFLAGS    += -lm
DESCRIPTION = Count lines, words, and characters
AUTHOR      = University of California, Berkeley

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
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/wc.c
	@# TODO: Add comparison commands
	@rm -f $(TARGET)_native
