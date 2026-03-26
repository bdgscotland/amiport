# Makefile for tr (amiport)
include ../common.mk

TARGET      = tr
SOURCES     = ported/tr.c ported/str.c
VERSION     = 1.22
REVISION    = 1
DESCRIPTION = Translate, squeeze, or delete chars
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
