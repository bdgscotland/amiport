# Makefile for fold (amiport)
include ../common.mk

TARGET      = fold
SOURCES     = ported/fold.c
VERSION     = 1.18
REVISION    = 1
DESCRIPTION = Fold long lines for fixed-width output
AUTHOR      = Kevin Ruddy, UC Berkeley

AMINET_CAT  = util/cli

.PHONY: all clean test package compare

all: fold

fold: ported/fold.c $(SHIM_LIB)
	$(CC) $(CFLAGS) -o fold ported/fold.c $(LDFLAGS)

clean:
	rm -f ported/*.o fold fold.map

test: fold
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
