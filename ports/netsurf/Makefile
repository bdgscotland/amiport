# ports/netsurf/Makefile
# NetSurf 3.11 MUI fork -- Phase 1 (FreeType + AMMX text rendering)
#
# This is a wrapper Makefile that invokes the upstream NetSurf build system
# with proper cross-compilation environment settings for bebbo-gcc 13.3.

VERSION  = 3.11
REVISION = 1
DESCRIPTION = NetSurf web browser for Vampire 68080
include ../common.mk

# Vampire-native: hardware-required (no scalar fallback)
# Needs Apollo 68080 / V4 / A6000.
# Note: -m68040 -m68881 is the baseline for Phase D. Phase E will add AMMX .asm files.
TARGET_CFLAGS  = -m68040 -m68881 -O0 -noixemul -std=gnu99
TARGET_CXXFLAGS = -m68040 -m68881 -O0 -noixemul -std=gnu++17

# Docker image with vasm
DOCKER_IMAGE = ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest

# NetSurf upstream build uses TARGET=mui for the MUI frontend on AmigaOS 3.x
NETSURF_TARGET = mui

# Where the upstream build will place the binary
NETSURF_BINARY = original/build/Darwin-$(NETSURF_TARGET)/NetSurf

# Override the upstream toolchain paths to use our bebbo-gcc 13.3 in Docker.
# NetSurf's build system expects m68k-unknown-amigaos-gcc, but bebbo-gcc uses
# m68k-amigaos-gcc. We'll need to create a symlink wrapper or override CC directly.
#
# For now, we'll attempt a direct build using make -C original/ and see what breaks.

.PHONY: all clean package install-emu test

all: $(NETSURF_BINARY)

$(NETSURF_BINARY):
	@echo "=== Building NetSurf-MUI 3.11 (unchanged upstream) ==="
	@echo "TARGET=$(NETSURF_TARGET)"
	@echo "CFLAGS=$(TARGET_CFLAGS)"
	@echo ""
	@if [ -n "$$(cd original && git diff --name-only)" ]; then \
		echo "[INFO] Submodule already patched; skipping git apply"; \
	else \
		echo "[INFO] Applying ports/netsurf/patches/01-amiport-netsurf-mui-buildsystem.patch"; \
		(cd original && git apply ../patches/01-amiport-netsurf-mui-buildsystem.patch); \
	fi
	docker run --rm \
		-v $(shell cd ../.. && pwd):/work \
		-w /work/ports/netsurf/original \
		-e "CFLAGS=$(TARGET_CFLAGS) -I/work/lib/sdi-headers/include -I/work/lib/cybergraphx-sdk/include -DVOLATILE=volatile" \
		-e "CXXFLAGS=$(TARGET_CXXFLAGS) -I/work/lib/sdi-headers/include -I/work/lib/cybergraphx-sdk/include -DVOLATILE=volatile" \
		-e "CC=m68k-amigaos-gcc" \
		-e "CXX=m68k-amigaos-g++" \
		-e "AR=m68k-amigaos-ar" \
		-e "RANLIB=m68k-amigaos-ranlib" \
		-e "STRIP=m68k-amigaos-strip" \
		-e "GCCSDK_INSTALL_ENV=/opt/amiga13/m68k-amigaos" \
		-e "GCCSDK_INSTALL_CROSSBIN=/opt/amiga13/bin" \
		-e "PKG_CONFIG_PATH=/work/lib/pkgconfig" \
		$(DOCKER_IMAGE) \
		bash -c "make TARGET=$(NETSURF_TARGET) V=1"

clean:
	@echo "=== Cleaning NetSurf build ==="
	docker run --rm \
		-v $(shell cd ../.. && pwd):/work \
		-w /work/ports/netsurf/original \
		$(DOCKER_IMAGE) \
		bash -c "make TARGET=$(NETSURF_TARGET) clean" || true
	@rm -f netsurf

package: all
	@echo "TODO Phase H: package netsurf-$(DISPLAY_VERSION).lha"

install-emu: all
	@echo "TODO Phase D Step 5: copy netsurf binary to build/amiga/"
	@if [ -f "$(NETSURF_BINARY)" ]; then \
		echo "Binary exists at: $(NETSURF_BINARY)"; \
	else \
		echo "ERROR: Binary not found at expected location"; \
		exit 1; \
	fi

test: all
	@echo "TODO: vamos or FS-UAE testing"

.PHONY: all clean package install-emu test
