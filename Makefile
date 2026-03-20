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

.PHONY: setup-toolchain build-shim build test test-shim package clean fetch-ndk help doctor smoke-test compare list-ports build-ports install-emu setup-emu emu publish

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
	@echo "  list-ports       List all ports and their status"
	@echo "  build-ports      Build all ports"
	@echo "  setup-emu        Install FS-UAE and check for Kickstart ROM"
	@echo "  publish          Prepare and upload a port to Aminet (TARGET=ports/cal)"
	@echo "  install-emu      Copy built binaries to emulator directory"
	@echo "  emu              Launch FS-UAE with built binaries mounted as WORK:"
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
	$(MAKE) -C $(TARGET) TARGET=$(notdir $(TARGET))

test:
ifndef TARGET
	$(error TARGET is required. Usage: make test TARGET=examples/wc)
endif
	$(MAKE) -C $(TARGET) TARGET=$(notdir $(TARGET)) test

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

list-ports:
	@echo "=== amiport ports ==="
	@for dir in ports/*/; do \
		if [ -f "$$dir/Makefile" ]; then \
			name=$$(basename "$$dir"); \
			if [ -f "$$dir/$$name" ]; then status="BUILT"; \
			elif [ -f "$$dir/ported/$$name.c" ]; then status="READY"; \
			elif [ -f "$$dir/original/$$name.c" ]; then status="ORIGINAL ONLY"; \
			else status="EMPTY"; fi; \
			printf "  %-20s %s\n" "$$name" "$$status"; \
		fi; \
	done
	@echo ""

build-ports: build-shim
	@for dir in ports/*/; do \
		if [ -f "$$dir/Makefile" ]; then \
			echo "Building $$(basename $$dir)..."; \
			$(MAKE) -C "$$dir" TARGET=$$(basename "$$dir") || exit 1; \
		fi; \
	done

publish:
ifndef TARGET
	$(error TARGET is required. Usage: make publish TARGET=ports/cal)
endif
	@bash scripts/publish-aminet.sh $(TARGET)

setup-emu:
	@bash scripts/setup-emu.sh

install-emu:
	@bash scripts/install-emu.sh

emu: install-emu
	@if ! command -v fs-uae >/dev/null 2>&1; then \
		echo "FS-UAE not found. Install with: brew install fs-uae"; \
		echo "You also need a Kickstart 3.1 ROM in ~/Documents/FS-UAE/Kickstarts/"; \
		exit 1; \
	fi
	fs-uae toolchain/configs/amiport-test.fs-uae

clean:
	$(MAKE) -C lib/posix-shim clean
	$(MAKE) -C tests/shim clean
	@for dir in ports/*/; do \
		if [ -f "$$dir/Makefile" ]; then $(MAKE) -C "$$dir" TARGET=$$(basename "$$dir") clean 2>/dev/null; fi; \
	done
	@if [ -n "$(TARGET)" ]; then $(MAKE) -C $(TARGET) TARGET=$(notdir $(TARGET)) clean; fi
