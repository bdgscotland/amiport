---
paths:
  - "ports/**"
---

# Port Workflow Rules

## Directory Structure

Every port lives in `ports/<toolname>/` with:
- `original/` — Unmodified source (reference copy)
- `ported/` — Transformed source for AmigaOS
- `Makefile` — Build rules
- `PORT.md` — Port documentation

## Porting Rules

- **Use port templates** — read `ports/templates/STRUCTURE.md` and copy `Makefile.template`, `PORT.md.template`, `readme.template` when setting up a new port. Fill in `__PLACEHOLDER__` variables.
- **Verify shim/emu availability** — check `lib/posix-shim/include/amiport/` for Tier 1 and `lib/posix-emu/include/amiport-emu/` for Tier 2 before assuming a wrapper exists
- **Follow the tier model** (ADR-008) — Tier 1 shim is automated, Tier 2 emu needs caveat documentation, Tier 3 redesign needs human review
- **Never remove functionality** — stub with a clear message if no Amiga equivalent
- **Prefer shim/emu wrappers** over inline AmigaDOS rewrites
- **Document every transformation** with `/* amiport: ... */` comments
- **Preserve original source** in `original/` — never modify it
- **Use upstream version numbers** — VERSION in Makefile, $VER string, and .readme must use the upstream project's version (e.g., `1.68` for OpenBSD grep rev 1.68, `5.4.7` for Lua 5.4.7), not a generic `1.0`
- Use `#ifdef __AMIGA__` blocks when code should remain cross-platform
- Target **AmigaOS 3.x on 68020+** as the default

## Before Starting a Port

1. Run `aminet-researcher` agent to check for existing ports
2. Run `dependency-auditor` agent for complex ports
3. Check `docs/posix-tiers.md` for tier classification of needed functions

## Critical: Never Mix amiport_open() with fdopen()

`amiport_open()` returns fds from amiport's internal table — a **separate namespace** from libnix fds. Calling `fdopen()` on an amiport fd creates a broken FILE* that silently fails. Use `fopen()` when you need a FILE*, `amiport_open()` only with `amiport_read()`/`amiport_write()`/`amiport_close()`. See crash-patterns.md #12.

## After Completing a Port

1. Run `/review-amiga` for Amiga-specific code review
2. Dispatch `memory-checker` agent (**mandatory** — AmigaOS has no memory protection or GC)
3. Dispatch `perf-optimizer` agent (**mandatory** — arguably the best agent, finds critical 68k wins like fgetc→fgets 3-5x speedup)
4. Dispatch `test-designer` agent to generate comprehensive `test-fsemu-cases.txt` (15+ tests for CLI, 20+ for scripting, 12+ for console/network). Do NOT write test cases manually — the agent analyzes source for flags, exit codes, error paths. Tests must include flag combinations, every error message path, edge cases (empty file, long line, no trailing newline), and crash-pattern regressions.
5. Run `make test-fsemu TARGET=ports/<name>` — FS-UAE testing is **mandatory for ALL categories**, not just Category 3-4. Test input files must be pre-created (no piping — ARexx `ADDRESS COMMAND` doesn't support it).
6. Run `make check-test-coverage` to verify the test suite meets the coverage standard
7. Update `PORTS.md` with the new port entry
8. Update `README.md` ports table
