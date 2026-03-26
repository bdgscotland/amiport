# Makefile for tr (amiport)

TARGET      = tr
SOURCES     = ported/tr.c ported/str.c
VERSION     = 1.22
REVISION    = 2
DESCRIPTION = Translate, squeeze, or delete chars
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

include ../common.mk

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
