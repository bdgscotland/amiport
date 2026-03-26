# Makefile for look (amiport)
include ../common.mk

TARGET      = look
SOURCES     = ported/look.c
VERSION     = 1.27
REVISION    = 1
DESCRIPTION = Display lines starting with a string
AUTHOR      = David Hitz, OpenBSD

AMINET_CAT  = util/cli

EMU_LIB     = ../../lib/posix-emu/libamiport-emu.a
CFLAGS     += -I../../lib/posix-emu/include
LDFLAGS    += -L../../lib/posix-emu -lamiport-emu

$(TARGET): $(SOURCES) $(SHIM_LIB) $(EMU_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="
