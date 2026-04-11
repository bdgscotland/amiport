# Port: cmp

## Overview

| Field | Value |
|-------|-------|
| Program | cmp |
| Version | 1.19 (port revision: 1) |
| Source | OpenBSD cmp v1.19 (BSD 3-Clause) |
| Category | 1 -- CLI |
| License | BSD 3-Clause |
| Original Author | UC Berkeley |
| Port Date | 2026-04-11 |

## Description

Compare two files byte by byte. Reports first difference location (byte and line number) or confirms files are identical. Supports silent mode (-s), verbose mode (-l), and byte skip offsets.

## Prior Art on Aminet

Only GUI-based Cmp-AW (1995) and simple Compare 1.0 (1997) exist -- neither is POSIX-compliant. No standalone command-line cmp with -s/-l options available. This port provides standard Unix cmp behavior.
