# posix-emu — Tier 2 POSIX Emulation Library for AmigaOS

`libamiport-emu.a` provides approximate POSIX emulation for functions that have no
direct AmigaOS equivalent. Each module documents its behavioral differences from POSIX.

## Quick Start

```c
#include <amiport-emu/amiport-emu.h>   /* all Tier 2 functions */
/* or include individual headers */
#include <amiport-emu/regex.h>
```

Link with: `-L../../lib/posix-emu -lamiport-emu`

For both Tier 1 + Tier 2:
```c
#include <amiport/amiport.h>
#define AMIPORT_INCLUDE_EMU
/* amiport.h includes Tier 2 when AMIPORT_INCLUDE_EMU is defined */
```

## Modules

### select() / poll() — I/O Multiplexing (`select.c`)

**Emulation:** `WaitForChar()` polling loop with `Wait()` on signal mask.

| Caveat | Detail |
|--------|--------|
| No level-triggered readiness | Uses polling, not notification |
| ~20ms minimum granularity | Limited to 1 Amiga tick |
| No socket fd support | Use bsdsocket-shim's `WaitSelect()` instead |
| `exceptfds` ignored | Not mapped |

### mmap(MAP_PRIVATE, PROT_READ) — Read-Only File Mapping (`mmap.c`)

**Emulation:** `AllocMem()` + `Read()` entire file. `munmap()` calls `FreeMem()`.

| Caveat | Detail |
|--------|--------|
| Full file read upfront | No lazy page faulting |
| Memory = file size | No page sharing or COW |
| Snapshot semantics only | File changes after mmap invisible |
| MAP_SHARED not supported | Use Tier 3 redesign |

### pipe() — Basic Pipe Emulation (`pipe.c`)

**Emulation:** `PIPE:amiport_NNNN` device (AmigaOS 2.0+).

| Caveat | Detail |
|--------|--------|
| Named pipe (not anonymous) | Visible in filesystem namespace |
| Different blocking/buffering | Not identical to POSIX anonymous pipes |
| No SIGPIPE | Write to closed read end doesn't signal |
| Less reliable EOF detection | Depends on PIPE: device behavior |

### alarm() / setitimer() — Timer Signals (`alarm.c`)

**Emulation:** `timer.device` request with cooperative signal checking.

| Caveat | Detail |
|--------|--------|
| Not asynchronous | Requires `amiport_check_alarm()` in main loop |
| No signal delivery | Cannot interrupt blocking calls |
| ~1ms resolution (68020+) | ~20ms on 68000 |

### regex — POSIX Regular Expressions (`regex.c`)

**Emulation:** Minimal backtracking NFA engine (859 lines).

| Caveat | Detail |
|--------|--------|
| No locale collation | `[.ch.]`, `[=a=]` not supported |
| No POSIX char classes | `[:alpha:]`, `[:digit:]` not supported |
| Max pattern: 512 bytes | Compiled pattern size limit |
| Max 9 capture groups | \1-\9 backreferences |
| Catastrophic backtracking | Not suitable for untrusted patterns |

## Design Rules

1. Each module is a single `.c`/`.h` pair
2. Headers are `<amiport-emu/name.h>`
3. Functions prefixed `amiport_emu_` (distinct from Tier 1 `amiport_`)
4. Every header has an `EMULATION NOTICE` comment block
5. May depend on Tier 1 internals via `<amiport/internal.h>` — never the reverse
6. Each module has `AMIPORT_EMU_xxx_ENABLED` compile-time flag to opt out

## Building

```bash
make build-emu     # from project root
# or
make               # from lib/posix-emu/
```

Produces `libamiport-emu.a`. Requires bebbo-gcc cross-compiler (via Docker).

## Testing

```bash
make test-emu      # from project root — runs all tests via vamos
```

Test files in `tests/emu/` (4 test files).

## Tier Model

This library is **Tier 2** — approximate emulation with documented caveats. For direct
mapping (Tier 1), see `lib/posix-shim/`. For functions requiring structural redesign
(Tier 3), see `docs/posix-tiers.md`.
