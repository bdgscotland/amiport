# PDR-004: Mandatory Aminet Research Before Porting

## Status

Accepted

## Date

2026-03-20

## Problem

Without checking what already exists, we risk spending effort porting a tool that's already available on Aminet in a recent, functional version. The Amiga community has been porting Unix tools since the late 1980s — many common utilities already exist.

## Target Users

- The porting pipeline (avoids wasted effort)
- Amiga users (get genuinely new or upgraded tools, not duplicates)

## Decision

Add a mandatory Stage 0 to the `/port-project` pipeline: dispatch an `aminet-researcher` agent that checks Aminet, Geek Gadgets, AmigaPorts, and community forums before any porting work begins.

The agent produces a verdict:
- **SKIP** — A good, recent port exists. Don't duplicate.
- **UPGRADE** — An old or limited port exists. Ours adds value.
- **PORT** — Nothing exists. Go ahead.

If SKIP, the pipeline stops and tells the user. If UPGRADE, the existing port is noted in PORT.md for context.

## Rationale

Initial analysis for this session recommended porting `tree` — but research showed DirTree-cmd was updated in 2021 and is actively maintained. That would have been wasted effort. Similarly, `hexdump`/`xxd` was updated in 2025. Without the research step, we'd have duplicated working tools.

The research step also produces better PORT.md documentation — noting what the port replaces and why it's an improvement.

## Success Criteria

- Every `/port-project` invocation runs the research agent first
- No port is started for a tool that already has a recent, functional, open-source equivalent
- PORT.md documents prior art when upgrading an existing port

## Alternatives Considered

- **Manual check by user**: Easy to forget, inconsistent
- **Static list of known ports**: Goes stale quickly, incomplete
- **Skip research for "obvious" ports**: Risky — our `tree` example proved assumptions wrong
