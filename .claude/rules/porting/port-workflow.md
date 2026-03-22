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

- **Never remove functionality** — stub with a clear message if no Amiga equivalent
- **Prefer shim/emu wrappers** over inline AmigaDOS rewrites
- **Document every transformation** with `/* amiport: ... */` comments
- **Preserve original source** in `original/` — never modify it
- **Use upstream version numbers** — VERSION in Makefile, $VER string, and .readme must use the upstream project's version (e.g., `1.68` for OpenBSD grep rev 1.68, `5.4.7` for Lua 5.4.7), not a generic `1.0`

## Before Starting a Port

1. Run `aminet-researcher` agent to check for existing ports
2. Run `dependency-auditor` agent for complex ports
3. Check `docs/posix-tiers.md` for tier classification of needed functions

## After Completing a Port

1. Run `/review-amiga` for Amiga-specific code review
2. Dispatch `memory-checker` agent (**mandatory** — AmigaOS has no memory protection or GC)
3. Dispatch `perf-optimizer` agent (optional — 68k performance tuning)
4. Dispatch `test-designer` agent to generate comprehensive `test-fsemu-cases.txt` (8+ tests for CLI, 10+ for scripting). Do NOT write test cases manually — the agent analyzes source for flags, exit codes, error paths.
5. Run `make test-fsemu TARGET=ports/<name>` — FS-UAE testing is **mandatory for ALL categories**, not just Category 3-4. Test input files must be pre-created (no piping — ARexx `ADDRESS COMMAND` doesn't support it).
6. Run `make check-test-coverage` to verify the test suite meets the coverage standard
7. Update `PORTS.md` with the new port entry
8. Update `README.md` ports table
