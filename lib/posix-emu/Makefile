# Makefile for amiport POSIX emulation library (Tier 2)
#
# Cross-compiles using m68k-amigaos-gcc (bebbo toolchain)
# Produces libamiport-emu.a
#
# Unlike the Tier 1 shim (libamiport.a), this library provides
# approximate POSIX emulation with documented behavioural differences.
# See docs/posix-tiers.md and ADR-008 for the design rationale.

# Use wrapper scripts that auto-detect Docker vs native toolchain
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# Include both our own headers and the Tier 1 shim headers (for internal.h)
CFLAGS = -O2 -noixemul -m68000 -Wall -Iinclude -I../posix-shim/include
TARGET = libamiport-emu.a

SRCDIR = src
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
