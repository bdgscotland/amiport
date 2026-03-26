# Makefile for rs (amiport)
include ../common.mk

TARGET      = rs
SOURCES     = ported/rs.c
VERSION     = 1.30
REVISION    = 1
DESCRIPTION = Reshape a data array
AUTHOR      = John Kunze, UC Berkeley

AMINET_CAT  = util/cli

# -lm required for floating-point operations (gutter proportional calculation)
LDFLAGS += -lm

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
