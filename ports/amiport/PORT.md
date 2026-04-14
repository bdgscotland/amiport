# PORT.md — amiport 1.0

## Overview

| Field | Value |
|-------|-------|
| Program | amiport |
| Version | 1.0 |
| Category | 4 (Network) |
| Author | Duncan Bowring |
| Source | Original code (not a port) |
| Binary Size | 43,832 bytes (42 KB) |
| Stack | 65,536 bytes |
| License | MIT |

## Description

amiport is the first package manager for classic 68k AmigaOS. It downloads, verifies (SHA-256), and installs software packages from the amiport repository at amiport.platesteel.net. 9 CLI commands cover the full package lifecycle: discover, install, upgrade, remove, diagnose.

This is original Amiga software, written in C89 from scratch, not a port of existing code.

## Architecture

```
+---------------------------+
|        amiport CLI         |  9 commands
+---------------------------+
        |           |
+-------+--+  +----+------+
| http-shim |  | json.c   |
| (HTTP/1.0)|  | sha256.c |
+-------+--+  | db.c     |
        |      | config.c |
+-------+--------+
| bsdsocket-shim |
+----------------+
        |
+-------+--------+
| bsdsocket.lib  |  (Roadshow/Miami/AmiTCP)
+----------------+
```

## Dependencies

| Dependency | Type | Notes |
|-----------|------|-------|
| lib/posix-shim | Static library | exit(), signal, tmpfile |
| lib/bsdsocket-shim | Static library | BSD socket API via bsdsocket.library |
| lib/http-shim | Static library | HTTP/1.0 GET client |
| C:lha | Runtime | Package extraction (must be installed) |
| TCP/IP stack | Runtime | Roadshow, Miami, or AmiTCP |

## Commands

| Command | Description | Exit Code |
|---------|-------------|-----------|
| list | Show all packages with [installed]/[update] tags | 0 |
| search <term> | Case-insensitive substring match | 0 |
| info <name> | Package details with stream-printed rich fields | 0/10 |
| install <name> | Download + SHA-256 verify + extract + track | 0/5/10 |
| upgrade [name] | Version comparison + reinstall if different | 0/10 |
| remove <name> | Delete binary + remove from DB | 0/10 |
| installed | List from S:amiport.db | 0 |
| doctor | 4-step network diagnostic | 0/10/20 |
| help | Usage text | 0 |

## Design Decisions

1. **Static buffers only** — no malloc for I/O. JSON manifest loaded into 128KB static buffer.
2. **Stream-print rich fields** — porting_notes (up to 2,399 chars) printed during JSON parse, not stored in struct.
3. **strcmp() version comparison** — server-authoritative versions, != 0 means "different."
4. **Dual-format LHA** — machine-installable (C/<name>) for amiport, Aminet format for humans.
5. **Path prefix safety** — remove only deletes within configured install path.
6. **S:amiport.conf** — optional KEY=VALUE config for server URL, install path, color.

## Build Instructions

```
make -C lib/bsdsocket-shim    # Build network shim (if not already built)
make -C lib/http-shim          # Build HTTP client library
make -C ports/amiport           # Build amiport
```

## Test Results

### vamos Smoke Tests (no network)

| Test | Expected | Result |
|------|----------|--------|
| No args | help, RC=0 | PASS |
| help | help, RC=0 | PASS |
| unknown command | error, RC=10 | PASS |
| search (no arg) | error, RC=10 | PASS |
| install (no arg) | error, RC=10 | PASS |
| remove (no arg) | error, RC=10 | PASS |
| installed (empty DB) | "No packages installed", RC=0 | PASS |
| doctor (no bsdsocket) | FAIL, RC=20 | PASS |

### FS-UAE Network Tests

Pending — requires Roadshow/Miami configured in FS-UAE.

### Real Hardware Tests

Pending — A2000 + Vampire V2 500+ + X-Surf 100 + Roadshow.

## Known Limitations (v1)

- No `amiport upgrade amiport` (self-update deferred to v2)
- No multi-file package support (all packages are single-binary)
- No proxy support
- No HTTPS (bsdsocket.library + plain TCP only)
- Package limit: 128 packages (static array, truncates with warning)
- No dependency auto-resolution (requires checked, warn-only)

## Porting Notes

This is original code, not a port. Written in C89 for bebbo-gcc targeting AmigaOS 3.x on 68020+.

Key implementation choices:
- SHA-256 from Brad Conte's public domain crypto-algorithms (~300 lines)
- JSON parser scoped to amiport manifest schema (~250 lines)
- HTTP/1.0 client as reusable library (lib/http-shim)
- Config file at S:amiport.conf with KEY=VALUE format
- Package DB at S:amiport.db with space-delimited records

## Memory Safety

Pending memory-checker review.

## Performance

Pending perf-optimizer review.

## Re-stamp note (2026-04-14)

The original `amiport-1.0.lha` was published with an empty `sha256` field in `site/data/packages/amiport.json` (the bootstrap commit never filled it in). On 2026-04-14 the local `ports/amiport/amiport-1.0.lha` (47064 bytes) was stamped as the canonical 1.0 build: `amiport.json` updated with the actual size and sha256, file staged into `site/packages/`, and re-deployed. No source changes — only the catalog metadata was reconciled with what the `make package` build actually produces. The earlier 43832-byte build had no integrity signature, so no integrity guarantee is broken by this reconciliation.
