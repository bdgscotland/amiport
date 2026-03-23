# Makefile for bc (amiport)
include ../common.mk

TARGET      = bc
SOURCES     = ported/bc.c ported/execute.c ported/global.c ported/load.c \
              ported/main.c ported/scan.c ported/storage.c ported/util.c \
              ported/warranty.c ported/number.c ported/getopt.c ported/getopt1.c
VERSION     = 1.07.1
DESCRIPTION = GNU arbitrary precision calculator
AUTHOR      = Philip A. Nelson

# All headers are in ported/ alongside the source
CFLAGS += -Iported/

# Need gnu99 for some C99 fallback paths in the lex-generated scanner
CFLAGS += -std=gnu99

# The bundled getopt.c/getopt1.c duplicate symbols that libnix also provides.
# --allow-multiple-definition lets the linker use the bundled (object-file)
# versions, which come first in link order, keeping GNU getopt_long semantics
# (libnix getopt_long is broken — see known-pitfalls).
LDFLAGS += -Wl,--allow-multiple-definition

# Stack cookie: bc does recursive descent and arbitrary precision math
VAMOS_STACK = 256

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# Test 1: basic arithmetic
	@echo "2+3" | ../../toolchain/scripts/vamos -s $(VAMOS_STACK) -- ./$(TARGET)
	@# Test 2: multiplication
	@echo "6*7" | ../../toolchain/scripts/vamos -s $(VAMOS_STACK) -- ./$(TARGET)
	@# Test 3: math library
	@echo "scale=10; 4*a(1)" | ../../toolchain/scripts/vamos -s $(VAMOS_STACK) -- ./$(TARGET) -l
	@# Test 4: quit command
	@echo "quit" | ../../toolchain/scripts/vamos -s $(VAMOS_STACK) -- ./$(TARGET)
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	@echo "(Native comparison not applicable for multi-file port)"
