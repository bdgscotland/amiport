# Makefile for ln (amiport)

TARGET      = ln
SOURCES     = ported/ln.c
VERSION     = 1.25
REVISION    = 1
DESCRIPTION = Create hard and symbolic links
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

include ../common.mk

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
