TARGET      = expand
SOURCES     = ported/expand.c
VERSION     = 1.15
REVISION    = 1
DESCRIPTION = Convert tabs to spaces
AUTHOR      = University of California, Berkeley

include ../common.mk

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/expand.c
	@rm -f $(TARGET)_native
