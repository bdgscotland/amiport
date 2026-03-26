# Makefile for sleep (amiport)
include ../common.mk

TARGET      = sleep
SOURCES     = ported/sleep.c
VERSION     = 1.29
REVISION    = 1
DESCRIPTION = Suspend execution for an interval
AUTHOR      = University of California, Berkeley

AMINET_CAT  = util/cli

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
