# API Mapping Reference

See the comprehensive mapping table in:
[.claude/skills/analyze-source/references/posix-to-amiga-map.md](../.claude/skills/analyze-source/references/posix-to-amiga-map.md)

This file serves as a pointer — the canonical mapping lives with the analyze-source skill
so that Claude agents have direct access during analysis.

## Specialized Mappings

For category-specific API mappings, see:
- [docs/references/console-ansi-mapping.md](references/console-ansi-mapping.md) — ncurses-to-ANSI escape mapping for console-shim (Category 3)
- [docs/references/bsdsocket-mapping.md](references/bsdsocket-mapping.md) — POSIX socket-to-bsdsocket.library mapping (Category 4)
- [docs/references/bsd-isms.md](references/bsd-isms.md) — BSD-specific functions and their shim status
- [docs/references/newlib-availability.md](references/newlib-availability.md) — C library functions available in -noixemul runtime
