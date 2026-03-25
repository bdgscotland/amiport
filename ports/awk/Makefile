TARGET      = awk
SOURCES     = ported/main.c ported/run.c ported/lex.c ported/lib.c ported/b.c \
              ported/tran.c ported/parse.c ported/awkgram.tab.c ported/proctab.c
VERSION     = 2024.12.25
REVISION    = 1
DESCRIPTION = Pattern scanning and processing language
AUTHOR      = Brian W. Kernighan

include ../common.mk

# AWK requires C99 (per ADR-022) and math library
# amiport-emu provides popen/pclose/system via temp files
CFLAGS += -std=gnu99 -I../../lib/posix-emu/include
LDFLAGS += -L../../lib/posix-emu -lm -lamiport-emu

# AWK is a scripting interpreter -- needs more stack for recursive descent
# parsing, regex NFA construction, and deep awk function calls
VAMOS_STACK = 256

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
