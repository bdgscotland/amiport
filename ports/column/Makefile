# Makefile for column (amiport)

TARGET      = column
SOURCES     = ported/column.c
VERSION     = 1.27
REVISION    = 2
DESCRIPTION = Columnate lists
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

include ../common.mk

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
