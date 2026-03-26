---
name: port_getopt_perf
description: Performance findings for ports/getopt 1.10 — trivial single-pass CLI, no hot paths, clean bill of health reviewed 2026-03-26
type: project
---

# getopt 1.10 Performance Notes

**Why:** getopt is a single-shot argument parser — runs once, exits. No hot paths.

## Findings
- No loops over data, no file I/O, no allocations beyond argv expansion.
- printf() called once per option found — not a performance concern.
- Clean bill of health. Nothing to optimize.

## Verdict
Trivial utility. No performance work needed.
