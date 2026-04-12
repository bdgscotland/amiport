---
name: logname_analysis
description: OpenBSD logname v1.10 portability analysis: EASY verdict, trivially portable, getlogin() returns hardcoded "amiga" (Tier 1 stub), __dead strip, pledge stub, exit code fix (1->10), single source file
type: project
---

OpenBSD logname v1.10 -- portability analysis completed 2026-04-11.

**Verdict:** EASY -- Category 1 CLI, smallest possible footprint.

**Issues:**
- `getlogin()` -- Tier 1 stub, `amiport_getlogin()` returns "amiga". Never NULL on AmigaOS so the err(1,NULL) branch is dead code.
- `<err.h>` -- must become `<amiport/err.h>`, provides err()/errx()
- `<unistd.h>` -- libnix provides it; getopt() shim needed -> amiport_getopt()
- `pledge()` -- stub as macro `#define pledge(p,e) (0)`
- `__dead` -- remove attribute (OpenBSD-specific)
- `exit(1)` in usage() and err(1,...) -- change to exit(10)/err(10,...)
- `<stdlib.h>` -- replace with `<amiport/stdlib.h>` for exit() macro

**Why:** `getlogin()` always returns "amiga" -- no NULL path possible. Semantic stub value
printed directly to stdout -- user will always see "amiga" on AmigaOS. This is correct
behavior for a single-user OS.
