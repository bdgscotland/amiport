# tests/libdom/Makefile -- unit tests for lib/libdom
#
# Tests run on vamos -C 68040 because libdom is built -m68040 -m68881
# (matches NetSurf-Vampire dep stack convention).
#
# Links against libdom.a + libhubbub.a + libparserutils.a + libwapcaplet.a
# in dependency-DAG order:
#   libdom -> libhubbub (via the bindings/hubbub binding)
#   libdom -> libwapcaplet (via dom_string interning)
#   libhubbub -> libparserutils (charset codecs / inputstream)

TOOLCHAIN_BIN = ../../toolchain/scripts
CC            = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VAMOS         = vamos

CFLAGS  = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall -Wextra
CFLAGS += -I../../lib/libdom/include
CFLAGS += -I../../lib/libhubbub/include
CFLAGS += -I../../lib/libparserutils/include
CFLAGS += -I../../lib/libwapcaplet/include
CFLAGS += -I../shim

LDFLAGS = -noixemul -m68040 -m68881
LIBS    = ../../lib/libdom/libdom.a \
          ../../lib/libhubbub/libhubbub.a \
          ../../lib/libparserutils/libparserutils.a \
          ../../lib/libwapcaplet/libwapcaplet.a

TARGET = test_libdom
# Resource sizing: this binary links the FULL NetSurf-Vampire dep stack
# (libdom + libhubbub + libparserutils + libwapcaplet) -- larger than any
# of the individual lib tests. Per
# ~/.claude/projects/-Users-duncan-Developer-amiport/memory/feedback_libnix_stack_scales_with_binary.md,
# stack and __MEMORY_STEP must be at least 524288 (set in test source).
# vamos resource budget: 4 MB stack budget, 8 MB total memory.
VAMOS_STACK = 4096
VAMOS_CPU   = 68040
VAMOS_MEM   = 8192

all: $(TARGET)

$(TARGET): test_libdom.c $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ test_libdom.c $(LIBS)

run: $(TARGET)
	$(VAMOS) -C $(VAMOS_CPU) -s $(VAMOS_STACK) -m $(VAMOS_MEM) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
