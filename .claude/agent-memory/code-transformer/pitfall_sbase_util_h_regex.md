---
name: sbase util.h regex dependency
description: sbase util.h pulls in <regex.h> which may not be available. Create sponge-util.h (or <portname>-util.h) with only the symbols the port actually uses.
type: feedback
---

When porting sbase tools (sponge, cat, etc.), the shared `util.h` includes `<regex.h>`.
If the tool being ported does not use regex, replacing `#include "util.h"` with a minimal
`<portname>-util.h` that only declares the functions the port calls avoids the regex.h
compile failure.

**Why:** bebbo-gcc libnix does not ship `<regex.h>`. Any file that transitively includes it
will fail to compile even if regex is never called.

**How to apply:** When a port uses only a subset of sbase helpers (eprintf/weprintf/concat/writeall),
create a `<portname>-util.h` with only those declarations. Replace all `#include "util.h"` and
`#include "../util.h"` references with the new header.
