# Makefile for cmp (amiport)

TARGET      = cmp
SOURCES     = ported/cmp.c ported/regular.c ported/special.c ported/misc.c
VERSION     = 1.19
REVISION    = 1
DESCRIPTION = Compare two files byte by byte
AUTHOR      = UC Berkeley

include ../common.mk

# amiport: add posix-emu include path for mmap emulation header
CFLAGS  += -I../../lib/posix-emu/include
# amiport: link posix-emu before posix-shim (mmap emulation uses shim internals)
LDFLAGS  = -L../../lib/posix-emu -lamiport-emu -L../../lib/posix-shim -lamiport

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
