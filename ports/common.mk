# ports/common.mk — Shared build settings for amiport ports
#
# Include from each port's Makefile:
#   include ../common.mk
#
# Each port must define:
#   TARGET  = program name (e.g. grep)
#   SOURCES = list of ported source files
#   VERSION = version string (e.g. 1.0)

# Cross-compiler (auto-detects Docker vs native)
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
CFLAGS = -O2 -noixemul -m68000 -Wall -I../../lib/posix-shim/include
LDFLAGS = -L../../lib/posix-shim -lamiport

SHIM_LIB = ../../lib/posix-shim/libamiport.a

# Native compiler for comparison builds
NATIVE_CC = cc
NATIVE_CFLAGS = -O2 -Wall

# Default version
VERSION ?= 1.0

# Aminet category (override per port if needed)
AMINET_CAT ?= util/cli

.PHONY: all clean test package compare

all: $(TARGET)

$(TARGET): $(SOURCES) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Package for Aminet distribution
package: $(TARGET) $(TARGET).readme
	lha a $(TARGET)-$(VERSION).lha $(TARGET) $(TARGET).readme PORT.md

# Generate Aminet readme from PORT.md metadata
$(TARGET).readme: PORT.md
	@echo "Short:    $(DESCRIPTION)" > $@
	@echo "Uploader: amiport (AI-assisted)" >> $@
	@echo "Author:   $(AUTHOR)" >> $@
	@echo "Type:     $(AMINET_CAT)" >> $@
	@echo "Architecture: m68k-amigaos >= 3.0" >> $@
	@echo "Version:  $(VERSION)" >> $@
	@echo "" >> $@
	@echo "Ported to AmigaOS by amiport (AI-assisted porting toolkit)." >> $@
	@echo "See PORT.md for transformation details." >> $@

clean:
	rm -f $(TARGET) $(TARGET).readme $(TARGET)-*.lha
