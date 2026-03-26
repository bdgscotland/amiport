# Makefile for tty (amiport)

TARGET      = tty
SOURCES     = ported/tty.c
VERSION     = 1.14
REVISION    = 1
DESCRIPTION = Print name of terminal
AUTHOR      = OpenBSD

AMINET_CAT  = util/cli

include ../common.mk

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
