# Console shim tests
#
# These test the non-interactive parts of the console shim.
# Interactive testing (screen output, keyboard input) requires FS-UAE.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
CFLAGS = -O2 -Wall -Wextra -noixemul -m68000 \
         -I../../lib/console-shim/include \
         -I../common

LDFLAGS = -L../../lib/console-shim -lamiport-console -noixemul

TESTS = test_curses test_termcap

.PHONY: all clean test test-termcap

all: $(TESTS)

test_curses: test_curses.c ../common/test_framework.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test_termcap: test_termcap.c ../common/test_framework.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Run only termcap tests (safe on vamos — no console init)
test-termcap: test_termcap
	@echo "--- Running test_termcap ---"
	vamos -s 65 -C ../../ ./test_termcap

test: test_termcap
	@echo "--- Running test_termcap ---"
	vamos -s 65 -C ../../ ./test_termcap
	@echo ""
	@echo "NOTE: test_curses requires FS-UAE (crashes on vamos)"

clean:
	rm -f $(TESTS) *.o
