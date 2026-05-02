# tests/libcss/Makefile -- unit tests for lib/libcss
#
# Tests run on vamos -C 68040 because libcss is built -m68040 -m68881
# (matches NetSurf-Vampire dep stack convention).
#
# Links against libcss.a + libwapcaplet.a + libparserutils.a in
# dependency-DAG order:
#   libcss -> libwapcaplet (interned property strings + values)
#   libcss -> libparserutils (lexer input stream + charset codecs)

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libcss/include
CFLAGS += -I../../lib/libwapcaplet/include
CFLAGS += -I../../lib/libparserutils/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libcss/libcss.a \
          ../../lib/libwapcaplet/libwapcaplet.a \
          ../../lib/libparserutils/libparserutils.a

TARGET = test_libcss
# Resource sizing: libcss + libwapcaplet + libparserutils. Slightly
# smaller than the libdom-class binary (no libhubbub + libdom). 1 MB
# cookies are conservative; could probably get away with 512 KB but
# stay consistent with the dep stack pattern (1 MB is the established
# floor for any test that links the dep stack libs together).
VAMOS_STACK = 4096
VAMOS_CPU   = 68040
VAMOS_MEM   = 8192

all: $(TARGET)

$(TARGET): test_libcss.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libcss.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
