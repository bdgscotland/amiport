TARGET      = rev
SOURCES     = ported/rev.c
VERSION     = 1.16
REVISION    = 1
DESCRIPTION = Reverse lines character by character
AUTHOR      = University of California, Berkeley

include ../common.mk

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/rev.c
	@rm -f $(TARGET)_native
