# Makefile for comm (amiport)
TARGET      = comm
SOURCES     = ported/comm.c
VERSION     = 1.11
REVISION    = 1
DESCRIPTION = Compare two sorted files line by line
AUTHOR      = University of California, Berkeley

include ../common.mk

# Aminet category
# AMINET_CAT = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# TODO: Add test commands using vamos
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/comm.c
	@# TODO: Add comparison commands
	@rm -f $(TARGET)_native
