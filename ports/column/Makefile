# Makefile for column (amiport)
include ../common.mk

TARGET      = column
SOURCES     = ported/column.c
VERSION     = 1.27
REVISION    = 1
DESCRIPTION = Columnate lists
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
