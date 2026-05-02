# tests/ammx-init/Makefile
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CFLAGS = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall \
         -I../../lib/posix-shim/include
LDFLAGS = -L../../lib/posix-shim -lamiport-020

VAMOS_STACK = 256
VAMOS_CPU = 68040

all: test_ammx_init

test_ammx_init: test_ammx_init.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: test_ammx_init
	vamos -C $(VAMOS_CPU) -s $(VAMOS_STACK) ./test_ammx_init

clean:
	rm -f test_ammx_init

.PHONY: all run clean
