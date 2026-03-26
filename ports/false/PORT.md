# Port: false

## Overview

| Field | Value |
|-------|-------|
| Program | false |
| Version | 1.1 (port revision: 1) |
| Source | OpenBSD 1.1 |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

Return false value (exit status non-zero). Used in shell scripts.

## Prior Art on Aminet

No existing false port found on Aminet for AmigaOS 3.x.

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
