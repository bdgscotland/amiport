# Makefile for mini-find (amiport example — complex)
include ../common.mk

TARGET = mini-find
SOURCE = ported/mini-find.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCE) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $< $(LDFLAGS)

test: $(TARGET)
	@echo "=== Testing mini-find ==="
	@mkdir -p test_tree/subdir
	@echo "hello" > test_tree/file1.c
	@echo "world" > test_tree/file2.txt
	@echo "nested" > test_tree/subdir/file3.c
	@echo "--- List all ---"
	vamos -- ./$(TARGET) test_tree
	@echo "--- Files only ---"
	vamos -- ./$(TARGET) -t f test_tree
	@echo "--- Dirs only ---"
	vamos -- ./$(TARGET) -t d test_tree
	@echo "--- Pattern *.c ---"
	vamos -- ./$(TARGET) -n "*.c" test_tree
	@echo "--- Nonexistent dir ---"
	vamos -- ./$(TARGET) nonexistent || true
	@rm -rf test_tree
	@echo "=== mini-find tests complete ==="

clean:
	rm -f $(TARGET)
	rm -rf test_tree
