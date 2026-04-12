# Makefile for pr (amiport)
include ../common.mk

TARGET      = pr
VERSION     = 1.46
REVISION    = 1
DESCRIPTION = Print files for page display
AUTHOR      = The Regents of the University of California

# strtonum() uses long long
CFLAGS     += -std=gnu99

OBJECTS     = ported/pr.o ported/egetopt.o

$(TARGET): $(OBJECTS) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

ported/%.o: ported/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean test package compare

clean:
	rm -f $(TARGET) ported/*.o

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
