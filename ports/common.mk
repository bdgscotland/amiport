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

# Package for Aminet distribution (includes binary, readme, docs, and source)
# Requires a hand-crafted <name>.readme — see ports/templates/readme.template
# Uses Docker lha since macOS lhasa is extraction-only
LHA_CMD = docker run --rm -v $(shell cd ../.. && pwd):/work -w /work/ports/$(TARGET) amigadev/crosstools:m68k-amigaos lha
STRIP = $(TOOLCHAIN_BIN)/m68k-amigaos-strip

package: $(TARGET)
	@test -s $(TARGET).readme || (echo "ERROR: $(TARGET).readme missing or empty — create from ports/templates/readme.template" && exit 1)
	@mkdir -p .pkg
	$(STRIP) --strip-debug -o .pkg/$(TARGET) $(TARGET)
	$(LHA_CMD) a $(TARGET)-$(VERSION).lha .pkg/$(TARGET) $(TARGET).readme PORT.md original/ ported/
	@rm -rf .pkg

clean:
	rm -f $(TARGET) $(TARGET)-*.lha $(TARGET).map
