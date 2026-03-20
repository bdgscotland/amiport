# ADR-008: Tiered POSIX Compatibility Strategy

## Status

Accepted

## Date

2026-03-20

## Context

ADR-001 established that a POSIX shim library is the right approach for mapping POSIX functions to AmigaOS equivalents. The shim works well for functions with direct AmigaDOS counterparts (`open()` → `Open()`, `stat()` → `Lock()`+`Examine()`), but as amiport targets more complex software the question becomes: what happens when the semantic distance between POSIX and AmigaOS grows?

Three categories of POSIX functionality have emerged:

1. Functions with near-direct AmigaDOS equivalents (file I/O, directories, basic time)
2. Functions with no direct equivalent but emulatable behaviour (select/poll, mmap for read-only, basic signal handling beyond SIGINT)
3. Functions representing fundamental execution model differences (fork/exec, pthreads, shared memory, BSD sockets, Unix permissions)

Treating all three the same — either extending the shim indefinitely or refusing everything beyond Tier 1 — leads to bad outcomes. An unbounded shim becomes ixemul.library: a heavyweight emulation layer that produces non-native Amiga programs. Refusing everything beyond simple wrappers limits amiport to trivial CLI tools.

## Decision

Adopt a three-tier model for POSIX compatibility. Each tier has a distinct implementation strategy, a distinct library location, distinct agent behaviour in the pipeline, and a distinct contract with the developer.

### Tier 1 — Shim (direct mapping)

**Location:** `lib/posix-shim/`
**Contract:** Behaviour matches POSIX semantics. One wrapper per function. Mechanical transformation — header swap + function rename.
**Agent behaviour:** Automated. The transform-source skill applies these without human input.

### Tier 2 — Emulation (approximate mapping)

**Location:** `lib/posix-emu/`
**Contract:** Behaviour approximates POSIX semantics with documented differences. May carry performance or memory cost. May have subtle behavioural divergence under edge cases.
**Agent behaviour:** Semi-automated. The analysis skill flags these as `needs-emu` (yellow). The transform skill applies the mapping but adds a `/* amiport-emu: ... */` comment documenting the behavioural difference. The port-coordinator presents the tradeoff to the user.

### Tier 3 — Redesign (paradigm mismatch)

**Location:** No library. Pattern templates in `.claude/skills/transform-source/references/redesign-patterns.md`.
**Contract:** Original POSIX code must be structurally rewritten. The shim/emu libraries cannot help. Automated agents propose patterns but a human reviews the choice.
**Agent behaviour:** The analysis skill flags these as `needs-redesign` (red). The transform skill does NOT silently stub these. The port-coordinator presents the available redesign patterns with tradeoffs and waits for a decision before proceeding.

### Classification rule

A POSIX function belongs in the lowest-numbered tier where its *primary use cases* can be served. If only niche edge cases fail, it belongs in the lower tier with documented limitations. If the common case diverges, it belongs in the higher tier.

## Consequences

### Positive

- Clear vocabulary for agents and humans when discussing portability issues
- The shim stays thin and trustworthy — no emulation code masquerading as a direct wrapper
- Emulation is opt-in and explicitly labelled in ported source
- Redesign patterns accumulate as reusable knowledge rather than one-off hacks
- Analysis reports become more actionable: green/yellow/red rather than binary portable/blocking

### Negative

- Three library/pattern locations to maintain instead of one
- Agent skills need updating to understand the tier classification
- Some functions sit ambiguously between tiers and require judgement calls

### Neutral

- The existing shim (`lib/posix-shim/`) is entirely Tier 1 already — no migration needed
- Existing `blocking` classification in analysis reports maps to Tier 3; `needs-shim` maps to Tier 1
- A new `needs-emu` classification is introduced for Tier 2
