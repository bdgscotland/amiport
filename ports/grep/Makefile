# Makefile for grep (amiport)
include ../common.mk

TARGET      = grep
SOURCES     = ported/grep.c ported/util.c ported/file.c ported/queue.c ported/binary.c
VERSION     = 1.68
DESCRIPTION = OpenBSD grep (pattern search) for AmigaOS
AUTHOR      = James Howard / Dag-Erling Smorgrav

# Category 1 CLI tool — uses Tier 2 regex emulation and Tier 1 posix-shim
CFLAGS += -I../../lib/posix-emu/include
# Override common.mk LDFLAGS: posix-emu must come before posix-shim; -lamiport appears once only
LDFLAGS = -L../../lib/posix-emu -L../../lib/posix-shim -lamiport-emu -lamiport

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# Test 1: basic pattern match
	@printf "hello world\ngoodbye world\nhello again\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) "hello"
	@# Test 2: inverted match (-v)
	@printf "apple\nbanana\ncherry\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -v "banana"
	@# Test 3: case-insensitive (-i)
	@printf "Hello\nhello\nHELLO\nworld\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -i "hello"
	@# Test 4: count only (-c)
	@printf "aaa\nbbb\naaa\nccc\naaa\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -c "aaa"
	@# Test 5: line numbers (-n)
	@printf "foo\nbar\nfoo\nbaz\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -n "foo"
	@# Test 6: extended regex (-E)
	@printf "cat\ndog\ncar\ncap\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -E "ca[rt]"
	@# Test 7: fixed string (-F)
	@printf "a.b\na*b\na+b\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -F "a.b"
	@# Test 8: exit code 5 for no match (Amiga RETURN_WARN)
	@printf "no match here\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) "XYZZY123"; \
	  rc=$$?; if [ "$$rc" = "5" ]; then echo "PASS: exit code 5 (RETURN_WARN) for no match"; \
	  else echo "FAIL: expected exit 5, got $$rc"; fi
	@# Test 9: file argument (not stdin) — exercises grep_open/fopen path
	@printf "alpha\nbeta\ngamma\n" > test_input.txt && \
	  ../../toolchain/scripts/vamos -C "" ./$(TARGET) "beta" test_input.txt && \
	  rm -f test_input.txt
	@# Test 10: context lines (-B1 -A1)
	@printf "line1\nline2\nMATCH\nline4\nline5\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -B1 -A1 "MATCH"
	@# Test 11: multiple files show filename prefix
	@printf "found here\n" > test_a.txt && printf "not here\n" > test_b.txt && \
	  ../../toolchain/scripts/vamos -C "" ./$(TARGET) "found" test_a.txt test_b.txt && \
	  rm -f test_a.txt test_b.txt
	@# Test 12: word boundary (-w)
	@printf "cat\ncatch\nconcat\nscat\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -w "cat"
	@# Test 13: only-matching (-o)
	@printf "foobar\nbazfoo\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -o "foo"
	@# Test 14: max-count (-m)
	@printf "a\na\na\na\na\n" | ../../toolchain/scripts/vamos -C "" ./$(TARGET) -m 2 "a"
	@echo "=== $(TARGET) tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	@echo "(Native comparison not applicable for multi-file port)"
