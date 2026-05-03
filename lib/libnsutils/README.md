# lib/libnsutils

NetSurf utility library -- monotonic time + base64 encode/decode +
pwrite/pread emulation.

Upstream: https://github.com/netsurf-browser/libnsutils @ commit
`0bd3906`. Copyright 2014-2022 Vincent Sanders + others, MIT-licensed
(see `COPYING`).

## What it is

Libnsutils is a small (3 TUs / ~600 LOC) collection of cross-platform
helpers used by other NetSurf libs (libcss, libdom, etc.) and by NetSurf
itself:

- **Monotonic time** (`nsu_getmonotonic_ms`) -- millisecond-resolution
  monotonic counter, never goes backwards
- **Positional I/O** (`nsu_pwrite`, `nsu_pread`) -- POSIX-style pwrite /
  pread emulated on top of lseek + read/write for systems that don't
  natively support them
- **Base64 encode / decode** (`nsu_base64_encode`, `_decode_alloc`,
  + URL-safe variants) -- RFC4648 implementation with both standard
  and URL-safe alphabets
- **Endian helpers** (`endian.h`) -- inline endian-test + swap
- **Static assertion macro** (`assert.h`) -- compile-time `ns_static_assert`

It is the second library shipped in NetSurf-Vampire Phase 1 Wave 3
(the first was `lib/libnslog/`). Standalone -- no NetSurf-internal
dependencies.

## Public API

See `include/nsutils/`:

- `time.h` -- `nsu_getmonotonic_ms(uint64_t *current)`
- `unistd.h` -- `nsu_pwrite`, `nsu_pread`
- `base64.h` -- 8 base64 functions: 2 encoders + 2 alloc-encoders + 2
  alloc-decoders, each in standard + URL-safe variants
- `endian.h` -- inline `endian_host_is_le`, `endian_swap`,
  `endian_host_to_big`, `endian_big_to_host`
- `errors.h` -- `nsuerror` enum (28 codes)
- `assert.h` -- `ns_static_assert(e)` compile-time check macro

## Build

```bash
make -C lib/libnsutils
```

Produces `libnsutils.a` (~3 KB).

**CPU target:** `-m68040 -m68881`. Same NetSurf-Vampire dep stack
convention.

**Defines:** `-DNDEBUG -D_DEFAULT_SOURCE -std=c99`.

**Optimization:** whole-archive `-O1 -fno-strict-aliasing` per Stage 7
audit (see `PERF-REPORT.md`). All three TUs cleared crash-patterns
#16 (no struct returns >8 bytes) and #2 (no soft-float pulls).

**Depends on:** nothing (only libc/libnix + `<proto/timer.h>` for
ReadEClock).

## amiport patch: ftruncate fallback removed in nsu_pwrite

`src/unistd.c` upstream has an ESPIPE-triggered fallback that calls
`ftruncate(fd, offset)` to extend the file when lseek-past-EOF fails:

```c
if (sk == (off_t)-1) {
    if (errno == ESPIPE) {
        ret = ftruncate(fd, offset);  /* not in libnix */
        ...
    }
}
```

We removed this fallback because:

1. `libnix` has no `ftruncate()` -- the symbol is unresolved at link.
2. `SetFileSize()` via dos.library would require the libnix
   `fd -> AmigaDOS BPTR` mapping, which libnix does not expose
   publicly.
3. NetSurf's typical `pwrite` use (cache write + download write) is
   sequential and never seeks past EOF, so the dropped path is rarely
   exercised in practice.

If a downstream consumer needs to extend a file via pwrite, **pre-grow
the file first** with a regular `fwrite`/`write` pass before calling
`nsu_pwrite` at the new offset. Documented inline in
`src/unistd.c`.

## Consumer requirement: timer.device for `nsu_getmonotonic_ms`

The Amiga branch of `nsu_getmonotonic_ms` uses `ReadEClock()` from
`<proto/timer.h>`. Caller MUST open `timer.device` and provide the
`TimerBase` global before calling `nsu_getmonotonic_ms()`. Libnix does
NOT auto-open timer.device.

NetSurf does this as part of its normal Amiga init. Test binaries
exercise this in `tests/libnsutils/test_libnsutils.c::open_timer_device()`.

Pattern:
```c
#include <proto/timer.h>
struct Device *TimerBase = NULL;
struct MsgPort *port = CreateMsgPort();
struct timerequest *req = (struct timerequest *)CreateIORequest(
    port, sizeof(struct timerequest));
OpenDevice("timer.device", UNIT_MICROHZ,
           (struct IORequest *)req, 0);
TimerBase = req->tr_node.io_Device;
/* ... use nsu_getmonotonic_ms ... */
CloseDevice((struct IORequest *)req);
DeleteIORequest((struct IORequest *)req);
DeleteMsgPort(port);
```

## Test

```bash
make -C tests/libnsutils run
```

Runs the 22-test suite via `vamos -C 68040 -s 1024 -m 4096 ./test_libnsutils`.
Coverage:

- 8 functional (base64 encode/decode/round-trip both variants;
  pwrite/pread round-trip; monotonic time returns OK; endian helpers;
  RFC4648 known vectors)
- 4 error path (decode invalid chars; pwrite past EOF; pread past EOF;
  zero-length encode)
- 4 edge case (base64 padding boundaries: 1mod3, 2mod3, 0mod3; decode
  with padding)
- 2 Amiga-specific (monotonic time forward progress over busy-wait;
  uint64_t doesn't truncate or sign-extend)
- 4 stress (50 base64 cycles; 4 KB round-trip; 50 monotonic samples
  monotonically non-decreasing; 100 endian swaps round-trip)

## Memory + cleanup discipline

The `_alloc` base64 variants malloc the output buffer; **caller MUST
free**. The non-alloc variants write into a caller-provided buffer
(no allocation).

- `nsu_base64_encode_alloc(in, in_len, &out, &out_len)` -- caller
  frees `out`
- `nsu_base64_decode_alloc(in, in_len, &out, &out_len)` -- caller
  frees `out`
- `nsu_base64_encode(in, in_len, out, &out_len)` -- caller-owned `out`
- `nsu_base64_encode_url_alloc` -- caller frees `out`
- ...etc.

On AmigaOS `-noixemul`, leaks are permanent until reboot. Track every
`_alloc` result and free it (or `atexit` sweep).

## Memory audit findings

See `lib/libnsutils/MEMORY-AUDIT.md`. APPROVED, no critical findings.

## Performance audit findings

See `lib/libnsutils/PERF-REPORT.md`. KEEP -O1 WHOLE-ARCHIVE. All TUs
trivially -O1-safe; base64 hot loop runs at ~10-12 cycles/byte which
is fine for NetSurf's typical <1 KB payloads.

## Test ASSERT-failure leak caveat

Same as the prior dep-stack libs.

## Consumers

- `ports/netsurf/` (Phase 1 final consumer) -- monotonic time for
  rendering ticks, base64 for HTTP Basic Auth + data URIs, pwrite for
  cache file management
