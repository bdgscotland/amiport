# Makefile for amigit http_client parser unit tests (PDR-012 Phase 3)
#
# Cross-compiles test_http_client for AmigaOS 68k and runs it via vamos.
# Parser-only: no bsdsocket, no AmiSSL. The TLS branch of http_client.c
# is gated behind __AMIGA__ but the ssl_* / socket_* code isn't reachable
# because the tests use http_connect_io() with a memory-backed vtable.
#
# Usage: make          (build and run)
#        make build    (build only)
#        make run      (run only -- assumes already built)
#        make clean

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

# -m68020 so vamos runs the same CPU flavor amigit targets. -O0 keeps
# bebbo-gcc's codegen honest (known-pitfalls: default to -O0 until
# proven safe). We pass -std=gnu99 so http_client.c's mixed
# declarations compile (matches amigit's CFLAGS).
CFLAGS = -O0 -noixemul -m68020 -std=gnu99 -Wall \
         -I../shim \
         -I../../ports/amigit/ported \
         -I../../lib/posix-shim/include \
         -I../../lib/bsdsocket-shim/include \
         -I../../lib/amissl-sdk/include

# Link against libamiport + libamiport-net + libamisslstubs + libm.
# The memory-backed parser test path never calls these at runtime,
# but http_client.c and amissl_glue.c reference amiport_socket etc.
# and SSL_* symbols at link time, so we need to satisfy the linker.
# This mirrors exactly what the real amigit binary links against
# (minus libgit2).
LDFLAGS = -L../../lib/posix-shim -lamiport \
          -L../../lib/bsdsocket-shim -lamiport-net \
          -L../../lib/amissl-sdk/lib -lamisslstubs \
          -lm

# vamos: -m68020 binary, 512 KB stack (small tests, but headroom for
# any future scenario tests that grow the buffer set), -C 68020 for
# the CPU (known-pitfalls: vamos Needs -C 68020), 4 MB memory.
VAMOS_STACK = 512
VAMOS_MEM   = 4096
VAMOS = $(TOOLCHAIN_BIN)/vamos -C 68020 -s $(VAMOS_STACK)

SHIM_LIB = ../../lib/posix-shim/libamiport.a

HTTP_SRCS = ../../ports/amigit/ported/http_client.c \
            ../../ports/amigit/ported/amissl_glue.c

HEADERS = test_http_client.c \
          ../shim/test_framework.h \
          ../../ports/amigit/ported/http_client.h

.PHONY: all build run clean

all: build run

build: test_http_client

$(SHIM_LIB):
	$(MAKE) -C ../../lib/posix-shim

test_http_client: test_http_client.c $(HTTP_SRCS) $(HEADERS) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $@ test_http_client.c $(HTTP_SRCS) $(LDFLAGS)

run: test_http_client
	VAMOS_MEM=$(VAMOS_MEM) $(VAMOS) ./test_http_client

clean:
	rm -f test_http_client
