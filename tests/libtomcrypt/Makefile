# LibTomCrypt unit tests
#
# Run via: make -C tests/libtomcrypt
# Or: make test-libtomcrypt (from project root)

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CRYPTO_DEFINES = \
    -DLTC_NOTHING \
    -DLTC_RIJNDAEL \
    -DLTC_CTR_MODE \
    -DLTC_CHACHA \
    -DLTC_POLY1305 \
    -DLTC_CHACHA20POLY1305_MODE \
    -DLTC_SHA256 \
    -DLTC_SHA1 \
    -DLTC_SHA512 \
    -DLTC_HASH_HELPERS \
    -DLTC_HMAC \
    -DLTC_FORTUNA \
    -DLTC_CHACHA20_PRNG \
    -DLTC_SPRNG \
    -DLTC_RNG_GET_BYTES \
    -DLTC_MRSA \
    -DLTC_MECC \
    -DLTC_MDH \
    -DLTC_DH2048 \
    -DLTC_DH4096 \
    -DLTC_ECC_SHAMIR \
    -DLTC_DER \
    -DLTC_SOURCE \
    -DLTM_DESC

CFLAGS = -O0 -noixemul -m68020 -std=gnu99 -Wall \
         -I../../lib/libtomcrypt/src/headers \
         -I../../lib/libtommath/include \
         -I../shim \
         $(CRYPTO_DEFINES)

LDFLAGS = -L../../lib/libtomcrypt -ltomcrypt \
          -L../../lib/libtommath -ltommath \
          -lm -noixemul

VAMOS_STACK = 256
VAMOS_CPU = 68020
VAMOS = vamos -q -C $(VAMOS_CPU) -s $(VAMOS_STACK)

.PHONY: all build run clean

all: build run

build: test_libtomcrypt

test_libtomcrypt: test_libtomcrypt.c ../shim/test_framework.h
	$(CC) $(CFLAGS) -o $@ test_libtomcrypt.c $(LDFLAGS)

run: test_libtomcrypt
	$(VAMOS) ./test_libtomcrypt

clean:
	rm -f test_libtomcrypt *.o
