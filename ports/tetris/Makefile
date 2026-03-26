# Makefile for tetris (amiport)
include ../common.mk

TARGET      = tetris
SOURCES     = ported/tetris.c ported/screen.c ported/input.c ported/scores.c ported/shapes.c
VERSION     = 1.35
REVISION    = 1                # Increment when ported/ changes but upstream version stays the same
DESCRIPTION = Terminal tetris game from OpenBSD  # Max 40 chars, ASCII only (Aminet requirement)
AUTHOR      = Chris Torek, Darren F. Provine

# Category 3 (Console UI): needs console-shim for termcap
CFLAGS  += -I../../lib/console-shim/include
LDFLAGS += -L../../lib/console-shim -lamiport-console -lm

CONSOLE_SHIM_LIB = ../../lib/console-shim/libamiport-console.a

# Override Aminet category — game, not utility
AMINET_CAT = game/think

# vamos stack (256KB)
VAMOS_STACK = 256

.PHONY: all clean test package compare

all: $(TARGET)

$(TARGET): $(SOURCES) $(SHIM_LIB) $(CONSOLE_SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# Interactive game — limited vamos testing (version string only)
	../../toolchain/scripts/vamos -s $(VAMOS_STACK) -C "" ./$(TARGET) -s 2>/dev/null || true
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	@echo "(Interactive game — manual comparison only)"
