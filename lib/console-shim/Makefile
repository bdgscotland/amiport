# console-shim — Minimal ncurses API for AmigaOS console.device
#
# Builds libamiport-console.a for linking with ported ncurses programs.

# Use wrapper scripts that auto-detect Docker vs native toolchain
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

CFLAGS = -O2 -Wall -Wextra -noixemul -m68000 \
         -Iinclude

SRCS = src/globals.c \
       src/initscr.c \
       src/output.c \
       src/input.c \
       src/color.c \
       src/attr.c \
       src/term.c

OBJS = $(SRCS:.c=.o)
LIB = libamiport-console.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)
