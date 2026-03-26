TARGET      = dirname
SOURCES     = ported/dirname.c
VERSION     = 1.17
REVISION    = 1
DESCRIPTION = Strip last component from filename
AUTHOR      = Todd C. Miller

# Aminet category
AMINET_CAT = util/cli
CATEGORY   = 1

# vamos stack for testing
VAMOS_STACK = 64

include ../common.mk

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/dirname.c
	@rm -f $(TARGET)_native
