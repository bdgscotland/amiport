# Makefile for wc (amiport MVP example)
include ../common.mk

TARGET = wc
SOURCE = ported/wc.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCE) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: $(TARGET)
	@echo "=== Testing wc ==="
	@echo "The quick brown fox jumps over the lazy dog." > test_input.txt
	@echo "Second line here." >> test_input.txt
	timeout 10 vamos ./$(TARGET) test_input.txt
	@rm -f test_input.txt
	@echo "=== wc tests complete ==="

clean:
	rm -f $(TARGET) test_input.txt
