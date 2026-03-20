# Port Report: cal

## Original
- **Program**: cal — display a calendar
- **Source**: OpenBSD usr.bin/cal (v1.32, 2024-08-18)
- **License**: BSD 3-Clause (Kim Letkeman, Regents of the University of California)
- **Language**: C (563 lines)
- **URL**: https://github.com/openbsd/src/blob/master/usr.bin/cal/cal.c

## Prior Art on Aminet
- `cal20.lha` (1995) — binary-only, no source, by Andreas Tetzl
- `Cal1_2.lha` (1997) — binary-only, no source, by Brian Savage
- Neither includes source code; both are 30+ years old

## Analysis
- **Verdict**: EASY
- Pure computation and output — no file I/O, no networking, no process management
- All dependencies are standard C library functions or BSD extensions with trivial shims
- Zero blocking issues

## Transformations Applied

| # | Type | Original | Replacement | Notes |
|---|------|----------|-------------|-------|
| 1 | Header | `<err.h>` | `<amiport/err.h>` | New shim created for this port |
| 2 | Header | `<unistd.h>` | `<amiport/unistd.h>` | For getopt |
| 3 | Header | Added | `<amiport/getopt.h>` | getopt shim |
| 4 | Header | Added | `<exec/types.h>` | For LONG type |
| 5 | Function | `pledge()` | `#define pledge(p,e) (0)` | OpenBSD-only, no-op stub |
| 6 | Function | `strtonum()` | `amiport_strtonum()` | Via amiport/err.h shim |
| 7 | Function | `err()`/`errx()` | `amiport_err()`/`amiport_errx()` | Via amiport/err.h shim |
| 8 | Function | `strptime()` | Local month name scan | Replaced with C89 tolower() loop |
| 9 | Boilerplate | — | Version string added | `$VER: cal 1.0 (20.03.2026)` |
| 10 | Boilerplate | — | Stack cookie added | `LONG __stack = 32768` |

## New Shim Functions Created
- `amiport/err.h` — provides `err()`, `errx()`, `warn()`, `warnx()`, `strtonum()`
- This shim is reusable for future BSD program ports

## Shim Functions Exercised
- `amiport_getopt()` — command-line option parsing (-j, -m, -w, -y)
- `amiport_err()` / `amiport_errx()` — error reporting
- `amiport_strtonum()` — safe string-to-number conversion

## Build
```
make build TARGET=ports/cal    # Cross-compile
make test TARGET=ports/cal     # Test via vamos
make -C ports/cal package      # Create LHA for Aminet
```

## Test Results

| Test Case | Input | Expected | Status |
|-----------|-------|----------|--------|
| Current month | `cal` | Valid calendar output | PASS |
| Specific month | `cal 3 2026` | March 2026 calendar | PASS |
| Full year | `cal -y 2026` | All 12 months | PASS |
| Julian days | `cal -j 3 2026` | Day numbers 60-90 | PASS |
| Monday start | `cal -m 3 2026` | Week starts Monday | PASS |
| Invalid month | `cal 13 2026` | Error message | PASS |
| Month by name | `cal mar 2026` | March 2026 | PASS |

**7/7 tests passed.**

## Known Limitations
- No highlight of current day (would require ANSI terminal codes, which work on most Amiga terminals but not tested)
- Year range 1-9999 (same as original)
- Uses clib2's `localtime()` for current date — timezone depends on Amiga Locale preferences
