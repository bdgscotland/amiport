# Makefile for nl (amiport)
TARGET      = nl
SOURCES     = ported/nl.c
VERSION     = 1.8
REVISION    = 2
DESCRIPTION = Line numbering filter
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

include ../common.mk

# amiport: add posix-emu for regex support
CFLAGS  += -I../../lib/posix-emu/include
LDFLAGS  = -L../../lib/posix-emu -lamiport-emu -L../../lib/posix-shim -lamiport

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
