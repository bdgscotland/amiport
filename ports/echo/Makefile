# Makefile for echo (amiport)
include ../common.mk

TARGET      = echo
SOURCES     = ported/echo.c
VERSION     = 1.12
REVISION    = 1
DESCRIPTION = Write arguments to standard output
AUTHOR      = University of California, Berkeley

AMINET_CAT  = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
