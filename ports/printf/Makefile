# Makefile for printf (amiport)
include ../common.mk

TARGET      = printf
SOURCES     = ported/printf.c
VERSION     = 1.28
REVISION    = 1
DESCRIPTION = Format and print data
AUTHOR      = University of California, Berkeley

AMINET_CAT  = util/cli

# -lm required for strtod and floating-point printf (%f, %e, %g)
LDFLAGS += -lm

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
