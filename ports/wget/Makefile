# Makefile for wget (amiport)
TARGET      = wget
include ../common.mk
VERSION     = 1.20.3
REVISION    = 1
DESCRIPTION = HTTP/HTTPS download utility
AUTHOR      = Free Software Foundation
CATEGORY    = 4

# Override for network port: 68020 (AmiSSL requires it), gnu99 (wget needs it)
CFLAGS = -std=gnu99 -O2 -noixemul -m68020 -Wall \
         -I../../lib/posix-shim/include \
         -I../../lib/bsdsocket-shim/include \
         -Iported -Iported/src -Iported/lib \
         -DHAVE_CONFIG_H \
         -DSYSTEM_WGETRC=\"S:wgetrc\" \
         -DAMIPORT_NET_MACROS \
         -Wno-unused-function -Wno-unused-variable -Wno-sign-compare

LDFLAGS = -L../../lib/posix-shim -lamiport \
          -L../../lib/bsdsocket-shim -lamiport-net \
          -lm

AMINET_CAT = comm/tcp

# vamos needs 68020 CPU for this binary
VAMOS_CPU = 68020
# vamos -s is in KiB -- 512 KiB stack for wget (network + recursion)
VAMOS_STACK = 512
VAMOS_MEM = 4096

# === Source files ===

# Core wget source (Phase 1: no openssl.c, no http-ntlm.c)
SRC_SOURCES = \
	ported/src/build_info.c \
	ported/src/connect.c \
	ported/src/convert.c \
	ported/src/cookies.c \
	ported/src/css_.c \
	ported/src/css-url.c \
	ported/src/exits.c \
	ported/src/ftp.c \
	ported/src/ftp-basic.c \
	ported/src/ftp-ls.c \
	ported/src/ftp-opie.c \
	ported/src/hash.c \
	ported/src/host.c \
	ported/src/hsts.c \
	ported/src/html-parse.c \
	ported/src/html-url.c \
	ported/src/http.c \
	ported/src/init.c \
	ported/src/log.c \
	ported/src/main.c \
	ported/src/netrc.c \
	ported/src/progress.c \
	ported/src/ptimer.c \
	ported/src/recur.c \
	ported/src/res.c \
	ported/src/retr.c \
	ported/src/spider.c \
	ported/src/url.c \
	ported/src/utils.c \
	ported/src/version.c \
	ported/src/warc.c \
	ported/src/amiga_stubs.c

# Gnulib support library (subset needed by wget)
LIB_SOURCES = \
	ported/lib/af_alg.c \
	ported/lib/base32.c \
	ported/lib/basename-lgpl.c \
	ported/lib/basename.c \
	ported/lib/c-ctype.c \
	ported/lib/c-strcasecmp.c \
	ported/lib/c-strcasestr.c \
	ported/lib/c-strncasecmp.c \
	ported/lib/dirname-lgpl.c \
	ported/lib/exitfail.c \
	ported/lib/getprogname.c \
	ported/lib/malloca.c \
	ported/lib/md2.c \
	ported/lib/md4.c \
	ported/lib/md5.c \
	ported/lib/quotearg.c \
	ported/lib/sha1.c \
	ported/lib/sha256.c \
	ported/lib/sha512.c \
	ported/lib/strnlen1.c \
	ported/lib/stripslash.c \
	ported/lib/u64.c \
	ported/lib/xalloc-die.c \
	ported/lib/xmalloc.c \
	ported/lib/xsize.c \
	ported/lib/xstrndup.c \
	ported/lib/error.c \
	ported/lib/getdelim.c \
	ported/lib/getline.c \
	ported/lib/memrchr.c \
	ported/lib/strptime.c \
	ported/lib/timegm.c \
	ported/lib/mkostemp.c \
	ported/lib/localcharset.c \
	ported/lib/fseeko.c \
	ported/lib/ftello.c \
	ported/lib/regex.c \
	ported/lib/getpass.c \
	ported/lib/tmpdir.c

SOURCES = $(SRC_SOURCES) $(LIB_SOURCES)

# Per-file object compilation (80+ files)
SRC_OBJS = $(SRC_SOURCES:.c=.o)
LIB_OBJS = $(LIB_SOURCES:.c=.o)
OBJECTS = $(SRC_OBJS) $(LIB_OBJS)

SHIM_LIB = ../../lib/posix-shim/libamiport.a
NET_LIB = ../../lib/bsdsocket-shim/libamiport-net.a

# Override default build rule — use per-file compilation
$(TARGET): $(OBJECTS) $(SHIM_LIB) $(NET_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Compile src/ files
ported/src/%.o: ported/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# amiport: main.c at -O0 to rule out bebbo-gcc optimizer bugs (crash-patterns #16)
ported/src/main.o: ported/src/main.c
	$(CC) $(subst -O2,-O0,$(CFLAGS)) -c -o $@ $<

# Compile lib/ files
ported/lib/%.o: ported/lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean test package

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "wget is a network tool -- basic smoke test via vamos"
	VAMOS_MEM=$(VAMOS_MEM) ../../toolchain/scripts/vamos -C 68020 -s $(VAMOS_STACK) ./$(TARGET) --version
	VAMOS_MEM=$(VAMOS_MEM) ../../toolchain/scripts/vamos -C 68020 -s $(VAMOS_STACK) ./$(TARGET) --help
	@echo "=== $(TARGET) smoke tests complete ==="

clean:
	rm -f ported/src/*.o ported/lib/*.o $(TARGET) $(TARGET).map
