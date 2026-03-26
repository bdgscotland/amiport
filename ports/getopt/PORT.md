# Port: getopt

## Overview

| Field | Value |
|-------|-------|
| Program | getopt |
| Version | 1.10 (port revision: 1) |
| Source | OpenBSD 1.10 |
| Category | 1 -- CLI |
| License | Public Domain |
| Original Author | Henry Spencer |
| Port Date | 2026-03-26 |

## Description

Parse command options in shell scripts.

## Prior Art on Aminet

No existing getopt port found on Aminet for AmigaOS 3.x.

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
