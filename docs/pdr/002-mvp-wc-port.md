# PDR-002: MVP — Port `wc` as First Proof of Concept

## Status

Completed — MVP validated, pipeline proven through smoke test and first real port (cal)

## Date

2026-03-19

## Problem

We need a concrete, end-to-end demonstration that the porting pipeline works before investing in more complex infrastructure.

## Target Users

Contributors and early adopters evaluating whether amiport is viable.

## Decision

Port the BSD `wc` (word count) utility as the first MVP. It exercises the full pipeline while keeping complexity minimal.

## Rationale

`wc` is ideal because:
- Under 200 lines of C
- Uses only `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, `getopt.h`
- No networking, no fork/exec, no signals beyond basic handling
- Easy to verify: count words/lines/chars and compare output
- Well-understood by any C programmer

## Success Criteria

- `/analyze-source examples/wc/original/wc.c` produces a correct portability report
- `/port-project examples/wc/original/wc.c` transforms, builds, and tests automatically
- `vamos wc testfile` produces identical output to the host `wc testfile`

## Alternatives Considered

- **`cat`**: Even simpler but almost trivially portable — doesn't exercise enough of the pipeline
- **`grep`**: More interesting but requires regex library — too complex for MVP
- **`bc`**: Exercises parser/interpreter patterns but scope is large
