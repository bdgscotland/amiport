# Makefile for test (amiport)
include ../common.mk

TARGET      = test
SOURCES     = ported/test.c
VERSION     = 1.23
REVISION    = 1
DESCRIPTION = Condition evaluation utility
AUTHOR      = Erik Baalbergen

AMINET_CAT  = util/cli

.PHONY: all clean run-tests package compare

# amiport: the binary is also named 'test' — avoid circular dependency by
# using 'run-tests' as the phony test runner target.  The 'test' name is
# reserved for the binary build rule inherited from common.mk.
run-tests: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
