# Makefile for amiget — Package manager for AmigaOS 3.x
#
# This is original code, not a port. Category 4 (Network).
# Links against http-shim, bsdsocket-shim, and posix-shim.

TARGET      = amiget
VERSION     = 1.0
DESCRIPTION = Package manager for AmigaOS 3.x
AUTHOR      = Duncan Bowring
CATEGORY    = 4
AMINET_CAT  = util/misc

include ../common.mk

SOURCES = ported/amiget.c ported/json.c ported/sha256.c ported/db.c ported/config.c

# Override default CFLAGS to add network includes and use 68020
CFLAGS = -O2 -noixemul -m68020 -Wall \
         -I../../lib/posix-shim/include \
         -I../../lib/bsdsocket-shim/include \
         -I../../lib/http-shim/include

# Override LDFLAGS for network libraries (order matters: http -> net -> posix)
LDFLAGS = -L../../lib/http-shim -L../../lib/bsdsocket-shim -L../../lib/posix-shim \
          -lhttp-shim -lamiport-net -lamiport

# Library dependencies for rebuild tracking
HTTP_LIB = ../../lib/http-shim/libhttp-shim.a
NET_LIB  = ../../lib/bsdsocket-shim/libamiport-net.a

VAMOS_STACK = 256
VAMOS_CPU = 68020

$(TARGET): $(SOURCES) $(HTTP_LIB) $(NET_LIB) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

test: $(TARGET)
	@echo "=== amiget smoke tests (vamos) ==="
	VAMOS_MEM=4096 ../../toolchain/scripts/vamos -C $(VAMOS_CPU) -s $(VAMOS_STACK) -- ./$(TARGET)
	VAMOS_MEM=4096 ../../toolchain/scripts/vamos -C $(VAMOS_CPU) -s $(VAMOS_STACK) -- ./$(TARGET) help
	@echo "PASS: help output"
