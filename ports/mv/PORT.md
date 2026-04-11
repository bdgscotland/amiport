# Port: mv

## Overview

| Field | Value |
|-------|-------|
| Program | mv |
| Version | 1.47 (port revision: 1) |
| Source | OpenBSD mv v1.47 (BSD 3-Clause) |
| Category | 1 -- CLI |
| License | BSD 3-Clause |
| Original Author | Ken Smith, UC Berkeley |
| Port Date | 2026-04-11 |

## Description

Move or rename files and directories. Uses rename() for same-volume moves, with fallback copy-and-delete for cross-volume regular file moves. Supports interactive (-i) and force (-f) modes.

## Prior Art on Aminet

GNU fileutils 3.3 (amiga-fileutils-3.3.lha, ~2000) includes mv but is 25+ years old. AmigaOS built-in RENAME command cannot move files across volumes. This port provides modern, standalone cross-volume move capability.
