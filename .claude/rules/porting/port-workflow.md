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

## Before Starting a Port

1. Run `aminet-researcher` agent to check for existing ports
2. Run `dependency-auditor` agent for complex ports
3. Check `docs/posix-tiers.md` for tier classification of needed functions

## After Completing a Port

1. Run `/review-amiga` for Amiga-specific code review
2. Update `PORTS.md` with the new port entry
3. Update `README.md` ports table
