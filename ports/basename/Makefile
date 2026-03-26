TARGET      = basename
SOURCES     = ported/basename.c
VERSION     = 1.14
REVISION    = 1
DESCRIPTION = Strip directory and suffix from filenames
AUTHOR      = The Regents of the University of California

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
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/basename.c
	@rm -f $(TARGET)_native
