# lib/libnspsl

NetSurf Public Suffix List lookup -- given a hostname, returns the
registrable domain (eTLD+1) for cookie scope checks and hyperlink
target validation.

Upstream: https://github.com/netsurf-browser/libnspsl @ commit
`82815c2`. Copyright 2016-2024 Vincent Sanders + others, MIT-licensed
(see `COPYING`).

## What it is

Libnspsl is a tiny (1 .c file / 208 LOC + ~13K LOC of pre-generated
static const tree+string data, 67 KB archive) lookup library backed by
the public suffix list from https://publicsuffix.org/.

Given a hostname like `www.example.co.uk`, the function returns
`example.co.uk` -- the registrable domain (eTLD+1). Per the PSL
algorithm: "the domain must match the public suffix plus one
additional label."

## Public API (single function)

```c
const char *nspsl_getpublicsuffix(const char *hostname);
```

- Returns: pointer INTO the input string (no allocation), or NULL on
  malformed input.
- The pointer is the start of the registrable domain within the input.
- The input must remain valid as long as the returned pointer is in
  use.
- No allocation, no free, no init, no cleanup.

Examples:

| Input | Returns |
|---|---|
| `example.com` | `example.com` |
| `www.example.com` | `example.com` |
| `bbc.co.uk` | `bbc.co.uk` |
| `a.b.c.example.co.uk` | `example.co.uk` |
| `u-tokyo.ac.jp` | `u-tokyo.ac.jp` |
| `NULL` | `NULL` |
| `""` | `NULL` |
| `.example.com` | `NULL` (leading dot rejected) |

## Build

```bash
make -C lib/libnspsl
```

Produces `libnspsl.a` (~67 KB; ~64 KB is the static const PSL data,
~3 KB is actual code).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention.

**Defines:** `-DNDEBUG -D_DEFAULT_SOURCE -std=c99`.

**Optimization:** whole-archive `-O1 -fno-strict-aliasing` per Stage 7
audit (see `PERF-REPORT.md`). Single TU is trivially -O1-safe per
crash-patterns #16 (no struct returns, integer-only).

**Depends on:** nothing (only `<stdint.h>` + `<string.h>`). Maximally
freestanding -- doesn't depend on libwapcaplet despite what the early
Phase D-prime planning notes assumed.

## Pre-generated PSL data (psl.inc)

The PSL data table (`src/psl.inc`) is generated from the upstream
PSL data file (`public_suffix_list.dat`, vendored alongside) via the
`src/genpubsuffix.pl` Perl tool. The output `.inc` is committed to the
tree so the library-build does NOT depend on Perl.

To regenerate when refreshing PSL data:

```bash
wget -O public_suffix_list.dat https://publicsuffix.org/list/public_suffix_list.dat
perl src/genpubsuffix.pl public_suffix_list.dat > src/psl.inc
```

## Test

```bash
make -C tests/libnspsl run
```

Runs the 18-test suite via `vamos -C 68040 -s 1024 -m 4096 ./test_libnspsl`.
Coverage:

- 8 functional (registrable domain extraction for .com / .org / .uk /
  .co.uk / .gov.au / .ac.jp; subdomain stripping; deep subdomain)
- 4 error path (NULL input, empty input, single-label input, leading dot)
- 3 edge case (long label, unrecognised TLD, returned-pointer-into-input)
- 1 Amiga-specific (no allocation -- 100 lookups, no leak)
- 2 stress (50 diverse lookups, 200-char hostname)

## Memory model

**Zero allocations.** All data is `static const` in the binary's text
segment. The function returns a pointer INTO the caller's input
string -- caller owns the lifetime, has nothing to free.

No process-wide globals that need `_finalise` cleanup. No init.
Re-entrant (no statics in the function or its helpers). Safe to call
from any context on AmigaOS `-noixemul`.

## Memory audit findings

See `lib/libnspsl/MEMORY-AUDIT.md`. APPROVED with no findings.

## Performance audit findings

See `lib/libnspsl/PERF-REPORT.md`. KEEP -O1 WHOLE-ARCHIVE. Per-call
cost ~3400 cycles on 68000 / ~1600 on 68060 -- negligible for
NetSurf's typical 10-100 calls per pageload.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- cookie domain scope
  validation, hyperlink target validation
