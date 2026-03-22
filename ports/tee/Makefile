# Makefile for tee (amiport)
include ../common.mk

TARGET      = tee
SOURCES     = ported/tee.c
VERSION     = 1.15
DESCRIPTION = duplicate standard input
AUTHOR      = University of California, Berkeley

# Override if not util/cli (e.g., util/misc, text/edit)
# AMINET_CAT = util/cli

# Add extra libraries for non-CLI ports:
# Category 3 (console UI): LDFLAGS += -lamiport-console
# Category 4 (network):    LDFLAGS += -lamiport-net

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "hello world" | ../../toolchain/scripts/vamos -C "" ./$(TARGET)
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/tee.c
	@# TODO: Add comparison commands
	@rm -f $(TARGET)_native
