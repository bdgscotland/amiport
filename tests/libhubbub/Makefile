# tests/libhubbub/Makefile -- unit tests for lib/libhubbub
#
# Tests run on vamos -C 68040 because libhubbub is built -m68040 -m68881
# (matches NetSurf-Vampire dep stack convention).
#
# Links against both libhubbub.a and libparserutils.a (libhubbub depends
# on libparserutils for inputstream + charset codecs).

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libhubbub/include
CFLAGS += -I../../lib/libparserutils/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libhubbub/libhubbub.a \
          ../../lib/libparserutils/libparserutils.a

TARGET       = test_libhubbub
# Larger resources than libwapcaplet/libparserutils tests because libhubbub's
# treebuilder + tokeniser combined consume ~80-120 KB worst-case stack and
# the 50-iteration parser_lifecycle stress test in main() repeatedly creates
# hubbub_parser instances each holding inputstream + tokeniser + treebuilder
# + a fresh charset codec, fragmenting the heap.
VAMOS_STACK  = 4096
VAMOS_CPU    = 68040
VAMOS_MEM    = 8192

all: $(TARGET)

$(TARGET): test_libhubbub.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libhubbub.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
