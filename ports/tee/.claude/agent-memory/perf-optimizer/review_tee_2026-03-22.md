---
name: tee performance review 2026-03-22
description: Performance review of tee 1.15 (OpenBSD). Primary finding: 64KB buffer is wasteful on low-RAM machines; Ctrl-C check missing from hot loop.
type: project
---

Reviewed tee 1.15 at ports/tee/ported/tee.c on 2026-03-22.

PRIMARY FINDING: BSIZE = 64KB is too large for 68000/A500 targets. On a 512KB or even 2MB Chip RAM machine this eats a non-trivial fraction of available RAM and forces all buffer traffic through contended Chip RAM DMA bandwidth. Recommend 4096 or 8192 bytes.

SECONDARY FINDING: No amiport_check_break() call in the main read loop — user cannot Ctrl-C out of a hung tee (e.g., stalled pipe). This is a known-pitfalls issue, not purely perf, but was flagged.

CONFIRMED NON-ISSUES:
- SLIST traversal in hot path: minimal overhead, nodes are tiny and cache-warm after first iteration
- Partial-write retry loop: correct, never triggered on AmigaDOS (Write() is all-or-nothing)
- ssize_t for loop variables: 32-bit, fine on all 68k variants
- malloc(BSIZE): single allocation at startup, acceptable

**Why:** AmigaOS has no swap and no memory protection. On a base A500, 64KB is a significant chunk of a 512KB Chip RAM system.
**How to apply:** If reviewing other ported utilities with large static buffers, flag anything over 8KB for 68000 targets.
