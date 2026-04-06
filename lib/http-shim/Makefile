# http-shim -- HTTP/1.0 client library for AmigaOS
#
# Builds libhttp-shim.a for linking with network programs.
# Depends on bsdsocket-shim (libamiport-net.a) and posix-shim (libamiport.a).

# Use wrapper scripts that auto-detect Docker vs native toolchain
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

CFLAGS = -O2 -Wall -Wextra -noixemul -m68000 \
         -Iinclude \
         -I../bsdsocket-shim/include \
         -I../posix-shim/include

SRCS = src/http.c
OBJS = $(SRCS:.c=.o)
LIB = libhttp-shim.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)
