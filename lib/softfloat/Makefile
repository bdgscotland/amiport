# libsoftfloat -- pure-integer IEEE 754 + software libm for AmigaOS 3.x
#
# Provides:
#   1. Soft-float helpers: __divsf3, __mulsf3, __divdf3, etc.
#      (override libnix versions that route through ROM mathieee*.library)
#   2. Software libm: sin, cos, sqrt, log, exp, atan, pow, etc.
#      (pure-integer fdlibm — no FPU, no mathieee* library)
#
# Source provenance: libSDL2-amigaos3 src/stdlib/SDL_os3*.c + src/libm/*.c
# Promoted into amiport during PDR-015 OpenTTD work for cross-port reuse.

include ../common.mk

# -m68020 ensures generated code can run on 68020+ with or without FPU.
# Critically, no -m68881 — these soft-float bodies must NOT use FPU
# instructions themselves (the whole point is to avoid the FPU path).
SF_CFLAGS = -m68020 -O1 -noixemul -fno-strict-aliasing -Wno-parentheses -Wno-uninitialized -Isrc/libm

OBJS = src/softfloat_single.o src/softfloat_double.o \
       src/libm/e_atan2.o src/libm/e_exp.o src/libm/e_fmod.o \
       src/libm/e_log.o src/libm/e_log10.o src/libm/e_pow.o \
       src/libm/e_rem_pio2.o src/libm/e_sqrt.o \
       src/libm/k_cos.o src/libm/k_rem_pio2.o src/libm/k_sin.o src/libm/k_tan.o \
       src/libm/s_atan.o src/libm/s_copysign.o src/libm/s_cos.o \
       src/libm/s_fabs.o src/libm/s_floor.o src/libm/s_scalbn.o \
       src/libm/s_sin.o src/libm/s_tan.o \
       src/libm/float_wrappers.o

.PHONY: all clean

all: libsoftfloat.a

libsoftfloat.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(SF_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libsoftfloat.a
