# Makefile for rm (amiport)
include ../common.mk

TARGET      = rm
SOURCES     = ported/rm.c
VERSION     = 1.45
REVISION    = 1
DESCRIPTION = Remove directory entries
AUTHOR      = The Regents of the University of California

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/rm.c
	@rm -f $(TARGET)_native
