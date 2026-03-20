# Makefile for head (amiport example — moderate complexity)
include ../common.mk

TARGET = head
SOURCE = ported/head.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCE) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: $(TARGET)
	@echo "=== Testing head ==="
	@echo "line 1" > test_input.txt
	@echo "line 2" >> test_input.txt
	@echo "line 3" >> test_input.txt
	@echo "line 4" >> test_input.txt
	@echo "line 5" >> test_input.txt
	@echo "line 6" >> test_input.txt
	@echo "line 7" >> test_input.txt
	@echo "line 8" >> test_input.txt
	@echo "line 9" >> test_input.txt
	@echo "line 10" >> test_input.txt
	@echo "line 11" >> test_input.txt
	@echo "line 12" >> test_input.txt
	@echo "--- Default (10 lines) ---"
	timeout 10 vamos ./$(TARGET) test_input.txt
	@echo "--- First 3 lines ---"
	timeout 10 vamos ./$(TARGET) -n 3 test_input.txt
	@echo "--- Empty file ---"
	@touch empty.txt
	timeout 10 vamos ./$(TARGET) empty.txt
	@echo "--- Nonexistent file ---"
	timeout 10 vamos ./$(TARGET) nonexistent.txt || true
	@rm -f test_input.txt empty.txt
	@echo "=== head tests complete ==="

clean:
	rm -f $(TARGET) test_input.txt empty.txt
