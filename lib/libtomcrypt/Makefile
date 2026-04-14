# libtomcrypt — Cryptographic primitives library for AmigaOS
#
# LibTomCrypt 1.18.2 — stripped build for Dropbear SSH.
# Only includes: AES-CTR, ChaCha20-Poly1305, SHA-1/256, RSA, ECDSA, DH,
# HMAC, Fortuna PRNG. Everything else disabled via LTC_NOTHING.
#
# Depends on: lib/libtommath (LibTomMath 1.3.0)
# Build: make -C lib/libtomcrypt

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# Start from nothing, enable only what SSH needs
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
    -DLTC_SHA384 \
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
    -DLTM_DESC \
    -DLTC_BASE64 \
    -DLTC_NO_FILE \
    -DLTC_CLEAN_STACK

CFLAGS = -O0 -Wall -Wextra -Wno-unused-parameter -noixemul -m68020 -std=gnu99 \
         -Isrc/headers \
         -I../libtommath/include \
         $(CRYPTO_DEFINES)

# All .c files under src/ — disabled algorithms compile to empty TUs
SRCS = $(shell find src -name "*.c" | sort)
OBJS = $(SRCS:.c=.o)
LIB = libtomcrypt.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

# Per-file -O1 for audited hot-path files (perf-optimizer 2026-04-14).
# All verified: scalar returns only, no struct-by-value >8 bytes.
HOTPATH_CFLAGS = $(subst -O0,-O1,$(CFLAGS)) -fno-strict-aliasing

src/ciphers/aes/aes.o: src/ciphers/aes/aes.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/stream/chacha/chacha_crypt.o: src/stream/chacha/chacha_crypt.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/hashes/sha2/sha256.o: src/hashes/sha2/sha256.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/mac/poly1305/poly1305.o: src/mac/poly1305/poly1305.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/modes/ctr/ctr_encrypt.o: src/modes/ctr/ctr_encrypt.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/prngs/fortuna.o: src/prngs/fortuna.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find src -name "*.o" -delete
	rm -f $(LIB)
