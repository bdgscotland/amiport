# libtommath — Big integer arithmetic library for AmigaOS
#
# LibTomMath 1.3.0 — pure C, zero OS dependencies.
# Used by LibTomCrypt and Dropbear SSH for cryptographic big-integer ops.
#
# Build: make -C lib/libtommath

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

# MP_31BIT: 10% fewer digits than MP_28BIT, better 68020 MULS.L utilization
# MP_USE_MEMSET: longword-fill zeroing instead of byte-by-byte loop
# MP_LOW_MEM: caps window table to 32 entries (stack safety, crash-patterns #10)
# Consumers MUST set __stack >= 16384 (s_mp_montgomery_reduce_fast has 4KB W[] on stack)
CFLAGS = -O0 -Wall -Wextra -noixemul -m68020 -std=gnu99 \
         -Iinclude \
         -DMP_NO_FILE \
         -DMP_LOW_MEM \
         -DMP_FIXED_CUTOFFS \
         -DMP_NO_DEV_URANDOM \
         -DMP_31BIT \
         -DMP_USE_MEMSET

SRCS = $(wildcard src/bn_*.c)
OBJS = $(SRCS:.c=.o)
LIB = libtommath.a

# Hot math files promoted to -O1 after source-analyzer audit.
# These are pure integer, no struct-by-value returns, safe for -O1.
HOTPATH_SRCS = src/bn_s_mp_mul_digs.c \
               src/bn_s_mp_mul_digs_fast.c \
               src/bn_s_mp_sqr.c \
               src/bn_s_mp_sqr_fast.c \
               src/bn_mp_exptmod.c \
               src/bn_s_mp_exptmod.c \
               src/bn_s_mp_exptmod_fast.c \
               src/bn_mp_montgomery_reduce.c \
               src/bn_s_mp_montgomery_reduce_fast.c

# -O2 audited safe: all hot-path functions return mp_err (int), no struct-by-value > 8 bytes
HOTPATH_CFLAGS = -O2 -fno-strict-aliasing -noixemul -m68020 -std=gnu99 \
                 -Iinclude \
                 -DMP_NO_FILE \
                 -DMP_LOW_MEM \
                 -DMP_FIXED_CUTOFFS \
                 -DMP_NO_DEV_URANDOM \
                 -DMP_31BIT \
                 -DMP_USE_MEMSET

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

# Hot-path files get -O1
src/bn_s_mp_mul_digs.o: src/bn_s_mp_mul_digs.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_s_mp_mul_digs_fast.o: src/bn_s_mp_mul_digs_fast.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_s_mp_sqr.o: src/bn_s_mp_sqr.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_s_mp_sqr_fast.o: src/bn_s_mp_sqr_fast.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_mp_exptmod.o: src/bn_mp_exptmod.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_s_mp_exptmod.o: src/bn_s_mp_exptmod.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_s_mp_exptmod_fast.o: src/bn_s_mp_exptmod_fast.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_mp_montgomery_reduce.o: src/bn_mp_montgomery_reduce.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

src/bn_s_mp_montgomery_reduce_fast.o: src/bn_s_mp_montgomery_reduce_fast.c
	$(CC) $(HOTPATH_CFLAGS) -c $< -o $@

# Default rule for non-hotpath files
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)
