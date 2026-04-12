# Makefile for sponge (amiport)
include ../common.mk

TARGET      = sponge
SOURCES     = ported/sponge.c ported/concat.c ported/writeall.c ported/eprintf.c
VERSION     = 0.1
REVISION    = 1
DESCRIPTION = Soak up stdin and write to file
AUTHOR      = Dimitris Papastamos, Hiltjo Posthuma

OBJECTS     = ported/sponge.o ported/concat.o ported/writeall.o ported/eprintf.o

$(TARGET): $(OBJECTS) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

ported/%.o: ported/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

clean:
	rm -f $(TARGET) ported/*.o

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
