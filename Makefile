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

.PHONY: setup setup-toolchain setup-debug-tools build-shim build-emu build-console build-net build test test-shim test-emu test-console test-net test-ports package clean fetch-ndk help doctor smoke-test compare list-ports build-ports install-emu setup-emu emu publish check-aminet build-uaequit test-fsemu check-docs check-agents check-test-coverage check-fix-propagation check-port-metadata scrape-adcd

help:
	@echo "amiport — AI-powered Amiga porting toolkit"
	@echo ""
	@echo "Targets:"
	@echo "  setup              Configure git hooks (run after cloning)"
	@echo "  setup-toolchain    Set up cross-compilation toolchain (Docker)"
	@echo "  setup-debug-tools  Install Enforcer/Mungwall/SegTracker for debug mode"
	@echo "  build-shim       Cross-compile the POSIX shim library (Tier 1)"
	@echo "  build-emu        Cross-compile the POSIX emulation library (Tier 2)"
	@echo "  build-console    Cross-compile the console shim library (ncurses)"
	@echo "  build-net        Cross-compile the BSD socket shim library"
	@echo "  build            Build a port (TARGET=examples/wc)"
	@echo "  test             Test a build via vamos (TARGET=examples/wc)"
	@echo "  test-shim        Run POSIX shim library tests via vamos"
	@echo "  test-emu         Run POSIX emulation library tests via vamos"
	@echo "  test-console     Run console shim tests via vamos"
	@echo "  test-net         Run BSD socket shim tests via vamos"
	@echo "  package          Create LHA archive (TARGET=examples/wc)"
	@echo "  fetch-ndk        Download AmigaOS NDK 3.2 R4"
	@echo "  clean            Remove build artifacts"
	@echo "  doctor           Check prerequisites (Docker, vamos, etc.)"
	@echo "  smoke-test       Run full end-to-end validation"
	@echo "  compare          Compare native vs Amiga output (TARGET=examples/foo)"
	@echo "  list-ports       List all ports and their status"
	@echo "  build-ports      Build all ports"
	@echo "  test-ports       Test all ports via vamos"
	@echo "  build-uaequit    Build UAEQuit (emulator shutdown tool)"
	@echo "  test-fsemu       Run FS-UAE automated tests (TARGET=ports/grep)"
	@echo "  setup-emu        Install FS-UAE and check for Kickstart ROM"
	@echo "  publish          Prepare and upload a port to Aminet (TARGET=ports/cal)"
	@echo "  install-emu      Copy built binaries to emulator directory"
	@echo "  emu              Launch FS-UAE with built binaries mounted as WORK:"
	@echo "  check-docs       Validate agent references across all docs"
	@echo "  check-agents     Validate agent/skill frontmatter fields"
	@echo "  check-test-coverage  Validate FS-UAE test suite completeness"
	@echo "  check-fix-propagation  Scan ports for known crash patterns"
	@echo "  check-port-metadata  Validate port metadata consistency"
	@echo "  scrape-adcd      Scrape ADCD and generate reference docs"
	@echo ""
	@echo "Claude Code skills:"
	@echo "  /analyze-source <path>   Analyze source for portability"
	@echo "  /transform-source <path> Transform source for Amiga"
	@echo "  /build-amiga             Cross-compile"
	@echo "  /test-amiga              Test in emulator"
	@echo "  /port-project <path>     Full pipeline"

setup:
	git config core.hooksPath .githooks
	@echo "Git hooks configured (.githooks/)"
	@echo "Run 'make setup-toolchain' next to install the cross-compiler."

setup-toolchain:
	bash toolchain/scripts/setup-toolchain.sh

setup-debug-tools:
	bash toolchain/scripts/setup-debug-tools.sh

build-shim:
	$(MAKE) -C lib/posix-shim

build-emu:
	$(MAKE) -C lib/posix-emu

build-console:
	$(MAKE) -C lib/console-shim

build-net:
	$(MAKE) -C lib/bsdsocket-shim

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

test-emu:
	$(MAKE) -C tests/emu

test-console: build-console
	$(MAKE) -C tests/console

test-net: build-net
	$(MAKE) -C tests/net

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

check-aminet:
	@bash scripts/check-aminet.sh

build-ports: build-shim build-emu
	@for dir in ports/*/; do \
		if [ -f "$$dir/Makefile" ]; then \
			echo "Building $$(basename $$dir)..."; \
			$(MAKE) -C "$$dir" TARGET=$$(basename "$$dir") || exit 1; \
		fi; \
	done

test-ports: build-shim build-emu
	@for dir in ports/*/; do \
		if [ -f "$$dir/Makefile" ]; then \
			echo "Testing $$(basename $$dir)..."; \
			$(MAKE) test TARGET="$$dir" || exit 1; \
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

build-uaequit:
	$(MAKE) -C toolchain/uaequit

test-fsemu: build-uaequit install-emu
ifndef TARGET
	$(error TARGET is required. Usage: make test-fsemu TARGET=ports/grep)
endif
	@bash scripts/test-fsemu.sh $(TARGET)

emu: install-emu
	@if ! command -v fs-uae >/dev/null 2>&1; then \
		echo "FS-UAE not found. Install with: brew install fs-uae"; \
		echo "You also need a Kickstart 3.1 ROM in ~/Documents/FS-UAE/Kickstarts/"; \
		exit 1; \
	fi
	fs-uae toolchain/configs/amiport-test.fs-uae

scrape-adcd:
	python3 scripts/scrape-adcd.py all --output docs/references/adcd/

check-docs:
	@echo "=== Checking doc consistency ==="
	@AGENTS=$$(ls .claude/agents/*.md 2>/dev/null | xargs -I{} basename {} .md | sort); \
	FAIL=0; \
	for agent in $$AGENTS; do \
		for doc in CLAUDE.md README.md docs/architecture.md; do \
			if ! grep -q "$$agent" "$$doc" 2>/dev/null; then \
				echo "MISSING: '$$agent' not found in $$doc"; \
				FAIL=1; \
			fi; \
		done; \
	done; \
	if [ "$$FAIL" -eq 0 ]; then \
		echo "All $$(echo "$$AGENTS" | wc -w | tr -d ' ') agents referenced in all docs."; \
	else \
		echo ""; \
		echo "FAIL: Agent references missing. Update docs per .claude/rules/documentation.md"; \
		exit 1; \
	fi

check-agents:
	@echo "=== Checking agent/skill frontmatter ==="
	@FAIL=0; \
	for f in .claude/agents/*.md; do \
		name=$$(basename "$$f" .md); \
		if ! head -20 "$$f" | grep -q '^name:'; then \
			echo "MISSING: '$$name' agent has no 'name' field"; FAIL=1; \
		fi; \
		if ! head -20 "$$f" | grep -q '^description:'; then \
			echo "MISSING: '$$name' agent has no 'description' field"; FAIL=1; \
		fi; \
		if head -20 "$$f" | grep -q 'allowed_tools'; then \
			echo "ERROR: '$$name' agent uses 'allowed_tools' (underscore) — should be 'allowed-tools' (hyphen)"; FAIL=1; \
		fi; \
	done; \
	for d in .claude/skills/*/; do \
		skill=$$(basename "$$d"); \
		f="$$d/SKILL.md"; \
		if [ ! -f "$$f" ]; then continue; fi; \
		if head -20 "$$f" | grep -q 'allowed_tools'; then \
			echo "ERROR: '$$skill' skill uses 'allowed_tools' (underscore) — should be 'allowed-tools' (hyphen)"; FAIL=1; \
		fi; \
	done; \
	if [ "$$FAIL" -eq 0 ]; then \
		echo "All agents and skills have valid frontmatter."; \
	else \
		echo ""; \
		echo "FAIL: Agent/skill frontmatter issues found. Fix the errors above."; \
		exit 1; \
	fi

check-test-coverage:
	@bash scripts/check-test-coverage.sh

check-fix-propagation:
	@bash scripts/check-fix-propagation.sh

check-port-metadata:
	@bash scripts/check-port-metadata.sh

clean:
	$(MAKE) -C lib/posix-shim clean
	$(MAKE) -C lib/posix-emu clean
	-$(MAKE) -C lib/console-shim clean
	-$(MAKE) -C lib/bsdsocket-shim clean
	$(MAKE) -C tests/shim clean
	@for dir in ports/*/; do \
		if [ -f "$$dir/Makefile" ]; then $(MAKE) -C "$$dir" TARGET=$$(basename "$$dir") clean 2>/dev/null; fi; \
	done
	@if [ -n "$(TARGET)" ]; then $(MAKE) -C $(TARGET) TARGET=$(notdir $(TARGET)) clean; fi
