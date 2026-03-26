# Port: which

## Overview

| Field | Value |
|-------|-------|
| Program | which |
| Version | 1.27 (port revision: 1) |
| Source | OpenBSD 1.27 |
| Category | 1 -- CLI |
| License | ISC |
| Original Author | Todd C. Miller |
| Port Date | 2026-03-26 |

## Description

Locate a program file in the user s path.

## Prior Art on Aminet

No existing which port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **PORTABLE** -- Pure Tier 1 POSIX dependencies.

## Transformations Applied

See amiport comments in ported source.

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |

## Test Results

See test-fsemu-cases.txt for full test suite.

## Known Limitations

None identified.
