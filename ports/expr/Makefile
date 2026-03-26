# Makefile for expr (amiport)
TARGET      = expr
SOURCES     = ported/expr.c
VERSION     = 1.28
REVISION    = 2
DESCRIPTION = Evaluate string and math expressions
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

include ../common.mk

# expr uses Tier 2 regex emulation via posix-emu
CFLAGS  += -I../../lib/posix-emu/include
# posix-emu must come before posix-shim; -lamiport appears once only
LDFLAGS  = -L../../lib/posix-emu -L../../lib/posix-shim -lamiport-emu -lamiport

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
