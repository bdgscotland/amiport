# PDR-001: Target Audience and Initial Scope

## Status

Accepted

## Date

2026-03-19

## Problem

There's no automated tooling for porting modern open-source C software to the Commodore Amiga. Existing ports are manual, labor-intensive, and rarely documented. The Amiga retro community wants more software but lacks the developer hours.

## Target Users

- **Amiga enthusiasts** who want Linux/BSD utilities on their machines but lack C porting expertise
- **Retro developers** who want to accelerate their porting workflow
- **Claude Code users** interested in AI-assisted systems programming

## Decision

Build an AI-powered porting toolkit that targets simple C CLI utilities first, expanding to more complex software over time. The tool is opinionated: it chooses bebbo-gcc, AmigaOS 3.x, and a shim-based approach by default.

## Rationale

Starting with CLI utilities (wc, cat, head, grep) keeps the POSIX surface area small and testable. The shim approach scales — each successfully ported function makes the next port easier. AI automation makes the barrier to entry near zero for simple programs.

## Success Criteria

- A complete port of `wc` from source analysis to working Amiga binary, fully automated
- The pipeline produces correct output verified in emulation
- A new user can run `/port-project` on a simple C program and get a working binary

## Alternatives Considered

- **Manual porting guides only**: Lower effort but doesn't leverage Claude's capabilities
- **Target AmigaOS 4.x first**: Easier (better POSIX) but smaller community and less interesting technically
- **Target all Amiga OS versions equally**: Too broad for initial scope
