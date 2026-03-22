# Makefile for yes (amiport)
include ../common.mk

TARGET      = yes
SOURCES     = ported/yes.c
VERSION     = 1.9
DESCRIPTION = Repeatedly output a string (OpenBSD)
AUTHOR      = The Regents of the University of California

# Override if not util/cli (e.g., util/misc, text/edit)
# AMINET_CAT = util/cli

# Add extra libraries for non-CLI ports:
# Category 3 (console UI): LDFLAGS += -lamiport-console
# Category 4 (network):    LDFLAGS += -lamiport-net

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# yes outputs infinite lines — test by piping through head
	@echo "Default output (y):" && echo "" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) 2>/dev/null | head -5
	@echo "Custom output:" && echo "" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) hello 2>/dev/null | head -5
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/yes.c
	@# TODO: Add comparison commands
	@rm -f $(TARGET)_native
