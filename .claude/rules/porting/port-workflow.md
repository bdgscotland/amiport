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
2. Dispatch `perf-optimizer` agent (mandatory — AmigaOS has no memory protection)
3. Run `make test-fsemu TARGET=ports/<name>` — FS-UAE testing is **mandatory for ALL categories**, not just Category 3-4. Create `test-fsemu-cases.txt` with test cases using pre-created input files (no piping — ARexx `ADDRESS COMMAND` doesn't support it)
4. Update `PORTS.md` with the new port entry
5. Update `README.md` ports table
