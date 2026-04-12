# Makefile for logname (amiport)
include ../common.mk

TARGET      = logname
SOURCES     = ported/logname.c
VERSION     = 1.10
REVISION    = 1
DESCRIPTION = Display user's login name
AUTHOR      = The Regents of the University of California

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/logname.c
	@rm -f $(TARGET)_native
