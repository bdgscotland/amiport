# BSD socket shim tests
#
# Tests the pure-C parts: inet conversion, fd tracking, constants.
# Network operations (actual socket calls) require FS-UAE + TCP/IP stack.

CC = m68k-amigaos-gcc
CFLAGS = -O2 -Wall -Wextra -noixemul -m68020 \
         -I../../lib/bsdsocket-shim/include \
         -I../common

LDFLAGS = -L../../lib/bsdsocket-shim -lamiport-net -noixemul

TESTS = test_inet

.PHONY: all clean test

all: $(TESTS)

test_inet: test_inet.c ../common/test_framework.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: $(TESTS)
	@for t in $(TESTS); do \
		echo "--- Running $$t ---"; \
		vamos -C ../../ ./$$t || exit 1; \
	done

clean:
	rm -f $(TESTS) *.o
