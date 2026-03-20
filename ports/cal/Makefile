# Makefile for cal (amiport)
include ../common.mk

TARGET      = cal
SOURCES     = ported/cal.c
VERSION     = 1.0
DESCRIPTION = Unix-style calendar display (OpenBSD cal port)
AUTHOR      = Kim Letkeman, OpenBSD contributors

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing cal ==="
	@echo "--- Current month ---"
	vamos -- ./$(TARGET)
	@echo "--- March 2026 ---"
	vamos -- ./$(TARGET) 3 2026
	@echo "--- Full year 2026 ---"
	vamos -- ./$(TARGET) -y 2026
	@echo "--- Julian day numbers ---"
	vamos -- ./$(TARGET) -j 3 2026
	@echo "=== cal tests complete ==="

compare: $(TARGET)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(NATIVE_CC) $(NATIVE_CFLAGS) -o $(TARGET)_native original/cal.c
	@echo "--- Native: March 2026 ---"
	./$(TARGET)_native 3 2026 > /tmp/cal_expected.txt
	@echo "--- Amiga: March 2026 ---"
	vamos -- ./$(TARGET) 3 2026 > /tmp/cal_actual.txt
	@diff /tmp/cal_expected.txt /tmp/cal_actual.txt && echo "MATCH" || echo "MISMATCH"
	@rm -f $(TARGET)_native /tmp/cal_expected.txt /tmp/cal_actual.txt
