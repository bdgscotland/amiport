# Makefile for touch (amiport)
include ../common.mk

TARGET      = touch
SOURCES     = ported/touch.c
VERSION     = 1.27
REVISION    = 1
DESCRIPTION = Change file access and modification time
AUTHOR      = The Regents of the University of California

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/touch.c
	@rm -f $(TARGET)_native
