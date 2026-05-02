# lib/vampire-sdk

Vendored from [github.com/flype44/Vampire](https://github.com/flype44/Vampire),
the de-facto Vampire (Apollo 68080) SDK. Apollo Team does not publish the
headers as a package, so flype44's repo is the canonical source.

**License:** MPL 2.0 (see LICENSE).

**Pinned commit:** 0b231e3ccfb56cb4b42443dbcb78ea717f4dfdb6

**Vendored files:**
- `include/vampire/vampire.h` — constants (`V_VAMPIRENAME`, `V_AMMX_V2`, unit numbers, return codes)
- `include/proto/vampire.h` — main header (includes clib protos + inline wrappers)
- `include/clib/vampire_protos.h` — C function prototypes (`V_EnableAMMX`, `V_AllocExpansionPort`, `V_FreeExpansionPort`)
- `include/inline/vampire.h` — GCC inline wrapper macros for function calls

**Updating:** Run `bash update-vampire-sdk.sh` to re-pull the latest upstream
and re-pin the commit. Review diff before committing.

**Consumers:**
- `lib/posix-shim/src/ammx_init.c` — `amiport_ammx_init()` calls
  `V_EnableAMMX(V_AMMX_V2)` after detecting AFB_68080.
- `ports/netsurf/` — Phase 1 of the FreeType+AMMX glyph compositor.

**Why we vendor instead of expecting the user to install:**
- The amiport build pipeline must be self-contained; CI cannot rely on
  external Aminet packages being present.
- Pinning to a specific commit gives us reproducible builds across
  upstream changes.
- MPL 2.0 permits redistribution as long as upstream license + notices
  remain intact.
