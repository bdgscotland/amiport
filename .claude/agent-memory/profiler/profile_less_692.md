---
name: less 692 runtime profile
description: ReadEClock profile of less 692 non-TTY (cat_file) path on vamos — 500 lines, 49KB input
type: project
---

## Profile Results (vamos, ~10MHz emulated)

Run: 500-line plain-text file, non-TTY mode (less exits via cat_file path when stdout not a terminal).

```
=== amiport profiler (10000000 Hz) ===
Function                Calls  Total(ms)  Avg(us)  Max(us)     %
cat_file                    1       1567   279320   279320   66%
ch_forw_get             49301        392        7      460   16%
putchr                  49300        380        7      275   16%
Total measured: 2340 ms
```

## Key Findings

### Non-TTY path (cat_file)
- `cat_file` accounts for 66% of total measured time — but this includes ch_forw_get and putchr overhead (they're nested)
- `ch_forw_get` and `putchr` are symmetric at ~16% each and ~7us/call avg
- At 7us/call and 49K chars, total I/O time is ~400ms for 49KB file on emulated 10MHz

### TTY display path — NOT measurable on vamos
`put_line`, `pappend_b`, `forw_line`, `is_ansi_end`, `is_ansi_middle` are only called when `is_tty=TRUE`. vamos doesn't provide a real TTY — less detects `isatty(1)=FALSE` and takes the `cat_file()` path, bypassing the entire display pipeline.

**Why**: less line 424 in main.c: `if (!is_tty) { cat_file(); quit(QUIT_OK); }`

To profile the display path, FS-UAE testing is required (real TTY via console.device).

### ANSI Lookup Table — Cannot verify on vamos
The `is_ansi_end`/`is_ansi_middle` lookup table optimization (replacing strchr over 24-char strings) cannot be empirically measured on vamos because it only fires in TTY display mode.

## Instrumented Functions

| Function | File | Status |
|----------|------|--------|
| `put_line` | output.c | Instrumented — not called (TTY only) |
| `pappend_b` | line.c | Instrumented — not called (TTY only) |
| `ch_forw_get` | ch.c | **Measured: 7us avg, 49K calls** |
| `forw_line` | input.c | Instrumented — not called (TTY only) |
| `is_ansi_end` | line.c | Instrumented — not called (TTY only) |
| `is_ansi_middle` | line.c | Instrumented — not called (TTY only) |
| `putchr` | output.c | **Measured: 7us avg, 49K calls** |
| `cat_file` | edit.c | **Measured: 1 call, entire non-TTY duration** |

## vamos Profiler Limitation
vamos E-Clock runs at 10MHz (not 709KHz PAL). Sub-microsecond functions complete in <1 tick and show 0 ticks. Functions that complete in <100ns on vamos won't appear in the table even if called thousands of times.

**Why:** line (profile.c:154): `if (!prof_initialized || prof_count == 0) return;`  — combined with fact that 0-tick functions do appear in table with call_count > 0, but in practice they all showed 0 entries because TTY path was never taken.

## Files Modified for Profiling
- `ports/less/ported/main.c` — profile init + atexit
- `ports/less/ported/output.c` — put_line, putchr instrumented
- `ports/less/ported/line.c` — pappend_b, is_ansi_end, is_ansi_middle instrumented
- `ports/less/ported/ch.c` — ch_forw_get instrumented
- `ports/less/ported/input.c` — forw_line instrumented
- `ports/less/ported/edit.c` — cat_file instrumented
- `ports/less/Makefile` — profile-build, profile-run targets added
- `ports/less/test-profile-input.txt` — 500-line test file

## Recommendation
For display-path profiling, dispatch FS-UAE run with test-profile-input.txt containing ANSI sequences (ESC[1m, ESC[31m etc) to exercise is_ansi_end/middle lookup tables.
