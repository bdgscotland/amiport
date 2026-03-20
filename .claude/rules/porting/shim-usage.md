---
paths:
  - "lib/posix-shim/**"
  - "lib/posix-emu/**"
  - "lib/console-shim/**"
  - "lib/bsdsocket-shim/**"
---

# Shim Library Rules

## Tier Model (ADR-008)

- **Tier 1** (`lib/posix-shim/`) — Direct POSIX-to-AmigaOS wrappers. `amiport_*` prefix. Automated transformation.
- **Tier 2** (`lib/posix-emu/`) — Approximate emulation with documented caveats. `amiport_emu_*` prefix.
- **Tier 3** — Requires redesign. Needs human review.

## Before Assuming a Wrapper Exists

Check the actual headers:
- Tier 1: `lib/posix-shim/include/amiport/`
- Tier 2: `lib/posix-emu/include/amiport-emu/`

## Adding New Functions

Use `/extend-shim <function-name>` — do not manually add shim functions.

1. Check `docs/references/newlib-availability.md` — is it already in libnix?
2. Check `docs/references/bsd-isms.md` — is it a BSD-ism with a known solution?
3. Classify per `docs/posix-tiers.md` decision tree
4. Implement + test + rebuild via the skill

## Linking

- Category 1 (CLI): `-lamiport`
- Category 3 (Console UI): `-lamiport-console -lamiport`
- Category 4 (Network): `-lamiport-net -lamiport`
