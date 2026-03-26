# Makefile for cat (amiport)
include ../common.mk

TARGET      = cat
SOURCES     = ported/cat.c
VERSION     = 1.34
REVISION    = 1
DESCRIPTION = Concatenate and print files
AUTHOR      = University of California, Berkeley

AMINET_CAT  = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
