# Makefile for sed (amiport)
include ../common.mk

TARGET      = sed
SOURCES     = ported/main.c ported/compile.c ported/misc.c ported/process.c
VERSION     = 1.47
DESCRIPTION = OpenBSD stream editor (sed) for AmigaOS
AUTHOR      = Diomidis Spinellis / University of California, Berkeley

# Category 1 CLI tool — uses Tier 2 regex emulation and Tier 1 posix-shim
CFLAGS += -I../../lib/posix-emu/include
LDFLAGS += -L../../lib/posix-emu -L../../lib/posix-shim -lamiport-emu -lamiport

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "hello world" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) 's/hello/goodbye/'
	@echo "line1" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -n 'p'
	@echo "abc123def" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) 's/[0-9]*/NUM/'
	@printf "line1\nline2\nline3\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) '2d'
	@printf "aaa\nbbb\nccc\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) 'y/abc/ABC/'
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	@echo "(Native comparison not applicable for multi-file port)"
