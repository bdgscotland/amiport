# amiport — Top-level Makefile
#
# Targets:
#   setup-toolchain  Set up the cross-compilation toolchain (Docker)
#   build-shim       Cross-compile the POSIX shim library
#   build            Build a specific port (TARGET=examples/wc)
#   test             Test a build via vamos (TARGET=examples/wc)
#   package          Create LHA archive (TARGET=examples/wc)
#   clean            Remove build artifacts
#   fetch-ndk        Download AmigaOS NDK 3.2 R4

.PHONY: setup-toolchain build-shim build test test-shim package clean fetch-ndk help doctor smoke-test compare

help:
	@echo "amiport — AI-powered Amiga porting toolkit"
	@echo ""
	@echo "Targets:"
	@echo "  setup-toolchain  Set up cross-compilation toolchain (Docker)"
	@echo "  build-shim       Cross-compile the POSIX shim library"
	@echo "  build            Build a port (TARGET=examples/wc)"
	@echo "  test             Test a build via vamos (TARGET=examples/wc)"
	@echo "  test-shim        Run POSIX shim library tests via vamos"
	@echo "  package          Create LHA archive (TARGET=examples/wc)"
	@echo "  fetch-ndk        Download AmigaOS NDK 3.2 R4"
	@echo "  clean            Remove build artifacts"
	@echo "  doctor           Check prerequisites (Docker, vamos, etc.)"
	@echo "  smoke-test       Run full end-to-end validation"
	@echo "  compare          Compare native vs Amiga output (TARGET=examples/foo)"
	@echo ""
	@echo "Claude Code skills:"
	@echo "  /analyze-source <path>   Analyze source for portability"
	@echo "  /transform-source <path> Transform source for Amiga"
	@echo "  /build-amiga             Cross-compile"
	@echo "  /test-amiga              Test in emulator"
	@echo "  /port-project <path>     Full pipeline"

setup-toolchain:
	bash toolchain/scripts/setup-toolchain.sh

build-shim:
	$(MAKE) -C lib/posix-shim

build:
ifndef TARGET
	$(error TARGET is required. Usage: make build TARGET=examples/wc)
endif
	$(MAKE) -C $(TARGET)

test:
ifndef TARGET
	$(error TARGET is required. Usage: make test TARGET=examples/wc)
endif
	$(MAKE) -C $(TARGET) test

test-shim:
	$(MAKE) -C tests/shim

package:
ifndef TARGET
	$(error TARGET is required. Usage: make package TARGET=examples/wc)
endif
	@echo "Packaging $(TARGET)..."
	@cd $(TARGET) && lha a $(notdir $(TARGET)).lha $(notdir $(TARGET)) README.md

fetch-ndk:
	@echo "Downloading NDK 3.2 R4..."
	@mkdir -p docs/references/ndk
	@curl -L -o /tmp/NDK3.2R4.lha \
		"https://www.hyperion-entertainment.com/index.php/downloads?view=details&file=126" \
		|| echo "Direct download failed. Try: https://aminet.net/package/dev/misc/NDK3.2"
	@echo "Extract with: lha x /tmp/NDK3.2R4.lha -w=docs/references/ndk/"

doctor:
	@bash scripts/doctor.sh

smoke-test: doctor setup-toolchain build-shim test-shim
	@echo ""
	@echo "=== Building examples ==="
	$(MAKE) build TARGET=examples/wc
	$(MAKE) build TARGET=examples/head
	$(MAKE) build TARGET=examples/mini-find
	@echo ""
	@echo "=== Testing examples ==="
	$(MAKE) test TARGET=examples/wc
	$(MAKE) test TARGET=examples/head
	$(MAKE) test TARGET=examples/mini-find
	@echo ""
	@echo "=== Shim Coverage ==="
	@bash scripts/shim-coverage.sh
	@echo ""
	@echo "========================================="
	@echo "  SMOKE TEST PASSED — all checks green"
	@echo "========================================="

compare:
	@test -n "$(TARGET)" || (echo "Usage: make compare TARGET=examples/head" && exit 1)
	@echo "=== Comparing native vs Amiga: $(TARGET) ==="
	$(MAKE) -C $(TARGET) compare

clean:
	$(MAKE) -C lib/posix-shim clean
	$(MAKE) -C tests/shim clean
	@if [ -n "$(TARGET)" ]; then $(MAKE) -C $(TARGET) clean; fi
