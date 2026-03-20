# Console shim tests
#
# These test the non-interactive parts of the console shim.
# Interactive testing (screen output, keyboard input) requires FS-UAE.

CC = m68k-amigaos-gcc
CFLAGS = -O2 -Wall -Wextra -noixemul -m68020 \
         -I../../lib/console-shim/include \
         -I../common

LDFLAGS = -L../../lib/console-shim -lamiport-console -noixemul

TESTS = test_curses

.PHONY: all clean test

all: $(TESTS)

test_curses: test_curses.c ../common/test_framework.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: $(TESTS)
	@for t in $(TESTS); do \
		echo "--- Running $$t ---"; \
		vamos -C ../../ ./$$t || exit 1; \
	done

clean:
	rm -f $(TESTS) *.o
