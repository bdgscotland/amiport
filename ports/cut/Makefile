# Makefile for cut (amiport)
include ../common.mk

TARGET      = cut
SOURCES     = ported/cut.c
VERSION     = 1.28
DESCRIPTION = OpenBSD cut - extract fields/columns from text
AUTHOR      = University of California, Berkeley

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "one:two:three" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -d: -f2
	@echo "--- -b byte mode ---"
	@echo "abcdefgh" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -b1-3,5
	@echo "--- -f field mode with -s ---"
	@printf "no-delim\nhas:delim\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -d: -f1 -s
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/cut.c
	@echo "one:two:three" > /tmp/cut_test.txt
	@echo "--- Native -d: -f2 ---"
	@cat /tmp/cut_test.txt | ./$(TARGET)_native -d: -f2
	@echo "--- Amiga -d: -f2 ---"
	@cat /tmp/cut_test.txt | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -d: -f2
	@rm -f $(TARGET)_native /tmp/cut_test.txt
