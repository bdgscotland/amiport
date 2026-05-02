# tests/libparserutils/Makefile -- unit tests for lib/libparserutils
#
# Tests run on vamos -C 68040 because the library is built -m68040 -m68881
# (matches NetSurf-Vampire dep stack convention; see lib/libwapcaplet
# Makefile header for rationale).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libparserutils/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libparserutils/libparserutils.a

TARGET       = test_libparserutils
VAMOS_STACK  = 1024
VAMOS_CPU    = 68040
VAMOS_MEM    = 4096

all: $(TARGET)

$(TARGET): test_libparserutils.c ../../lib/libparserutils/libparserutils.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libparserutils.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
