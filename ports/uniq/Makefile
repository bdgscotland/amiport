# Makefile for uniq (amiport)
include ../common.mk

TARGET      = uniq
SOURCES     = ported/uniq.c
VERSION     = 1.33
REVISION    = 1
DESCRIPTION = Filter/report repeated lines in files  # Max 40 chars, ASCII only (Aminet requirement)
AUTHOR      = OpenBSD Project

# Override if not util/cli (e.g., util/misc, text/edit)
# AMINET_CAT = util/cli

# Add extra libraries for non-CLI ports:
# Category 3 (console UI): LDFLAGS += -lamiport-console
# Category 4 (network):    LDFLAGS += -lamiport-net

VAMOS_STACK = 256

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# TODO: Add test commands using vamos
	@# Example: echo "input" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -flags
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/uniq.c
	@# TODO: Add comparison commands
	@rm -f $(TARGET)_native
