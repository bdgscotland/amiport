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

# Default version and revision
VERSION ?= 1.0
REVISION ?= 1

# Package suffix: version alone for rev 1, version-revision for rev 2+
ifeq ($(REVISION),1)
PKG_SUFFIX = $(VERSION)
else
PKG_SUFFIX = $(VERSION)-$(REVISION)
endif

# Aminet category (override per port if needed)
AMINET_CAT ?= util/cli

# Port category (1=CLI, 2=Scripting, 3=Console, 4=Network, 5=GUI)
CATEGORY ?= 1

.PHONY: all clean test package compare

all: $(TARGET)

$(TARGET): $(SOURCES) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Package for Aminet distribution (includes binary, readme, docs, and source)
# Requires a hand-crafted <name>.readme — see ports/templates/readme.template
# Uses Docker lha since macOS lhasa is extraction-only
#
# Staging layout under .pkg/:
#   .pkg/<name>          — stripped binary (stored as <name> in archive)
#   .pkg/<name>.readme   — Aminet readme
#   .pkg/PORT.md         — porting log
#   .pkg/TEST-REPORT.md  — FS-UAE test results (if present)
#   .pkg/original/       — upstream source
#   .pkg/ported/         — transformed source
# lha is run from inside .pkg/ so no path prefix appears in the archive.
LHA_CMD = docker run --rm -v $(shell cd ../.. && pwd):/work -w /work/ports/$(TARGET)/.pkg amigadev/crosstools:m68k-amigaos lha
STRIP = $(TOOLCHAIN_BIN)/m68k-amigaos-strip

package: $(TARGET)
	@test -s $(TARGET).readme || (echo "ERROR: $(TARGET).readme missing or empty — create from ports/templates/readme.template" && exit 1)
	@rm -rf .pkg && mkdir -p .pkg
	$(STRIP) --strip-debug -o .pkg/$(TARGET) $(TARGET)
	@cp $(TARGET).readme .pkg/
	@cp PORT.md .pkg/
	@if [ -f TEST-REPORT.md ]; then cp TEST-REPORT.md .pkg/; fi
	@cp -r original/ .pkg/original/
	@cp -r ported/ .pkg/ported/
	@if [ -f TEST-REPORT.md ]; then \
		$(LHA_CMD) a ../$(TARGET)-$(PKG_SUFFIX).lha $(TARGET) $(TARGET).readme PORT.md TEST-REPORT.md original/ ported/; \
	else \
		$(LHA_CMD) a ../$(TARGET)-$(PKG_SUFFIX).lha $(TARGET) $(TARGET).readme PORT.md original/ ported/; \
	fi
	@rm -rf .pkg
	@cp $(TARGET).readme $(TARGET)-$(PKG_SUFFIX).readme
	@echo "Package ready:"
	@echo "  $(TARGET)-$(PKG_SUFFIX).lha     — upload to ftp://main.aminet.net/new/"
	@echo "  $(TARGET)-$(PKG_SUFFIX).readme  — upload alongside the .lha"

clean:
	rm -f $(TARGET) $(TARGET)-*.lha $(TARGET).map
