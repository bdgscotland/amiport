# Makefile for wc (amiport MVP example)

CC = m68k-amigaos-gcc
CFLAGS = -O2 -noixemul -m68020 -Wall -I../../lib/posix-shim/include
LDFLAGS = -L../../lib/posix-shim -lamiport

TARGET = wc
SOURCE = ported/wc.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCE) ../../lib/posix-shim/libamiport.a
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: $(TARGET)
	@echo "Testing wc with vamos..."
	@echo "The quick brown fox jumps over the lazy dog." > test_input.txt
	@echo "Second line here." >> test_input.txt
	vamos ./$(TARGET) test_input.txt
	@rm -f test_input.txt

clean:
	rm -f $(TARGET) test_input.txt
