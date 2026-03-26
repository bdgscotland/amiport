# Makefile for tty (amiport)
include ../common.mk

TARGET      = tty
SOURCES     = ported/tty.c
VERSION     = 1.14
REVISION    = 1
DESCRIPTION = Print name of terminal
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
