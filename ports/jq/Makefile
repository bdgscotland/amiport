# Makefile for jq (amiport)
TARGET      = jq

include ../common.mk
VERSION     = 1.7.1
REVISION    = 1
DESCRIPTION = Lightweight command-line JSON processor
AUTHOR      = Stephen Dolan / jqlang
AMINET_CAT  = util/cli
CATEGORY    = 1

# jq requires C99 (60+ for-init declarations) — see ADR-022
CFLAGS += -std=gnu99

# CRITICAL: jq MUST be compiled with -O0 on bebbo-gcc 6.5.0 for 68k.
# GCC -O1/-O2 generates incorrect code for jq's 16-byte jv struct returns,
# corrupting kind_flags (first byte reads as 0 instead of correct value).
# This is a GCC codegen bug, not a source code issue. -O0 produces correct
# output for all jq operations. Binary is ~12% larger (378KB vs 338KB).
CFLAGS := $(subst -O2,-O0,$(CFLAGS))

# Stack: jq's recursive descent JSON parser and filter evaluator
# need significant stack depth for nested structures.
# VAMOS_STACK is in KB (vamos -s flag). 256 KB = 262144 bytes = __stack value.
# The FS-UAE test runner uses "Stack 262144" (bytes) separately.
VAMOS_STACK = 256
# RAM: jq's 377KB binary + builtin library compilation needs ~8MB.
# vamos default is 1MB which is insufficient to even load the binary.
VAMOS_MEM = 8192

# Internal include path for jq headers
CFLAGS += -I ported

# Config flags for AmigaOS (replaces autotools config.h)
# See ADR-022 for C99 rationale
JQ_DEFS = \
	-DPACKAGE_NAME=\"jq\" \
	-DPACKAGE_VERSION=\"1.7.1\" \
	-DPACKAGE=\"jq\" \
	-DVERSION=\"1.7.1\" \
	-DHAVE_STDIO_H=1 \
	-DHAVE_STDLIB_H=1 \
	-DHAVE_STRING_H=1 \
	-DHAVE_INTTYPES_H=1 \
	-DHAVE_STDINT_H=1 \
	-DHAVE_STRINGS_H=1 \
	-DHAVE_SYS_STAT_H=1 \
	-DHAVE_SYS_TYPES_H=1 \
	-DHAVE_UNISTD_H=1 \
	-DSTDC_HEADERS=1 \
	-DHAVE_ALLOCA_H=1 \
	-DHAVE_ALLOCA=1 \
	-DHAVE_ISATTY=1 \
	-DHAVE_SETENV=1 \
	-UHAVE_MEMMEM \
	-DHAVE_ATEXIT=1 \
	-DHAVE_STRFTIME=1 \
	-DHAVE_GMTIME=1 \
	-DHAVE_SETLOCALE=1 \
	-UHAVE_STRPTIME \
	-DHAVE_GMTIME_R=1 \
	-DHAVE_LOCALTIME_R=1 \
	-DHAVE_GETTIMEOFDAY=1 \
	-DIEEE_MC68k=1

# Math functions available in libnix -lm (C89 standard set)
JQ_MATH_DEFS = \
	-DHAVE_ACOS=1 -DHAVE_ASIN=1 -DHAVE_ATAN=1 -DHAVE_ATAN2=1 \
	-DHAVE_CEIL=1 -DHAVE_COS=1 -DHAVE_COSH=1 -DHAVE_EXP=1 \
	-DHAVE_FABS=1 -DHAVE_FLOOR=1 -DHAVE_FMOD=1 -DHAVE_FREXP=1 \
	-DHAVE_LDEXP=1 -DHAVE_LOG=1 -DHAVE_LOG10=1 -DHAVE_MODF=1 \
	-DHAVE_POW=1 -DHAVE_SIN=1 -DHAVE_SINH=1 -DHAVE_SQRT=1 \
	-DHAVE_TAN=1 -DHAVE_TANH=1 -DHAVE_HYPOT=1
# GNU math extensions NOT defined — jq stubs them at runtime with error messages

# Disable optional features
# - No oniguruma: regex builtins (test/match/capture) return runtime errors
# - No decnum: uses IEEE 754 doubles (standard JSON behavior)
# - No pthreads: single-threaded stubs in ported/amiga_pthread.h
# - No __thread TLS: not available on AmigaOS
JQ_DISABLE = \
	-UHAVE_LIBONIG \
	-UHAVE_PTHREAD \
	-UHAVE_PTHREAD_KEY_CREATE \
	-UHAVE_PTHREAD_ONCE \
	-UHAVE___THREAD \
	-UUSE_DECNUM \
	-UHAVE_TM_TM_GMT_OFF \
	-UHAVE_TM___TM_GMT_OFF \
	-UHAVE_TIMEGM \
	-UHAVE_DLFCN_H

CFLAGS += $(JQ_DEFS) $(JQ_MATH_DEFS) $(JQ_DISABLE)

# Additional libraries
LDFLAGS += -lm

# Source files (all in ported/)
SOURCES = \
	ported/main.c \
	ported/builtin.c \
	ported/bytecode.c \
	ported/compile.c \
	ported/execute.c \
	ported/jv.c \
	ported/jv_alloc.c \
	ported/jv_aux.c \
	ported/jv_dtoa.c \
	ported/jv_dtoa_tsd.c \
	ported/jv_file.c \
	ported/jv_parse.c \
	ported/jv_print.c \
	ported/jv_unicode.c \
	ported/lexer.c \
	ported/linker.c \
	ported/locfile.c \
	ported/parser.c \
	ported/util.c

# NOT included: jq_test.c (uses real pthreads), inject_errors.c (uses dlfcn)

.PHONY: test

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# vamos defaults to 8 KiB stack and 1 MB RAM; the libnix __stack cookie is not read at runtime.
	@# jq's recursive evaluator and builtin compilation need 256 KiB stack and ~8 MB RAM.
	@# VAMOS_MEM is passed as an env var to avoid conflicting with program flags (-m).
	echo '{"name":"amiga","year":1985}' | VAMOS_MEM=$(VAMOS_MEM) ../../toolchain/scripts/vamos -s $(VAMOS_STACK) -- ./$(TARGET) '.name'
	echo '[1,2,3]' | VAMOS_MEM=$(VAMOS_MEM) ../../toolchain/scripts/vamos -s $(VAMOS_STACK) -- ./$(TARGET) '.[] | . * 2'
	@echo "=== $(TARGET) tests complete ==="
