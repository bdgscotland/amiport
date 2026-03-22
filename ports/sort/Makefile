# Makefile for sort (amiport)
TARGET      = sort
SOURCES     = ported/sort.c
VERSION     = 1.0
DESCRIPTION = POSIX sort utility — sort lines of text files
AUTHOR      = Plan 9 from User Space (Lucent Technologies)

include ../common.mk

# Override if not util/cli (e.g., util/misc, text/edit)
# AMINET_CAT = util/cli

# Add extra libraries for non-CLI ports:
# Category 3 (console UI): LDFLAGS += -lamiport-console
# Category 4 (network):    LDFLAGS += -lamiport-net

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# TODO: Add test commands using vamos
	@# Example: echo "input" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -flags
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/sort.c
	@# TODO: Add comparison commands
	@rm -f $(TARGET)_native
