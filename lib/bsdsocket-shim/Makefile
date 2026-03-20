# bsdsocket-shim — BSD socket API for AmigaOS via bsdsocket.library
#
# Builds libamiport-net.a for linking with ported network programs.

CC = m68k-amigaos-gcc
AR = m68k-amigaos-ar
RANLIB = m68k-amigaos-ranlib

CFLAGS = -O2 -Wall -Wextra -noixemul -m68020 \
         -Iinclude

SRCS = src/socket.c \
       src/sockfd.c \
       src/resolve.c \
       src/select_net.c \
       src/inet.c

OBJS = $(SRCS:.c=.o)
LIB = libamiport-net.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)
