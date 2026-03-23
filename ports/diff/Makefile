# Makefile for diff (amiport — OpenBSD diff port)
include ../common.mk

TARGET      = diff
SOURCES     = ported/diff.c ported/diffreg.c ported/diffdir.c ported/xmalloc.c
VERSION     = 1.95
DESCRIPTION = OpenBSD diff file comparison tool
AUTHOR      = Todd C. Miller, OpenBSD contributors
AMINET_CAT  = util/cli

# amiport: diff needs ~70 KiB stack (FileInfoBlock + GetDeviceProc + probe buffers).
# vamos defaults to 8 KiB; the libnix __stack cookie is not read at runtime.
VAMOS_STACK = 256

# Profiling support: make profile-build && make profile-run
PROF_OBJ = ../../lib/posix-shim/src/profile.o
PROF_CFLAGS = -DAMIPORT_PROFILE

.PHONY: all clean test package compare profile-build profile-run

test: $(TARGET)
	@echo "=== Testing diff ==="
	@echo "line 1" > test_a.txt
	@echo "line 2" >> test_a.txt
	@echo "line 3" >> test_a.txt
	@echo "line 1" > test_b.txt
	@echo "line 2 modified" >> test_b.txt
	@echo "line 3" >> test_b.txt
	@echo "line 4 added" >> test_b.txt
	@echo "--- Normal diff ---"
	vamos -s $(VAMOS_STACK) -- ./$(TARGET) test_a.txt test_b.txt || true
	@echo "--- Unified diff ---"
	vamos -s $(VAMOS_STACK) -- ./$(TARGET) -u test_a.txt test_b.txt || true
	@echo "--- Context diff ---"
	vamos -s $(VAMOS_STACK) -- ./$(TARGET) -c test_a.txt test_b.txt || true
	@echo "--- Identical files ---"
	vamos -s $(VAMOS_STACK) -- ./$(TARGET) test_a.txt test_a.txt; echo "Exit: $$?"
	@echo "--- Nonexistent file ---"
	vamos -s $(VAMOS_STACK) -- ./$(TARGET) test_a.txt nonexistent.txt || true
	@rm -f test_a.txt test_b.txt
	@echo "=== diff tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/diff.c original/diffreg.c original/diffdir.c original/xmalloc.c
	@echo "line 1" > test_a.txt
	@echo "line 2" >> test_a.txt
	@echo "line 1" > test_b.txt
	@echo "line 2 changed" >> test_b.txt
	./$(TARGET)_native test_a.txt test_b.txt > /tmp/diff_expected.txt 2>&1 || true
	vamos -s $(VAMOS_STACK) -- ./$(TARGET) test_a.txt test_b.txt > /tmp/diff_actual.txt 2>&1 || true
	@diff /tmp/diff_expected.txt /tmp/diff_actual.txt && echo "MATCH" || echo "MISMATCH"
	@rm -f $(TARGET)_native test_a.txt test_b.txt /tmp/diff_expected.txt /tmp/diff_actual.txt

profile-build:
	$(CC) $(CFLAGS) $(PROF_CFLAGS) -o $(TARGET) $(SOURCES) $(PROF_OBJ) $(LDFLAGS)

profile-run: profile-build
	@echo "=== Profiling diff ==="
	vamos -s $(VAMOS_STACK) -- ./$(TARGET) test-diff-a.txt test-diff-b.txt 2>profile-output.txt || true
	@echo "--- Profile output ---"
	@cat profile-output.txt
	@rm -f profile-output.txt

clean:
	rm -f $(TARGET) $(TARGET)_native test_a.txt test_b.txt profile-output.txt
