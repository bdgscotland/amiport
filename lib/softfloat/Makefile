# libsoftfloat -- pure-integer IEEE 754 for AmigaOS 3.x / libnix on FS-UAE
#
# See include/softfloat.h for full rationale. Short version: libnix's
# __divsf3 etc. call ROM mathieeesingbas.library which crashes on FS-UAE.
# This lib provides software-only replacements that override libnix.

include ../common.mk

# Force -m68020 (no -m68881) so the soft-float bodies themselves don't
# emit FPU instructions. The whole point is to avoid FPU.
SF_CFLAGS = -m68020 -O1 -noixemul -fno-strict-aliasing -Wall

OBJS = src/softfloat_single.o src/softfloat_double.o

.PHONY: all clean

all: libsoftfloat.a

libsoftfloat.a: $(OBJS)
	$(AR) rcs $@ $^

src/%.o: src/%.c
	$(CC) $(SF_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libsoftfloat.a
