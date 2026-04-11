# Makefile for seq (amiport)

TARGET      = seq
SOURCES     = ported/seq.c
VERSION     = 1.8
REVISION    = 1
DESCRIPTION = Print sequences of numbers
AUTHOR      = Todd C. Miller

include ../common.mk

LDFLAGS    += -lm

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
