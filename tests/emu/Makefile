# Makefile for amiport POSIX emulation (Tier 2) tests
#
# Cross-compiles test binaries and runs them via vamos.
# Usage: make          (build and run all tests)
#        make build    (build only)
#        make run      (run only)
#        make clean    (remove binaries)

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
CFLAGS = -O2 -noixemul -m68000 -Wall \
         -I../../lib/posix-shim/include \
         -I../../lib/posix-emu/include \
         -I.
LDFLAGS = -L../../lib/posix-emu -lamiport-emu \
          -L../../lib/posix-shim -lamiport
VAMOS = vamos -q

SHIM_DIR = ../../lib/posix-shim
EMU_DIR = ../../lib/posix-emu

TESTS = test_regex \
        test_mmap \
        test_pipe \
        test_select

.PHONY: all build run clean

all: build run

build: $(SHIM_DIR)/libamiport.a $(EMU_DIR)/libamiport-emu.a $(TESTS)

$(SHIM_DIR)/libamiport.a:
	$(MAKE) -C $(SHIM_DIR)

$(EMU_DIR)/libamiport-emu.a:
	$(MAKE) -C $(EMU_DIR)

%: %.c test_framework.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: $(TESTS)
	@FAILED=0; \
	for test in $(TESTS); do \
		echo ""; \
		$(VAMOS) ./$$test; \
		if [ $$? -ne 0 ]; then FAILED=1; fi; \
	done; \
	echo ""; \
	if [ $$FAILED -ne 0 ]; then \
		echo "*** SOME TESTS FAILED ***"; \
		exit 1; \
	else \
		echo "All test suites passed."; \
	fi

clean:
	rm -f $(TESTS)
