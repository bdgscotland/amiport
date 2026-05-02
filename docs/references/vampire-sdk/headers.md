# Vampire SDK Reference (from flype44/Vampire)

**Source:** [github.com/flype44/Vampire](https://github.com/flype44/Vampire) — the de-facto third-party Vampire SDK. Apollo Team does not publish these headers as a package, so flype44's repo is the canonical distribution. License: MPL 2.0 (per the SDK), header copyright "Apollo Team 2017", author Henryk Richter.

## Headers we need for amiport AMMX consumers

`includes/vampire/vampire.h` — the resource ID + AMMX constants:

```c
#define V_VAMPIRENAME "vampire.resource"

/* AMMX enable variants */
#define V_AMMX_DISABLE     0
#define V_AMMX_V1          1
#define V_AMMX_V2          2

/* AMMX enable return codes */
#define VRES_ERROR         0
#define VRES_OK            1
#define VRES_AMMX_WAS_ON   2

/* Unit numbers (resource ownership for HW access) */
#define V_SDPORT      0  /* SD port on V500/600 */
#define V_WIFIPORT    1  /* WiFi expansion port on V500+ */
#define V_PAMELA_45   2  /* Pamela channels 4,5 = first two 16-bit capable channels */
#define V_PAMELA_67   3  /* Pamela channels 6,7 = second two 16-bit capable channels */
#define V_PIP         4  /* reserve access to PIP */
#define V_V4NET       5  /* reserve access to V4 networking */
```

`includes/clib/vampire_protos.h` — function prototypes:

```c
APTR  V_AllocExpansionPort(ULONG unitNum, UBYTE *name);
VOID  V_FreeExpansionPort(ULONG unitNum);
ULONG V_EnableAMMX(ULONG version);
```

`includes/inline/vampire.h` — inline LP*() macros for direct library calls (auto-generated via sfdc 1.10):

```c
/* Library offsets:
 *   V_AllocExpansionPort = 0x6
 *   V_FreeExpansionPort  = 0xC
 *   V_EnableAMMX         = 0x12  (registers: D0 = version)
 */

#define V_EnableAMMX(___version) \
      LP1(0x12, ULONG, V_EnableAMMX, ULONG, ___version, d0,\
      , VAMPIRE_BASE_NAME)
```

`includes/proto/vampire.h` — pulls in the right glue based on compiler (GCC, AROS, OS4):

```c
#include <clib/vampire_protos.h>
#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/vampire.h>
#  else
#   include <inline/vampire.h>
#  endif
# else
#  include <pragmas/vampire_pragmas.h>
# endif
#endif
extern struct Library *VampireBase;
```

## Canonical init pattern (cribbed from arczi84/NetSurf-MUI jsimd_ammx.c)

```c
#include <vampire/vampire.h>
#include <proto/vampire.h>
#include <proto/exec.h>
#include <exec/execbase.h>

struct Library *VampireBase;

int ammx_init(void)
{
    VampireBase = OpenResource("vampire.resource");
    if (!VampireBase) return 0;
    if (VampireBase->lib_Version < 45) return 0;

    int res = V_EnableAMMX(V_AMMX_V2);
    if (res == VRES_ERROR) return 0;

    /* res == VRES_OK : we just turned it on (caller should disable on exit)
     * res == VRES_AMMX_WAS_ON : someone else already enabled it (don't disable) */
    return 1;
}

int ammx_exit(int res_from_init)
{
    /* Only disable if we were the ones who enabled it */
    if (res_from_init == VRES_OK)
        V_EnableAMMX(V_AMMX_DISABLE);
    return 1;
}
```

## Why we vendor instead of expecting users to install

- The amiport build pipeline must be self-contained; CI cannot rely on external Aminet packages
- Pinning to a specific commit gives reproducible builds across upstream changes
- MPL 2.0 permits redistribution as long as upstream license + notices remain intact

In amiport: `lib/vampire-sdk/` (vendored from this repo, see plan Task 2). The `amiport_ammx_init()` wrapper in `lib/posix-shim/include/amiport/ammx.h` consolidates the init+exit pattern above.

## Repo layout (in case other components are needed later)

```
flype44/Vampire/
├── includes/                — what we vendor for amiport
│   ├── vampire/vampire.h
│   ├── proto/vampire.h
│   ├── inline/vampire.h
│   ├── clib/vampire_protos.h
│   ├── pragmas/, fd/, sfd/, lvo/, vbcc/  — VBCC + classic toolchain glue
│   └── hardware/              — register defines for V4 hardware (Pamela audio, etc.)
├── ApolloControl/             — Apollo Control utility source
├── CPU/                       — Nemo, Macros, DisLib080
├── VIDEO/                     — sagacard, WBCandy, SuperSprites
├── AUDIO/, ETHERNET/, FLASH/, JOYPAD/, SDCARD/, ApolloWHDSet/, DEMOS/, ASSETS/
└── LICENSE  (MPL 2.0)
```

For the NetSurf Phase 1 work, only `includes/vampire/`, `includes/proto/`, `includes/clib/`, `includes/inline/` are needed.

## Cross-references

- `lib/posix-shim/include/amiport/ammx.h` — `amiport_ammx_init()` wrapper (planned)
- `ports/netsurf/` — first amiport consumer (NetSurf Vampire Phase 1)
- `docs/references/ammx/instruction-reference.md` — Apollo PRM AMMX instructions

## Discovery context

Cloned and indexed 2026-05-02 during the NetSurf Vampire Phase 1 KB hydration.
