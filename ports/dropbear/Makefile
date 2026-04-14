# Dropbear SSH Client (dbclient) for AmigaOS 3.x
# PDR-013 Phase 2
#
# Requires: lib/libtomcrypt, lib/libtommath, lib/posix-shim, lib/bsdsocket-shim

VERSION     = 2025.89
REVISION    = 1
DESCRIPTION = SSH client for AmigaOS (68020+)

include ../common.mk

PROGRAM = dbclient

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

# Client-only: -DDROPBEAR_CLIENT, NOT -DDROPBEAR_SERVER
CFLAGS = -O0 -m68020 -noixemul -std=gnu99 \
         -Wall -Wno-unused-parameter -Wno-unused-function \
         -DDROPBEAR_CLIENT \
         -DAMIPORT_NET_MACROS \
         -DLOCALOPTIONS_H_EXISTS \
         -include ported/amigaos_compat.h \
         -Iported \
         -I../../lib/libtomcrypt/src/headers \
         -I../../lib/libtommath/include \
         -I../../lib/posix-shim/include \
         -I../../lib/bsdsocket-shim/include

# Common objects (shared between client and server)
COMMON_SRCS = \
    ported/dbutil.c \
    ported/buffer.c \
    ported/dbhelpers.c \
    ported/bignum.c \
    ported/signkey.c \
    ported/rsa.c \
    ported/dss.c \
    ported/dbrandom.c \
    ported/queue.c \
    ported/atomicio.c \
    ported/compat.c \
    ported/fake-rfc2553.c \
    ported/ltc_prng.c \
    ported/ecc.c \
    ported/ecdsa.c \
    ported/sk-ecdsa.c \
    ported/crypto_desc.c \
    ported/curve25519.c \
    ported/ed25519.c \
    ported/sk-ed25519.c \
    ported/dbmalloc.c \
    ported/gensignkey.c \
    ported/gendss.c \
    ported/genrsa.c \
    ported/gened25519.c

# Client/server shared objects
CLISVRCOMMON_SRCS = \
    ported/common-session.c \
    ported/packet.c \
    ported/common-algo.c \
    ported/common-kex.c \
    ported/common-channel.c \
    ported/common-chansession.c \
    ported/termcodes.c \
    ported/tcp-accept.c \
    ported/listener.c \
    ported/process-packet.c \
    ported/dh_groups.c \
    ported/common-runopts.c \
    ported/circbuffer.c \
    ported/list.c \
    ported/netio.c \
    ported/chachapoly.c \
    ported/gcm.c \
    ported/kex-x25519.c \
    ported/kex-dh.c \
    ported/kex-ecdh.c

# Client-specific objects
CLI_SRCS = \
    ported/cli-main.c \
    ported/cli-auth.c \
    ported/cli-authpasswd.c \
    ported/cli-kex.c \
    ported/cli-session.c \
    ported/cli-runopts.c \
    ported/cli-chansession.c \
    ported/cli-authpubkey.c \
    ported/cli-tcpfwd.c \
    ported/cli-channel.c \
    ported/cli-authinteract.c \
    ported/cli-agentfwd.c \
    ported/cli-readconf.c

# AmigaOS stubs and glue
AMIGA_SRCS = \
    ported/dropbear_stubs.c

ALL_SRCS = $(COMMON_SRCS) $(CLISVRCOMMON_SRCS) $(CLI_SRCS) $(AMIGA_SRCS)
OBJS = $(ALL_SRCS:.c=.o)

LDFLAGS = -noixemul -m68020
LIBS = -L../../lib/libtomcrypt -ltomcrypt \
       -L../../lib/libtommath -ltommath \
       -L../../lib/posix-shim -lamiport \
       -L../../lib/bsdsocket-shim -lamiport-net \
       -lm

VAMOS_STACK = 262144
VAMOS_CPU = 68020

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	@ls -la $@

ported/%.o: ported/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f ported/*.o $(PROGRAM)
