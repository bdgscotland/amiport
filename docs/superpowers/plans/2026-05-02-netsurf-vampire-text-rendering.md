# NetSurf Vampire Text Rendering — Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship NetSurf 3.11 on Apollo A6000 (Vampire V4) with FreeType-rendered, AMMX2-composited anti-aliased text. Hardware-required (no scalar fallback). Land reusable amiport infrastructure (vasm in toolchain, vendored Vampire SDK, glyph cache library, amiport_ammx_init API) along the way.

**Architecture:** Fork from arczi84/NetSurf-3.11-MUI (working AMMX precedent). Toolchain prep first (vasm 1.8b in Docker image, flype44 SDK vendored). Build amiport-side infrastructure next (`amiport_ammx_init`, `lib/glyph-cache/`). Then NetSurf skeleton building unchanged. Then AMMX assembly kernel as a standalone tested unit. Then C glue (`font_freetype.c`) wiring FreeType → glyph cache → AMMX kernel into NetSurf's font dispatch via a new `nsoption_bool(freetype_fonts)`. Hardware verification on A6000 each step. CI baselines last.

**Tech Stack:** bebbo-gcc 13.3 (existing `:gcc13` Docker image), vasm 1.8b (added to image), libnix, FreeType 2.13.3 (existing `lib/freetype/`), flype44/Vampire SDK (vendored), Apollo AMMX2 instructions (`PCMP`, `PMULA`, `STOREm3 #1`), NetSurf 3.11 MUI fork.

**Spec:** [`docs/superpowers/specs/2026-05-02-netsurf-vampire-text-rendering-design.md`](../specs/2026-05-02-netsurf-vampire-text-rendering-design.md) (commit `ed3a3344`).

---

## Pre-implementation: mandatory amiga-kb queries

Before starting any task, run these queries and capture results into `ports/netsurf/.claude/agent-memory/kb-baseline.md`. The plan's later tasks reference assumptions these queries should validate.

- [ ] **Query 1:** `amiga_pitfalls_for("vampire.resource V_EnableAMMX context switch")` — currently empty (gap logged). Re-query before starting; if anyone has added entries, factor them in.
- [ ] **Query 2:** `amiga_pitfalls_for("LockBitMapTags Picasso96 ARGB framebuffer pixel access")` — currently empty (gap logged). Same as above.
- [ ] **Query 3:** `amiga_pitfalls_for("FT_Load_Char FreeType bullet glyph rendering")` — should return our captured pitfall about `OT_GlyphMap8Bit` being OS4-only.
- [ ] **Query 4:** `amiga_pitfalls_for("AMMX PMULA STOREm3 alpha blend")` — read all hits; the AMMX section of `.claude/rules/known-pitfalls.md` is the canonical reference for the asm we'll write.
- [ ] **Query 5:** `amiga_search("RastPort BitMap Picasso96 LockBitMap pixel format")` — read top 5 hits to ground the framebuffer access pattern.
- [ ] **Query 6:** `amiga_recipe_lookup("ammx_alpha_blit")` — if a recipe exists, follow it; if not, our work establishes one.

Capture findings in the agent-memory file — they become the reference when the implementing engineer hits an unfamiliar API.

**Throughout implementation:** every new pitfall encountered MUST be routed via `/capture-learning` to `amiga_add_pitfall`. The corpus growth is a Phase 1 deliverable per the spec.

---

## File Structure

**New files (creates):**

```
toolchain/docker/Dockerfile.bebbo-gcc13         (modify — add vasm)
lib/vampire-sdk/                                 (new directory)
  ├── README.md                                  (new — vendor pin docs)
  ├── LICENSE                                    (new — MPL 2.0 from flype44)
  ├── include/vampire/vampire.h                  (new — vendored)
  ├── include/proto/vampire.h                    (new — vendored)
  ├── include/vampire/vampire_lvo.h              (new — vendored, if used)
  └── update-vampire-sdk.sh                      (new — re-pull script)
lib/posix-shim/include/amiport/ammx.h            (new)
lib/posix-shim/src/ammx_init.c                   (new)
lib/posix-shim/Makefile                          (modify — add ammx_init.o)
lib/glyph-cache/                                 (new directory)
  ├── Makefile                                   (new)
  ├── include/amiport/glyph_cache.h              (new)
  └── src/glyph_cache.c                          (new)
tests/glyph-cache/                               (new directory)
  ├── Makefile                                   (new)
  └── test_glyph_cache.c                         (new)
ports/netsurf/                                   (new port directory)
  ├── original/                                  (NetSurf-MUI clone, RO)
  ├── ported/frontends/amiga/font_freetype.c     (new)
  ├── ported/frontends/amiga/font_freetype.h     (new)
  ├── ported/frontends/amiga/font_freetype_ammx.asm  (new — vasm)
  ├── ported/frontends/amiga/font.c              (modify — patch dispatch)
  ├── ported/frontends/amiga/gui_options.c       (modify — add nsoption)
  ├── Makefile                                   (new)
  ├── PORT.md                                    (new)
  ├── netsurf.readme                             (new)
  ├── test-fsemu-cases.txt                       (new)
  └── test-fsemu-visual-cases.txt                (new)
top-level Makefile                               (modify — add build-vampire-sdk, build-glyph-cache, test-glyph-cache)
PORTS.md                                         (modify — add netsurf row)
README.md                                        (modify — add netsurf to ports table)
data/catalog.json                                (modify — add netsurf entry)
site/data/catalog.json                           (modify — sync from data/)
site/data/packages/netsurf.json                  (new — site package metadata)
CLAUDE.md                                        (modify — codebase map entries)
```

**Reference files (read-only, just to study):**
- `~/Developer/amiport/lib/freetype/Makefile` — pattern for amiport library Makefiles
- `~/Developer/amiport/lib/zlib/Makefile` — pattern for `-O0` default with per-file `-O1` hot-path promotion
- `~/Developer/amiport/lib/oniguruma/Makefile` — pattern for minimal library Makefile
- `~/Developer/amiport/.claude/rules/known-pitfalls.md` AMMX section — canonical AMMX2 instruction reference
- `~/Developer/amiport/.claude/rules/library-pipeline.md` — mandatory stages for `lib/<name>/` ports
- `/tmp/netsurf-68k-research/frontends/amiga/font_bullet.c:559-616` — current AA-broken path
- `https://github.com/arczi84/NetSurf-3.11-MUI` `frontends/amiga/jsimd_ammx.c` — AMMX wiring boilerplate
- `https://github.com/arczi84/NetSurf-3.11-MUI` `frontends/amiga/jdcolor-ammx.asm` — vasm AMMX kernel pattern

---

## Phase A — Toolchain Prep (vasm + Vampire SDK)

### Task 1: Patch Dockerfile to install vasm 1.8b

**Files:**
- Modify: `toolchain/docker/Dockerfile.bebbo-gcc13`

- [ ] **Step 1: Read the existing Dockerfile**

```bash
cat toolchain/docker/Dockerfile.bebbo-gcc13
```

Note where the toolchain installs (`/opt/m68k-amigaos/`) and where to add the vasm install block (after the bebbo-gcc install, before the final WORKDIR/ENTRYPOINT).

- [ ] **Step 2: Add vasm install block to Dockerfile**

Add the following block after the bebbo-gcc install step:

```dockerfile
# vasm 1.8b — AMMX2-aware m68k assembler (for Apollo 68080 AMMX kernels)
# Source: http://sun.hasenbraten.de/vasm/  (free, redistributable)
# Required by ports/netsurf and any future AMMX consumer.
RUN cd /tmp && \
    wget -q -O vasm.tar.gz http://sun.hasenbraten.de/vasm/release/vasm.tar.gz && \
    tar xzf vasm.tar.gz && cd vasm && \
    make CPU=m68k SYNTAX=mot && \
    install -m 755 vasmm68k_mot /usr/local/bin/ && \
    cd / && rm -rf /tmp/vasm*
```

- [ ] **Step 3: Build the patched image locally**

```bash
docker build -t amiport-toolchain-gcc13:vasm-test \
  -f toolchain/docker/Dockerfile.bebbo-gcc13 toolchain/docker/
```

Expected: build completes, `vasm.tar.gz` downloads successfully (if it fails to download, pin a specific URL — check sun.hasenbraten.de for the current release filename).

- [ ] **Step 4: Verify vasm is available in the image**

```bash
docker run --rm amiport-toolchain-gcc13:vasm-test which vasmm68k_mot
docker run --rm amiport-toolchain-gcc13:vasm-test vasmm68k_mot -? 2>&1 | head -5
```

Expected: prints `/usr/local/bin/vasmm68k_mot` and a vasm version banner mentioning m68k.

- [ ] **Step 5: Smoke-test vasm with -m68080**

```bash
docker run --rm -i amiport-toolchain-gcc13:vasm-test bash -c '
cat > /tmp/hello.s <<EOF
    section text,code
    moveq   #0,d0
    rts
EOF
vasmm68k_mot -m68080 -Fhunk -o /tmp/hello.o /tmp/hello.s && echo OK
'
```

Expected: prints `OK`. Confirms vasm accepts -m68080 mode.

- [ ] **Step 6: Commit**

```bash
git add toolchain/docker/Dockerfile.bebbo-gcc13
git commit -m "build(toolchain): add vasm 1.8b to gcc13 Docker image for AMMX kernels"
```

---

### Task 2: Vendor flype44/Vampire SDK to lib/vampire-sdk/

**Files:**
- Create: `lib/vampire-sdk/README.md`
- Create: `lib/vampire-sdk/LICENSE`
- Create: `lib/vampire-sdk/include/vampire/vampire.h`
- Create: `lib/vampire-sdk/include/proto/vampire.h`
- Create: `lib/vampire-sdk/update-vampire-sdk.sh`

- [ ] **Step 1: Clone flype44/Vampire to a temp location**

```bash
git clone --depth 1 https://github.com/flype44/Vampire /tmp/flype44-vampire
cd /tmp/flype44-vampire && git rev-parse HEAD > /tmp/flype44-vampire-commit.txt
```

Capture the commit SHA — we pin to it.

- [ ] **Step 2: Identify which headers we need**

```bash
grep -rn "V_EnableAMMX\|V_AMMX_V2\|V_VAMPIRENAME\|VampireBase\|AFB_68080" /tmp/flype44-vampire/ | head -20
```

Note which header file declares each. Expected: `include/proto/vampire.h` for the function prototypes, `include/vampire/vampire.h` for the constants. Verify these are the only headers we need (don't pull the whole SDK).

- [ ] **Step 3: Create lib/vampire-sdk directory and copy minimal headers**

```bash
mkdir -p lib/vampire-sdk/include/vampire lib/vampire-sdk/include/proto
cp /tmp/flype44-vampire/include/vampire/vampire.h lib/vampire-sdk/include/vampire/
cp /tmp/flype44-vampire/include/proto/vampire.h lib/vampire-sdk/include/proto/
cp /tmp/flype44-vampire/LICENSE lib/vampire-sdk/LICENSE 2>/dev/null || \
  curl -L https://www.mozilla.org/media/MPL/2.0/index.txt -o lib/vampire-sdk/LICENSE
```

If flype44 ships additional `vampire/` subheaders that the two main ones include, copy those too — check with `gcc -E` after Task 4.

- [ ] **Step 4: Write lib/vampire-sdk/README.md**

```markdown
# lib/vampire-sdk

Vendored from [github.com/flype44/Vampire](https://github.com/flype44/Vampire),
the de-facto Vampire (Apollo 68080) SDK. Apollo Team does not publish the
headers as a package, so flype44's repo is the canonical source.

**License:** MPL 2.0 (see LICENSE).

**Pinned commit:** <COMMIT_SHA from /tmp/flype44-vampire-commit.txt>

**Vendored files:**
- `include/vampire/vampire.h` — constants (`V_VAMPIRENAME`, `V_AMMX_V2`)
- `include/proto/vampire.h` — function prototypes (`V_EnableAMMX`)

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
```

Replace `<COMMIT_SHA>` with the value from `/tmp/flype44-vampire-commit.txt`.

- [ ] **Step 5: Write lib/vampire-sdk/update-vampire-sdk.sh**

```bash
#!/bin/bash
# Re-pull flype44/Vampire SDK and update the vendored headers.
# Run from project root: bash lib/vampire-sdk/update-vampire-sdk.sh
set -euo pipefail
TMP=$(mktemp -d)
trap "rm -rf $TMP" EXIT
git clone --depth 1 https://github.com/flype44/Vampire "$TMP/vampire"
COMMIT=$(cd "$TMP/vampire" && git rev-parse HEAD)
echo "Upstream HEAD: $COMMIT"
cp "$TMP/vampire/include/vampire/vampire.h" lib/vampire-sdk/include/vampire/
cp "$TMP/vampire/include/proto/vampire.h" lib/vampire-sdk/include/proto/
sed -i.bak "s/^\*\*Pinned commit:\*\* .*/\*\*Pinned commit:\*\* $COMMIT/" lib/vampire-sdk/README.md
rm lib/vampire-sdk/README.md.bak
echo "Done. Review diff and commit."
```

```bash
chmod +x lib/vampire-sdk/update-vampire-sdk.sh
```

- [ ] **Step 6: Verify the headers parse with bebbo-gcc**

```bash
cat > /tmp/vamp_test.c <<'EOF'
#include <vampire/vampire.h>
#include <proto/vampire.h>
int main(void) { return V_AMMX_V2; }
EOF

docker run --rm -v $(pwd):/work -v /tmp:/tmp amiport-toolchain-gcc13:vasm-test \
  bash -c "m68k-amigaos-gcc -noixemul -m68040 -m68881 -I/work/lib/vampire-sdk/include -E /tmp/vamp_test.c > /dev/null && echo OK"
```

Expected: `OK`. If preprocessor errors fire, copy missing transitively-included headers from flype44 and retry.

- [ ] **Step 7: Commit**

```bash
git add lib/vampire-sdk/
git commit -m "build(vampire-sdk): vendor flype44/Vampire headers for AMMX init

Pinned to commit <SHA>. MPL 2.0 license preserved. Consumed by
amiport_ammx_init() and ports/netsurf/."
```

Replace `<SHA>` with the actual pinned commit.

---

### Task 3: Push the patched Docker image to the registry

**Files:** none (registry operation)

- [ ] **Step 1: Tag the locally-built image**

```bash
docker tag amiport-toolchain-gcc13:vasm-test \
  ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b
```

- [ ] **Step 2: Push to ghcr.io**

```bash
docker push ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b
```

If push fails with auth errors, follow the user's standard `gh auth token | docker login ghcr.io -u $USER --password-stdin` flow.

- [ ] **Step 3: Update Makefile / scripts that reference the image tag**

```bash
grep -rn "amiport-toolchain-gcc13:latest" --include=Makefile --include="*.sh" .
```

For each hit, decide whether to bump to `:vasm-1.8b`. Conservative play: leave `:latest` for existing consumers (they don't need vasm), tag NetSurf-specific builds with `:vasm-1.8b` explicitly via `ports/netsurf/Makefile`.

- [ ] **Step 4: Update CI workflow**

```bash
grep -rn "amiport-toolchain-gcc13" .github/workflows/
```

If the CI references the image tag, ensure the `:vasm-1.8b` (or new `:latest` if you bumped) is used for the netsurf build job. Defer the actual netsurf CI job until Phase H.

- [ ] **Step 5: Smoke-test from registry**

```bash
docker pull ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b
docker run --rm ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b vasmm68k_mot -? 2>&1 | head -3
```

Expected: vasm version banner.

- [ ] **Step 6: Commit any Makefile changes from Step 3**

```bash
git status
git diff
git add -- Makefile <other modified files>
git commit -m "build(toolchain): pin netsurf to amiport-toolchain-gcc13:vasm-1.8b"
```

If no files changed in Step 3 (decided to defer until Phase D), skip the commit.

---

## Phase B — `amiport_ammx_init` API in lib/posix-shim

### Task 4: Write the public header `amiport/ammx.h`

**Files:**
- Create: `lib/posix-shim/include/amiport/ammx.h`

- [ ] **Step 1: Write the header**

```c
/* amiport/ammx.h — Apollo 68080 AMMX2 initialization wrapper.
 *
 * MUST be called once at process startup before any AMMX kernel runs.
 * Without this, AMMX2-aware code (E0-E23 register file, AMMX2 opcodes)
 * will not be saved correctly across context switches and the system
 * will Guru on the first task switch under load.
 *
 * This is the canonical entry point for amiport ports that link
 * vasm-assembled AMMX kernels.
 *
 * Hardware-required: there is no scalar fallback. If your port wants to
 * support stock 68k systems, it must check this return value and exit
 * cleanly (or use a non-AMMX code path managed by the port itself).
 */

#ifndef AMIPORT_AMMX_H
#define AMIPORT_AMMX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize AMMX2 context-switch handling for the current task.
 * Returns 0 on success, non-zero on failure:
 *   1 = no Apollo 68080 detected (AFB_68080 not in ExecBase->AttnFlags)
 *   2 = vampire.resource missing (V_VAMPIRENAME OpenResource failed)
 *   3 = vampire.resource version too old (< 45)
 *   4 = V_EnableAMMX(V_AMMX_V2) returned VRES_ERROR
 *
 * Safe to call multiple times — second and subsequent calls are no-ops
 * that re-return the cached first-call result.
 */
int amiport_ammx_init(void);

/* Returns the cached result of the most recent amiport_ammx_init call.
 * Returns -1 if amiport_ammx_init has never been called.
 */
int amiport_ammx_status(void);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_AMMX_H */
```

- [ ] **Step 2: Commit**

```bash
git add lib/posix-shim/include/amiport/ammx.h
git commit -m "shim: add amiport/ammx.h public API for Vampire AMMX init"
```

---

### Task 5: Write the implementation `lib/posix-shim/src/ammx_init.c`

**Files:**
- Create: `lib/posix-shim/src/ammx_init.c`

- [ ] **Step 1: Write the implementation**

```c
/* ammx_init.c — Apollo 68080 AMMX2 initialization.
 * Wraps the canonical pattern from arczi84/NetSurf-MUI's jsimd_ammx.c
 * and the Apollo team's V_EnableAMMX documentation.
 */

#include <amiport/ammx.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <proto/exec.h>

#include <vampire/vampire.h>
#include <proto/vampire.h>

extern struct ExecBase *SysBase;

/* Bit position in ExecBase->AttnFlags for Apollo 68080 detection.
 * Per Apollo PRM and known-pitfalls AMMX section. */
#ifndef AFB_68080
#define AFB_68080 10
#endif

#ifndef AFF_68080
#define AFF_68080 (1 << AFB_68080)
#endif

static int g_ammx_init_status = -1;

int amiport_ammx_init(void)
{
    struct Library *VampireBase;

    if (g_ammx_init_status != -1) {
        return g_ammx_init_status;
    }

    /* Step 1: confirm Apollo silicon present */
    if (!(SysBase->AttnFlags & AFF_68080)) {
        g_ammx_init_status = 1;
        return g_ammx_init_status;
    }

    /* Step 2: open vampire.resource */
    VampireBase = (struct Library *)OpenResource((CONST_STRPTR)V_VAMPIRENAME);
    if (!VampireBase) {
        g_ammx_init_status = 2;
        return g_ammx_init_status;
    }

    /* Step 3: confirm version >= 45 (V_EnableAMMX entry point) */
    if (VampireBase->lib_Version < 45) {
        g_ammx_init_status = 3;
        return g_ammx_init_status;
    }

    /* Step 4: enable AMMX2 context-switch handling for this task */
    if (V_EnableAMMX(V_AMMX_V2) == VRES_ERROR) {
        g_ammx_init_status = 4;
        return g_ammx_init_status;
    }

    g_ammx_init_status = 0;
    return g_ammx_init_status;
}

int amiport_ammx_status(void)
{
    return g_ammx_init_status;
}
```

- [ ] **Step 2: Update lib/posix-shim/Makefile to compile ammx_init.c**

```bash
grep -n "OBJS" lib/posix-shim/Makefile | head -5
```

Find the `OBJS = ...` list. Add `src/ammx_init.o`. Also add `-I../vampire-sdk/include` to the CFLAGS so vampire headers are reachable.

Show me the actual Makefile diff:

```makefile
# Before:
OBJS = src/file_io.o src/getopt.o src/...

# After:
OBJS = src/file_io.o src/getopt.o src/... src/ammx_init.o

# CFLAGS — add -I../vampire-sdk/include alongside the existing include dirs
CFLAGS += -Iinclude -I../vampire-sdk/include
```

- [ ] **Step 3: Build the shim**

```bash
make -C lib/posix-shim clean && make -C lib/posix-shim
```

Expected: builds clean, `libamiport.a` produced, contains `ammx_init.o`.

```bash
docker run --rm -v $(pwd):/work ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b \
  m68k-amigaos-nm /work/lib/posix-shim/libamiport.a | grep -E "ammx_init|ammx_status"
```

Expected: `_amiport_ammx_init T` and `_amiport_ammx_status T` symbols listed.

- [ ] **Step 4: Commit**

```bash
git add lib/posix-shim/src/ammx_init.c lib/posix-shim/Makefile
git commit -m "shim: implement amiport_ammx_init() for Apollo 68080 / Vampire"
```

---

### Task 6: Write a smoke-test consumer for ammx_init

**Files:**
- Create: `tests/ammx-init/Makefile`
- Create: `tests/ammx-init/test_ammx_init.c`

The test cannot prove AMMX2 actually works on vamos / FS-UAE (those don't emulate it), but it can prove that on non-Apollo systems `amiport_ammx_init()` returns the expected non-zero error code, and that on real A6000 it returns 0. We'll test both.

- [ ] **Step 1: Write tests/ammx-init/test_ammx_init.c**

```c
/* test_ammx_init.c — smoke test for amiport_ammx_init().
 * Expected behavior:
 *   - On non-Apollo systems (vamos, FS-UAE without Apollo emulation): returns non-zero
 *   - On real Apollo A6000: returns 0
 * The test prints the result code for inspection; it does not assert
 * a specific value because we want to run it on both Apollo and non-Apollo
 * to verify both paths.
 */

#include <stdio.h>
#include <stdlib.h>
#include <amiport/ammx.h>

long __stack = 32768;

int main(void)
{
    int rc1, rc2;

    printf("amiport_ammx_init test\n");
    printf("======================\n");

    rc1 = amiport_ammx_init();
    printf("First call:  rc=%d ", rc1);
    switch (rc1) {
        case 0: printf("(SUCCESS — AMMX2 enabled)\n"); break;
        case 1: printf("(no Apollo 68080 — expected on stock 68k)\n"); break;
        case 2: printf("(vampire.resource missing — expected on non-Vampire)\n"); break;
        case 3: printf("(vampire.resource too old — V<45)\n"); break;
        case 4: printf("(V_EnableAMMX failed — driver issue)\n"); break;
        default: printf("(unexpected code)\n"); break;
    }

    /* Second call should return cached value */
    rc2 = amiport_ammx_init();
    if (rc1 != rc2) {
        printf("FAIL: second call returned different value (%d vs %d)\n", rc1, rc2);
        return 10;
    }

    if (amiport_ammx_status() != rc1) {
        printf("FAIL: amiport_ammx_status() != amiport_ammx_init() result\n");
        return 10;
    }

    printf("Second call cache check: PASS\n");
    return 0;
}
```

- [ ] **Step 2: Write tests/ammx-init/Makefile**

```makefile
# tests/ammx-init/Makefile
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CFLAGS = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall \
         -I../../lib/posix-shim/include \
         -I../../lib/vampire-sdk/include
LDFLAGS = -L../../lib/posix-shim -lamiport

VAMOS_STACK = 256
VAMOS_CPU = 68040

all: test_ammx_init

test_ammx_init: test_ammx_init.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: test_ammx_init
	vamos -C $(VAMOS_CPU) -s $(VAMOS_STACK) ./test_ammx_init

clean:
	rm -f test_ammx_init

.PHONY: all run clean
```

- [ ] **Step 3: Build the test**

```bash
make -C tests/ammx-init
```

Expected: builds clean, `tests/ammx-init/test_ammx_init` produced.

- [ ] **Step 4: Run on vamos**

```bash
make -C tests/ammx-init run
```

Expected output:
```
amiport_ammx_init test
======================
First call:  rc=2 (vampire.resource missing — expected on non-Vampire)
Second call cache check: PASS
```

(rc may be 1 instead of 2 depending on whether vamos reports AFF_68080.)

- [ ] **Step 5: Document the A6000 verification step in PORT.md format**

Note in the test directory's README that the on-A6000 test is required and shows expected output `rc=0`. Defer actually running on A6000 to Task 7.

- [ ] **Step 6: Commit**

```bash
git add tests/ammx-init/
git commit -m "test: add amiport_ammx_init smoke test (vamos: expects rc!=0)"
```

---

### Task 7: Hardware smoke-test on A6000

**Files:** none (verification only)

- [ ] **Step 1: Push test_ammx_init binary to A6000**

```bash
python3 -m amigactl --host 192.168.1.215 put tests/ammx-init/test_ammx_init T:test_ammx_init
```

(Adjust IP and amigactl path per `~/.claude/projects/-Users-duncan-Developer-amiport/memory/reference_amigactl.md`.)

- [ ] **Step 2: Run on A6000**

```bash
python3 -m amigactl --host 192.168.1.215 exec "T:test_ammx_init"
```

Expected output:
```
amiport_ammx_init test
======================
First call:  rc=0 (SUCCESS — AMMX2 enabled)
Second call cache check: PASS
```

- [ ] **Step 3: If rc != 0 on A6000, debug**

If you see rc=2 on A6000 (vampire.resource missing), the user's A6000 may not have the resource installed — verify with `python3 -m amigactl --host 192.168.1.215 sysinfo`. If rc=3, the resource is too old — note in PORT.md as a runtime requirement. If rc=4, V_EnableAMMX itself rejected — file a `/capture-learning` and dig deeper before proceeding to Phase C.

- [ ] **Step 4: Capture learning if any new pitfall surfaced**

If anything surprising happened (e.g., vampire.resource version requirement higher than expected, AttnFlags bit not set despite being on Apollo, V_EnableAMMX behavior different from documentation), invoke `/capture-learning` and route to `amiga_add_pitfall` per spec.

- [ ] **Step 5: No commit needed (verification only)**

---

## Phase C — `lib/glyph-cache/` standalone library

### Task 8: Write the public header `amiport/glyph_cache.h`

**Files:**
- Create: `lib/glyph-cache/include/amiport/glyph_cache.h`

- [ ] **Step 1: Write the header**

```c
/* amiport/glyph_cache.h — generic LRU glyph cache for AmigaOS ports.
 *
 * Reusable by any text-rendering port (NetSurf, mg, less, future PDF
 * viewer, terminal emulator). Caches 8-bit alpha bitmaps keyed by
 * (face_id, codepoint, px_size, hint_flags).
 *
 * Storage: pre-sized slab arena allocated at create() time. Glyph
 * bitmaps allocated bump-pointer style; LRU eviction reclaims oldest
 * entries when the arena fills.
 *
 * Thread-safety: NONE. AmigaOS is single-threaded; this is fine.
 *
 * No dynamic dependencies — pure C, links into libnix programs without
 * pulling additional libraries.
 */

#ifndef AMIPORT_GLYPH_CACHE_H
#define AMIPORT_GLYPH_CACHE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct amiport_glyph_cache amiport_glyph_cache_t;

typedef struct {
    const uint8_t *bitmap;   /* 8-bit alpha, owned by cache */
    int32_t        advance_x_q16; /* 16.16 fixed-point */
    int16_t        bearing_x;
    int16_t        bearing_y;
    uint16_t       width;
    uint16_t       height;
    uint16_t       stride;   /* bytes per row */
} amiport_glyph_t;

/* Create a cache with the given arena size in bytes.
 * Returns NULL on allocation failure.
 * Recommended sizes: 256 KB - 2 MB depending on expected text load.
 */
amiport_glyph_cache_t *amiport_glyph_cache_create(size_t arena_bytes);

/* Free the cache and its arena. Safe to pass NULL. */
void amiport_glyph_cache_destroy(amiport_glyph_cache_t *cache);

/* Look up a glyph by composite key.
 * Returns 1 on hit (out_glyph filled in), 0 on miss.
 * Hits update the LRU ordering.
 */
int amiport_glyph_cache_lookup(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                amiport_glyph_t *out_glyph);

/* Insert a glyph. Copies the bitmap into the cache's arena.
 * Returns 1 on success, 0 if the glyph is larger than the arena
 * can ever hold (caller should bypass cache for this glyph).
 * Triggers LRU eviction as needed to make room.
 */
int amiport_glyph_cache_insert(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                const amiport_glyph_t *glyph);

/* Statistics (for tuning). */
typedef struct {
    size_t arena_bytes;
    size_t arena_used;
    size_t entry_count;
    size_t hit_count;
    size_t miss_count;
    size_t eviction_count;
} amiport_glyph_cache_stats_t;

void amiport_glyph_cache_get_stats(const amiport_glyph_cache_t *cache,
                                    amiport_glyph_cache_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_GLYPH_CACHE_H */
```

- [ ] **Step 2: Commit**

```bash
mkdir -p lib/glyph-cache/include/amiport lib/glyph-cache/src
git add lib/glyph-cache/include/amiport/glyph_cache.h
git commit -m "lib(glyph-cache): public API header"
```

---

### Task 9: Write the test framework setup

**Files:**
- Create: `tests/glyph-cache/Makefile`
- Create: `tests/glyph-cache/test_glyph_cache.c`

- [ ] **Step 1: Write tests/glyph-cache/test_glyph_cache.c**

Use the existing `tests/shim/test_framework.h` pattern — `TEST(name) { ASSERT(...); }` style. Examine `tests/shim/test_*.c` for the convention.

```c
/* test_glyph_cache.c — unit tests for lib/glyph-cache.
 * Runs on vamos (no AMMX or AmigaOS-specific calls).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <amiport/glyph_cache.h>
#include "../shim/test_framework.h"

long __stack = 32768;

/* Tests added in subsequent tasks: */
TEST(create_destroy) {
    amiport_glyph_cache_t *c = amiport_glyph_cache_create(64 * 1024);
    ASSERT(c != NULL);
    amiport_glyph_cache_destroy(c);
}

TEST_MAIN(create_destroy)
```

- [ ] **Step 2: Write tests/glyph-cache/Makefile**

```makefile
# tests/glyph-cache/Makefile
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

CFLAGS = -O0 -noixemul -m68000 -std=gnu99 -Wall \
         -I../../lib/glyph-cache/include \
         -I../shim
LDFLAGS = -L../../lib/glyph-cache -lglyphcache

VAMOS_STACK = 256

all: test_glyph_cache

test_glyph_cache: test_glyph_cache.c ../../lib/glyph-cache/libglyphcache.a
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: test_glyph_cache
	vamos -s $(VAMOS_STACK) ./test_glyph_cache

clean:
	rm -f test_glyph_cache

.PHONY: all run clean
```

- [ ] **Step 3: Verify the test build setup compiles (it will fail because no implementation yet — that's expected)**

```bash
make -C tests/glyph-cache 2>&1 | head -20
```

Expected: link error — `libglyphcache.a` doesn't exist yet, or undefined references to `amiport_glyph_cache_*`. This confirms our test scaffolding is wired correctly.

- [ ] **Step 4: Commit (test scaffolding only — implementation comes next)**

```bash
git add tests/glyph-cache/
git commit -m "test(glyph-cache): test scaffolding (red — implementation TBD)"
```

---

### Task 10: TDD — implement create/destroy

**Files:**
- Create: `lib/glyph-cache/Makefile`
- Create: `lib/glyph-cache/src/glyph_cache.c`

- [ ] **Step 1: Write lib/glyph-cache/Makefile**

```makefile
# lib/glyph-cache/Makefile
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

CFLAGS = -O0 -noixemul -m68000 -std=gnu99 -Wall -Iinclude

OBJS = src/glyph_cache.o

all: libglyphcache.a

libglyphcache.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libglyphcache.a

.PHONY: all clean
```

- [ ] **Step 2: Write minimal src/glyph_cache.c stub for create/destroy**

```c
/* glyph_cache.c — implementation of amiport/glyph_cache.h. */

#include <amiport/glyph_cache.h>
#include <stdlib.h>
#include <string.h>

struct amiport_glyph_cache {
    uint8_t *arena;
    size_t   arena_bytes;
    size_t   arena_used;
    /* hash table, LRU list, etc. added in later tasks */
};

amiport_glyph_cache_t *amiport_glyph_cache_create(size_t arena_bytes)
{
    amiport_glyph_cache_t *c;

    if (arena_bytes < 1024) return NULL; /* nonsense size */

    c = calloc(1, sizeof(*c));
    if (!c) return NULL;

    c->arena = malloc(arena_bytes);
    if (!c->arena) { free(c); return NULL; }

    c->arena_bytes = arena_bytes;
    c->arena_used = 0;
    return c;
}

void amiport_glyph_cache_destroy(amiport_glyph_cache_t *cache)
{
    if (!cache) return;
    free(cache->arena);
    free(cache);
}

/* Stubs for the rest of the API — filled in by Tasks 11-12. */
int amiport_glyph_cache_lookup(amiport_glyph_cache_t *cache,
                                uint32_t face_id, uint32_t codepoint,
                                uint16_t px_size, uint16_t hint_flags,
                                amiport_glyph_t *out_glyph)
{
    (void)cache; (void)face_id; (void)codepoint;
    (void)px_size; (void)hint_flags; (void)out_glyph;
    return 0; /* always miss until implemented */
}

int amiport_glyph_cache_insert(amiport_glyph_cache_t *cache,
                                uint32_t face_id, uint32_t codepoint,
                                uint16_t px_size, uint16_t hint_flags,
                                const amiport_glyph_t *glyph)
{
    (void)cache; (void)face_id; (void)codepoint;
    (void)px_size; (void)hint_flags; (void)glyph;
    return 0; /* always reject until implemented */
}

void amiport_glyph_cache_get_stats(const amiport_glyph_cache_t *cache,
                                    amiport_glyph_cache_stats_t *out)
{
    if (!cache || !out) return;
    memset(out, 0, sizeof(*out));
    out->arena_bytes = cache->arena_bytes;
    out->arena_used = cache->arena_used;
}
```

- [ ] **Step 3: Build the library**

```bash
make -C lib/glyph-cache
```

Expected: `libglyphcache.a` produced, no errors.

- [ ] **Step 4: Build and run the test**

```bash
make -C tests/glyph-cache run
```

Expected output: the test framework's standard pass output for `create_destroy` test, exit code 0.

- [ ] **Step 5: Commit**

```bash
git add lib/glyph-cache/Makefile lib/glyph-cache/src/glyph_cache.c
git commit -m "lib(glyph-cache): implement create/destroy (other API stubbed)"
```

---

### Task 11: TDD — insert/lookup with hash table

**Files:**
- Modify: `tests/glyph-cache/test_glyph_cache.c`
- Modify: `lib/glyph-cache/src/glyph_cache.c`

- [ ] **Step 1: Add insert/lookup tests**

Add to test_glyph_cache.c after the existing `create_destroy` test:

```c
TEST(insert_then_lookup_hits) {
    amiport_glyph_cache_t *c = amiport_glyph_cache_create(64 * 1024);
    ASSERT(c != NULL);

    uint8_t bitmap[16] = {0,0,0,0, 1,2,3,4, 5,6,7,8, 9,10,11,12};
    amiport_glyph_t in = { .bitmap = bitmap, .advance_x_q16 = 0x100000,
                           .bearing_x = 1, .bearing_y = 2,
                           .width = 4, .height = 4, .stride = 4 };
    ASSERT(amiport_glyph_cache_insert(c, 1, 'A', 16, 0, &in) == 1);

    amiport_glyph_t out;
    ASSERT(amiport_glyph_cache_lookup(c, 1, 'A', 16, 0, &out) == 1);
    ASSERT(out.width == 4);
    ASSERT(out.height == 4);
    ASSERT(out.advance_x_q16 == 0x100000);
    ASSERT(memcmp(out.bitmap, bitmap, 16) == 0);

    amiport_glyph_cache_destroy(c);
}

TEST(lookup_miss_returns_zero) {
    amiport_glyph_cache_t *c = amiport_glyph_cache_create(64 * 1024);
    ASSERT(c != NULL);
    amiport_glyph_t out;
    ASSERT(amiport_glyph_cache_lookup(c, 99, 99, 99, 0, &out) == 0);
    amiport_glyph_cache_destroy(c);
}

TEST(different_keys_distinct) {
    amiport_glyph_cache_t *c = amiport_glyph_cache_create(64 * 1024);
    uint8_t bm_a[4] = {1,2,3,4};
    uint8_t bm_b[4] = {5,6,7,8};
    amiport_glyph_t in_a = { .bitmap = bm_a, .width = 2, .height = 2, .stride = 2 };
    amiport_glyph_t in_b = { .bitmap = bm_b, .width = 2, .height = 2, .stride = 2 };
    ASSERT(amiport_glyph_cache_insert(c, 1, 'A', 16, 0, &in_a) == 1);
    ASSERT(amiport_glyph_cache_insert(c, 1, 'B', 16, 0, &in_b) == 1);
    amiport_glyph_t out_a, out_b;
    ASSERT(amiport_glyph_cache_lookup(c, 1, 'A', 16, 0, &out_a) == 1);
    ASSERT(amiport_glyph_cache_lookup(c, 1, 'B', 16, 0, &out_b) == 1);
    ASSERT(out_a.bitmap[0] == 1);
    ASSERT(out_b.bitmap[0] == 5);
    amiport_glyph_cache_destroy(c);
}
```

Update `TEST_MAIN(...)` to include the new tests.

- [ ] **Step 2: Run tests — expect failures**

```bash
make -C tests/glyph-cache run
```

Expected: `insert_then_lookup_hits` and `different_keys_distinct` FAIL (insert always returns 0 in stub). `lookup_miss_returns_zero` and `create_destroy` PASS.

- [ ] **Step 3: Implement hash table + insert/lookup in glyph_cache.c**

Add to the struct:

```c
#define GC_MAX_ENTRIES 4096
#define GC_HASH_SIZE   8192  /* 2x for open addressing */

typedef struct {
    uint64_t key;       /* (face_id<<32) | (codepoint<<16) | (px_size<<8) | hint_flags
                         * note: hint_flags 8-bit only; widen if needed */
    size_t   bitmap_off; /* offset into arena */
    int32_t  advance_x_q16;
    int16_t  bearing_x, bearing_y;
    uint16_t width, height, stride;
    uint32_t lru_prev, lru_next; /* index into entries[] */
} gc_entry_t;

struct amiport_glyph_cache {
    uint8_t *arena;
    size_t arena_bytes, arena_used;
    gc_entry_t *entries;
    size_t entry_count, entry_cap;
    uint32_t *hash_table; /* GC_HASH_SIZE entries, value = entry index + 1, 0 = empty */
    uint32_t lru_head, lru_tail; /* doubly linked list, indices into entries[] */
    size_t hit_count, miss_count, eviction_count;
};
```

Pack the key:

```c
static uint64_t make_key(uint32_t face_id, uint32_t codepoint,
                         uint16_t px_size, uint16_t hint_flags)
{
    return ((uint64_t)face_id << 32) |
           ((uint64_t)codepoint << 16) |
           ((uint64_t)px_size << 8) |
           ((uint64_t)hint_flags & 0xff);
}

static uint32_t hash_key(uint64_t key)
{
    /* MurmurHash3 64->32 finalizer */
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53ULL;
    key ^= key >> 33;
    return (uint32_t)key & (GC_HASH_SIZE - 1);
}
```

Implement create / destroy with allocations for `entries[]` and `hash_table`.

Implement `insert` — bump-allocate from arena, store entry, insert into hash table via linear probing, link into LRU tail:

```c
int amiport_glyph_cache_insert(amiport_glyph_cache_t *c, uint32_t face_id,
    uint32_t codepoint, uint16_t px_size, uint16_t hint_flags,
    const amiport_glyph_t *g)
{
    size_t bytes = (size_t)g->stride * g->height;
    if (bytes > c->arena_bytes / 4) return 0; /* reject huge glyphs */
    if (c->entry_count >= c->entry_cap) return 0; /* eviction in Task 12 */

    /* Bump-alloc; eviction handled in Task 12 if arena full */
    if (c->arena_used + bytes > c->arena_bytes) return 0;

    uint32_t entry_idx = (uint32_t)c->entry_count++;
    gc_entry_t *e = &c->entries[entry_idx];
    e->key = make_key(face_id, codepoint, px_size, hint_flags);
    e->bitmap_off = c->arena_used;
    memcpy(c->arena + c->arena_used, g->bitmap, bytes);
    c->arena_used += bytes;
    e->advance_x_q16 = g->advance_x_q16;
    e->bearing_x = g->bearing_x;
    e->bearing_y = g->bearing_y;
    e->width = g->width;
    e->height = g->height;
    e->stride = g->stride;

    /* hash table insert (linear probe) */
    uint32_t h = hash_key(e->key);
    while (c->hash_table[h] != 0) h = (h + 1) & (GC_HASH_SIZE - 1);
    c->hash_table[h] = entry_idx + 1; /* +1 so 0 = empty sentinel */

    /* LRU tail link — added in Task 12 */
    return 1;
}

int amiport_glyph_cache_lookup(amiport_glyph_cache_t *c, uint32_t face_id,
    uint32_t codepoint, uint16_t px_size, uint16_t hint_flags,
    amiport_glyph_t *out)
{
    uint64_t key = make_key(face_id, codepoint, px_size, hint_flags);
    uint32_t h = hash_key(key);
    while (c->hash_table[h] != 0) {
        uint32_t entry_idx = c->hash_table[h] - 1;
        gc_entry_t *e = &c->entries[entry_idx];
        if (e->key == key) {
            out->bitmap = c->arena + e->bitmap_off;
            out->advance_x_q16 = e->advance_x_q16;
            out->bearing_x = e->bearing_x;
            out->bearing_y = e->bearing_y;
            out->width = e->width;
            out->height = e->height;
            out->stride = e->stride;
            c->hit_count++;
            return 1;
        }
        h = (h + 1) & (GC_HASH_SIZE - 1);
    }
    c->miss_count++;
    return 0;
}
```

Update `create()` to allocate `entries[]` (size `GC_MAX_ENTRIES` for now — Task 12 makes this a function of arena_bytes) and `hash_table`. Update `destroy()` to free them.

- [ ] **Step 4: Build and run tests — expect PASS**

```bash
make -C lib/glyph-cache && make -C tests/glyph-cache run
```

Expected: all 4 tests PASS.

- [ ] **Step 5: Commit**

```bash
git add lib/glyph-cache/src/glyph_cache.c tests/glyph-cache/test_glyph_cache.c
git commit -m "lib(glyph-cache): hash table + insert/lookup (no eviction yet)"
```

---

### Task 12: TDD — LRU eviction

**Files:**
- Modify: `tests/glyph-cache/test_glyph_cache.c`
- Modify: `lib/glyph-cache/src/glyph_cache.c`

- [ ] **Step 1: Add eviction tests**

```c
TEST(eviction_under_pressure) {
    /* Tiny arena: only ~4 small glyphs fit */
    amiport_glyph_cache_t *c = amiport_glyph_cache_create(4096);
    ASSERT(c != NULL);

    uint8_t bm[256];
    memset(bm, 0xAB, 256);
    amiport_glyph_t in = { .bitmap = bm, .width = 16, .height = 16, .stride = 16 };

    /* Insert way more than fits */
    for (int i = 0; i < 100; i++) {
        amiport_glyph_cache_insert(c, 1, i, 16, 0, &in);
    }

    /* Earliest should be evicted, latest should be present */
    amiport_glyph_t out;
    ASSERT(amiport_glyph_cache_lookup(c, 1, 0, 16, 0, &out) == 0);  /* evicted */
    ASSERT(amiport_glyph_cache_lookup(c, 1, 99, 16, 0, &out) == 1); /* recent */

    amiport_glyph_cache_stats_t s;
    amiport_glyph_cache_get_stats(c, &s);
    ASSERT(s.eviction_count > 0);

    amiport_glyph_cache_destroy(c);
}

TEST(lookup_promotes_to_mru) {
    amiport_glyph_cache_t *c = amiport_glyph_cache_create(4096);
    uint8_t bm[256]; memset(bm, 0, 256);
    amiport_glyph_t in = { .bitmap = bm, .width = 16, .height = 16, .stride = 16 };

    /* Fill cache with codepoints 0..N */
    for (int i = 0; i < 10; i++) amiport_glyph_cache_insert(c, 1, i, 16, 0, &in);

    /* Repeatedly look up codepoint 0 to keep it MRU */
    for (int j = 0; j < 50; j++) {
        amiport_glyph_t out;
        amiport_glyph_cache_lookup(c, 1, 0, 16, 0, &out);
    }

    /* Insert many more — codepoint 0 should survive */
    for (int i = 100; i < 200; i++) amiport_glyph_cache_insert(c, 1, i, 16, 0, &in);

    amiport_glyph_t out;
    ASSERT(amiport_glyph_cache_lookup(c, 1, 0, 16, 0, &out) == 1);

    amiport_glyph_cache_destroy(c);
}
```

- [ ] **Step 2: Run tests — expect failures**

```bash
make -C tests/glyph-cache run
```

Expected: `eviction_under_pressure` and `lookup_promotes_to_mru` FAIL (insert just returns 0 once arena full).

- [ ] **Step 3: Implement LRU + eviction**

Add LRU list operations to glyph_cache.c:

```c
#define GC_NULL_IDX 0xFFFFFFFFu

static void lru_unlink(amiport_glyph_cache_t *c, uint32_t idx)
{
    gc_entry_t *e = &c->entries[idx];
    if (e->lru_prev != GC_NULL_IDX) c->entries[e->lru_prev].lru_next = e->lru_next;
    else c->lru_head = e->lru_next;
    if (e->lru_next != GC_NULL_IDX) c->entries[e->lru_next].lru_prev = e->lru_prev;
    else c->lru_tail = e->lru_prev;
}

static void lru_push_tail(amiport_glyph_cache_t *c, uint32_t idx)
{
    gc_entry_t *e = &c->entries[idx];
    e->lru_prev = c->lru_tail;
    e->lru_next = GC_NULL_IDX;
    if (c->lru_tail != GC_NULL_IDX) c->entries[c->lru_tail].lru_next = idx;
    else c->lru_head = idx;
    c->lru_tail = idx;
}

/* Remove from hash table (called during eviction) */
static void hash_remove(amiport_glyph_cache_t *c, uint64_t key)
{
    uint32_t h = hash_key(key);
    while (c->hash_table[h] != 0) {
        uint32_t entry_idx = c->hash_table[h] - 1;
        if (c->entries[entry_idx].key == key) {
            c->hash_table[h] = 0;
            /* Re-insert subsequent entries that may have probed past this slot */
            uint32_t next_h = (h + 1) & (GC_HASH_SIZE - 1);
            while (c->hash_table[next_h] != 0) {
                uint32_t reinsert_idx = c->hash_table[next_h] - 1;
                c->hash_table[next_h] = 0;
                uint32_t nh = hash_key(c->entries[reinsert_idx].key);
                while (c->hash_table[nh] != 0) nh = (nh + 1) & (GC_HASH_SIZE - 1);
                c->hash_table[nh] = reinsert_idx + 1;
                next_h = (next_h + 1) & (GC_HASH_SIZE - 1);
            }
            return;
        }
        h = (h + 1) & (GC_HASH_SIZE - 1);
    }
}

/* Eviction: remove LRU head until enough arena space is free.
 * Note: we don't compact the arena (would invalidate pointers).
 * Instead, we track a free-list of evicted-bitmap regions. For
 * simplicity in v1, we do whole-arena reset when fragmentation
 * exceeds 50% — pragmatic, accepts cost of re-rasterizing surviving
 * glyphs on next access. Document this tradeoff.
 */
static void evict_until_room(amiport_glyph_cache_t *c, size_t needed_bytes)
{
    /* Simple v1 strategy: evict everything when room runs out.
     * Subsequent lookups will repopulate from FreeType. Trades cache
     * thrashing under pressure for implementation simplicity. */
    if (c->arena_used + needed_bytes > c->arena_bytes) {
        /* Walk LRU list, evict everything */
        uint32_t i = c->lru_head;
        while (i != GC_NULL_IDX) {
            gc_entry_t *e = &c->entries[i];
            hash_remove(c, e->key);
            c->eviction_count++;
            i = e->lru_next;
        }
        c->arena_used = 0;
        c->entry_count = 0;
        c->lru_head = c->lru_tail = GC_NULL_IDX;
    }
}
```

Update `insert()` to call `evict_until_room()` before bump-alloc, and `lru_push_tail()` after inserting.

Update `lookup()` to call `lru_unlink()` then `lru_push_tail()` on hit (promote to MRU).

Initialize `lru_head` and `lru_tail` to `GC_NULL_IDX` in `create()`.

- [ ] **Step 4: Build and run tests — expect PASS**

```bash
make -C lib/glyph-cache && make -C tests/glyph-cache run
```

Expected: all 6 tests PASS. The whole-arena eviction strategy is simple but correct for v1; tune later if perf data warrants.

- [ ] **Step 5: Document the v1 eviction tradeoff**

Add to `lib/glyph-cache/README.md` (create new):

```markdown
# lib/glyph-cache

Generic LRU glyph cache for AmigaOS text-rendering ports.

## v1 implementation notes

**Eviction policy:** when the bump-allocated arena fills, the cache
is wholesale reset rather than compacted. Subsequent lookups will
miss and repopulate from the renderer (FreeType, bullet, etc.).

This trades cache thrashing under pressure for implementation
simplicity. The LRU ordering is maintained even though it doesn't
drive partial eviction in v1 — it's there to guide a future
compacting evictor if perf data shows it matters.

**Recommended arena size:** 256 KB to 2 MB depending on text load.
NetSurf body text on Wikipedia uses ~80 unique glyphs per page;
at 16 px x 16 px x 1 byte alpha = 256 bytes per glyph = ~20 KB
working set. 256 KB gives ~10x headroom across a typical
browsing session.
```

- [ ] **Step 6: Commit**

```bash
git add lib/glyph-cache/src/glyph_cache.c tests/glyph-cache/test_glyph_cache.c lib/glyph-cache/README.md
git commit -m "lib(glyph-cache): LRU + arena-reset eviction (simple v1)"
```

---

### Task 13: Integrate lib/glyph-cache into top-level Makefile

**Files:**
- Modify: `Makefile` (top level)

- [ ] **Step 1: Add glyph-cache build/test targets**

```bash
grep -n "build-shim\|test-shim" Makefile | head -5
```

Add adjacent targets for glyph-cache:

```makefile
build-glyph-cache: ## Build the glyph cache library
	$(MAKE) -C lib/glyph-cache

test-glyph-cache: build-glyph-cache ## Run glyph cache tests via vamos
	$(MAKE) -C tests/glyph-cache run

clean-glyph-cache:
	$(MAKE) -C lib/glyph-cache clean
	$(MAKE) -C tests/glyph-cache clean
```

Add `build-glyph-cache` and `test-glyph-cache` to `.PHONY` line. Add to `make help` output.

- [ ] **Step 2: Verify**

```bash
make help | grep glyph-cache
make build-glyph-cache
make test-glyph-cache
```

Expected: all green.

- [ ] **Step 3: Update CLAUDE.md codebase map**

Add a line under the codebase map for `lib/glyph-cache/` describing it as: "Generic LRU glyph cache for text-rendering ports. ~8 KB. Built by NetSurf Phase 1 spec."

- [ ] **Step 4: Run check-docs**

```bash
make check-docs
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add Makefile CLAUDE.md
git commit -m "build: wire lib/glyph-cache into top-level make targets"
```

---

## Phase D — NetSurf-MUI port skeleton (no font changes yet)

### Task 14: Clone NetSurf-MUI into ports/netsurf/original/

**Files:**
- Create: `ports/netsurf/original/` (NetSurf-MUI source tree)
- Create: `ports/netsurf/.gitignore`

- [ ] **Step 1: Create the port directory**

```bash
mkdir -p ports/netsurf
```

- [ ] **Step 2: Clone NetSurf-MUI as the upstream**

```bash
cd ports/netsurf && git clone --depth 1 https://github.com/arczi84/NetSurf-3.11-MUI.git original
cd original && git rev-parse HEAD > ../UPSTREAM_COMMIT.txt
cd ../../.. # back to project root
```

Capture the upstream commit SHA for the PORT.md.

- [ ] **Step 3: Add .gitignore**

```bash
cat > ports/netsurf/.gitignore <<'EOF'
# Build artifacts inside the upstream tree
original/build/
original/*.o
original/*.a
original/*.lha
EOF
```

- [ ] **Step 4: Make original/ read-only per amiport convention**

Per `.claude/rules/port-directory-hygiene.md`: `original/` is read-only after import. The `block-original-edits.sh` hook will enforce this. Don't strip the .git directory (we'll need it for upstream pulls), but treat the contents as immutable.

- [ ] **Step 5: Inspect what we got**

```bash
ls ports/netsurf/original/frontends/amiga/ | head -20
grep -l "AMMX\|jsimd_ammx\|V_EnableAMMX" ports/netsurf/original/frontends/amiga/ 2>/dev/null
```

Confirm the AMMX-relevant files are present: `jsimd_ammx.c`, `jdcolor-ammx.asm`, `jdmerge-ammx.asm`, `jidctfst-ammx.asm`, `jmemset-ammx.asm` (or however the MUI fork names them).

- [ ] **Step 6: Read the AMMX precedent files**

```bash
cat ports/netsurf/original/frontends/amiga/jsimd_ammx.c | head -100
```

Capture into `ports/netsurf/.claude/agent-memory/jsimd-ammx-pattern.md`:
- How `V_EnableAMMX(V_AMMX_V2)` is called (where in startup, what gates it)
- How extern declarations of asm-defined functions look
- How the Makefile invokes vasm
- Any error-handling patterns we should mirror in `font_freetype.c`

- [ ] **Step 7: Commit**

```bash
git add ports/netsurf/.gitignore ports/netsurf/original/ ports/netsurf/UPSTREAM_COMMIT.txt
# This will be a LARGE commit (entire NetSurf source). That's fine — amiport convention.
git commit -m "feat(netsurf): import NetSurf-MUI 3.11 upstream source

Vendored from arczi84/NetSurf-3.11-MUI commit <SHA>. This is the
working AMMX-precedent fork. See spec docs/superpowers/specs/2026-05-02-
netsurf-vampire-text-rendering-design.md for the rationale."
```

Replace `<SHA>` with the actual commit from `UPSTREAM_COMMIT.txt`.

---

### Task 15: Write ports/netsurf/Makefile (initial — no font changes)

**Files:**
- Create: `ports/netsurf/Makefile`

- [ ] **Step 1: Read the existing port Makefile templates**

```bash
cat ports/templates/Makefile.template
cat ports/amigit/Makefile  # nearest-precedent: complex C++ port that links libgit2 etc.
```

Note the conventions: `VERSION`, `REVISION`, `DESCRIPTION`, common.mk include, `include ../common.mk`.

- [ ] **Step 2: Write ports/netsurf/Makefile**

```makefile
# ports/netsurf/Makefile
# NetSurf 3.11 MUI fork — Phase 1 (FreeType + AMMX text rendering)

VERSION  = 3.11
REVISION = 1
DESCRIPTION = NetSurf web browser for Vampire 68080
include ../common.mk

# Vampire-native: hardware-required (no scalar fallback)
# Needs Apollo 68080 / V4 / A6000.
CFLAGS  += -m68040 -m68881 -O0 -noixemul -std=gnu99
CXXFLAGS += -m68040 -m68881 -O0 -noixemul -std=gnu++17

# Link against amiport libs
LDFLAGS += -L../../lib/posix-shim -lamiport
LDFLAGS += -L../../lib/glyph-cache -lglyphcache
LDFLAGS += -L../../lib/freetype -lfreetype

# Vampire SDK + ammx header
CFLAGS  += -I../../lib/vampire-sdk/include
CFLAGS  += -I../../lib/posix-shim/include
CFLAGS  += -I../../lib/freetype/include

# vasm — Phase E adds .asm files; Phase D builds without them
VASM = vasmm68k_mot
VASMFLAGS = -m68080 -Fhunk -quiet

# Use the vasm-equipped Docker image
DOCKER_IMAGE = ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b

# Phase D goal: get the upstream build working unchanged.
# We don't touch font_*.c yet — just confirm the toolchain compiles
# the existing tree.
all:
	$(MAKE) -C original/ TARGET=amiga68k

clean:
	$(MAKE) -C original/ clean

package: all
	@echo "TODO Phase H: package netsurf-$(DISPLAY_VERSION).lha"

install-emu: all
	@echo "TODO Phase D Step 5: copy netsurf binary to build/amiga/"

.PHONY: all clean package install-emu
```

The `all:` target is provisional — it relies on NetSurf's own Makefile. The Makefile is well-engineered upstream but may need patches; we'll discover those in Step 3.

- [ ] **Step 3: Attempt the build (expect issues)**

```bash
make -C ports/netsurf 2>&1 | tee /tmp/netsurf-build-1.log | tail -50
```

Expected: SOMETHING fails. Common issues:
- NetSurf's Makefile expects different compiler flags (e.g., `-m68020` rather than `-m68040`)
- Missing `pkg-config` in the Docker image
- libcurl, libxml2, etc. not present (though MUI fork should bundle these)
- `__amigaos4__` macro accidentally defined

Read the log carefully. For each error, decide: (a) patch upstream Makefile in `original/` (NO — read-only; instead, pass override via environment), (b) add a missing tool to Docker image, (c) add a wrapper Makefile rule.

- [ ] **Step 4: Iterate on the build until it produces a binary**

This may take many sub-iterations. For each:
1. Identify the specific error
2. Fix it (Docker image patch, environment override, additional CFLAGS)
3. Re-run `make -C ports/netsurf`
4. Repeat until you have an `original/build/amiga/netsurf` binary (or wherever NetSurf-MUI puts its output)

If you hit a blocker that requires modifying upstream source, dispatch the `build-manager` agent rather than poking at `original/` directly. Per project rules, upstream source is read-only.

- [ ] **Step 5: Capture build issues as learnings**

For each non-trivial issue resolved, invoke `/capture-learning` with the topic and fix. Issues likely worth capturing:
- "NetSurf-MUI requires specific bebbo-gcc 13.3 patches for ___"
- "vasm 1.8b -m68080 emits ___ vs vasm 1.x"
- Any new pitfall in the toolchain interaction

- [ ] **Step 6: Verify the binary runs unchanged on FS-UAE**

```bash
make install-emu TARGET=ports/netsurf
make emu  # launch FS-UAE
# Inside the Amiga shell: WORK:netsurf
```

Expected: NetSurf launches, displays the MUI UI, can navigate to a local file. Body text is JAGGED (we haven't touched fonts yet — this is the baseline we're improving).

Take a screenshot. Save to `ports/netsurf/.claude/agent-memory/baseline-jagged.png`.

- [ ] **Step 7: Commit**

```bash
git add ports/netsurf/Makefile <any Docker image fixes> <any /capture-learning artifacts>
git commit -m "feat(netsurf): build NetSurf-MUI 3.11 unchanged on bebbo-gcc 13.3 + vasm

Phase D milestone: original tree builds clean. Body text still uses
bullet.library (jagged on AmigaOS 3) — Phase F adds font_freetype.c.
Baseline screenshot in .claude/agent-memory/baseline-jagged.png."
```

---

### Task 16: Write initial ports/netsurf/PORT.md and netsurf.readme

**Files:**
- Create: `ports/netsurf/PORT.md`
- Create: `ports/netsurf/netsurf.readme`

- [ ] **Step 1: Write PORT.md from template**

```bash
cat ports/templates/PORT.md.template
```

Adapt for NetSurf:

```markdown
# NetSurf 3.11 (Vampire-native) — Port Notes

| Field | Value |
|---|---|
| Version | 3.11 |
| Revision | 1 |
| Upstream | https://github.com/arczi84/NetSurf-3.11-MUI |
| Upstream commit | <from UPSTREAM_COMMIT.txt> |
| License | GPL v2 (NetSurf), MPL 2.0 (Vampire SDK) |
| Category | Network — web browser |
| Hardware required | Apollo 68080 (Vampire V4 / A6000) |
| Last update | 2026-05-02 |

## Status

Phase D milestone: upstream source builds unchanged on bebbo-gcc 13.3
+ vasm 1.8b. Body text uses bullet.library (jagged AA on AmigaOS 3).

Phase F (planned) adds font_freetype.c with FreeType-rendered AA glyphs
and AMMX2 alpha compositor. See spec at
`docs/superpowers/specs/2026-05-02-netsurf-vampire-text-rendering-design.md`.

## Build

`make -C ports/netsurf` — produces `original/build/amiga/netsurf` (or
wherever upstream emits the binary; verify path).

## Hardware requirements

- Apollo 68080 (Vampire V2/V4) with vampire.resource >= V45
- 32 MB Fast RAM minimum (recommended 64 MB)
- Picasso96 RTG display (AGA NOT supported)
- TruType fonts in FONTS:Truetype/ (Phase F+)

The binary refuses to launch on stock 68040/060 with a clear error
message. Use mainline NetSurf 68k or arczi84/NetSurf-68k for non-Apollo
systems.

## Test results

Phase D: TBD pending Phase H CI integration.

## Known limitations

- No JavaScript (Phase 3 — Duktape integration deferred)
- No HTTP/2 (out of scope per spec)
- No WebP / AVIF (out of scope per spec)

## Porting transformations

Phase D: none — upstream tree builds unchanged.
Phase F: adds `frontends/amiga/font_freetype.c` and patches `font.c`.

## Memory safety

(After Phase F:) memory-checker audit pending.

## Performance notes

(After Phase G hardware test:) ReadEClock-instrumented per-glyph cycle
counts pending.
```

- [ ] **Step 2: Write netsurf.readme (Aminet-format, must be ASCII, <40-char Short)**

```
Short:        NetSurf web browser for Vampire 68080
Author:       NetSurf Developers + Duncan Bowring (port)
Uploader:     Duncan Bowring (amiport at platesteel net)
Type:         comm/www
Version:      3.11-1
Architecture: m68k-amigaos
Replaces:     (none — first upload)

NetSurf 3.11 ported to AmigaOS 3.x with Vampire 68080 / Apollo
Core acceleration. Phase 1 of an ongoing improvement effort —
this revision builds the upstream MUI fork unchanged. Anti-
aliased text rendering via FreeType + AMMX2 ships in Phase 2.

Hardware requirements:
  - Apollo 68080 (Vampire V2/V4 or Apollo Standalone A6000)
  - vampire.resource V45 or later
  - 32 MB Fast RAM minimum
  - Picasso96 RTG display

Refuses to launch on stock 68k systems. Use mainline NetSurf
68k or arczi84/NetSurf-68k for non-Apollo Amigas.

Source: https://github.com/arczi84/NetSurf-3.11-MUI
License: GPL v2
```

- [ ] **Step 3: Validate the .readme against amiport rules**

```bash
make check-port-metadata 2>&1 | grep -A2 netsurf
```

Expected: PASS. If FAIL on Short length (max 40 chars) or non-ASCII, fix and re-run.

- [ ] **Step 4: Commit**

```bash
git add ports/netsurf/PORT.md ports/netsurf/netsurf.readme
git commit -m "docs(netsurf): initial PORT.md and Aminet readme"
```

---

## Phase E — AMMX glyph compositor kernel

### Task 17: Study the jsimd_ammx pattern from MUI

**Files:**
- Create: `ports/netsurf/.claude/agent-memory/ammx-pattern-notes.md`

- [ ] **Step 1: Read jsimd_ammx.c thoroughly**

```bash
cat ports/netsurf/original/frontends/amiga/jsimd_ammx.c
```

Note:
- How `V_EnableAMMX(V_AMMX_V2)` is gated (one-shot init vs per-call)
- How extern declarations of asm-defined symbols are written
- How the C-side dispatch decides "use AMMX" vs "skip"

- [ ] **Step 2: Read jdcolor-ammx.asm (or similar) for assembly conventions**

```bash
cat ports/netsurf/original/frontends/amiga/jdcolor-ammx.asm | head -100
```

Note:
- How vasm preamble is structured (sections, EXPORT directives)
- How C calling convention parameters are accessed (stack-based on m68k SysV)
- Which AMMX2 instructions are used and their syntax in vasm
- Register save/restore conventions

- [ ] **Step 3: Read the AMMX section of known-pitfalls.md**

```bash
sed -n '/^## Apollo 68080 AMMX/,/^## /p' .claude/rules/known-pitfalls.md | head -300
```

This is the canonical reference for our font kernel. The relevant primitives:
- `PCMP src vs 0 → mask` (per-byte compare, mask in another register)
- `PMULA dst, alpha → blended bytes` (per-byte alpha multiply)
- `STOREm3 #1 → write only non-zero` (V4-only, the key write-without-read primitive)

- [ ] **Step 4: Write notes file**

```bash
cat > ports/netsurf/.claude/agent-memory/ammx-pattern-notes.md <<'EOF'
# AMMX integration pattern (cribbed from jsimd_ammx.c)

## Initialization
- Call `V_EnableAMMX(V_AMMX_V2)` ONCE at startup — we use
  `amiport_ammx_init()` from `lib/posix-shim` instead.
- Before any AMMX kernel runs, init MUST have returned 0.

## Calling convention
- vasm `.asm` files declare functions with `xdef _funcname`
- C calls them with leading underscore stripped (linker adds it)
- Args passed via stack (m68k SysV); first arg at 4(sp), second at 8(sp), etc.
- Return value in d0
- Caller saves d0/d1/a0/a1; callee saves everything else

## AMMX kernel template (placeholder for actual font compositor)
;
;   xdef _font_compose_glyph_argb32_ammx
;_font_compose_glyph_argb32_ammx:
;       movem.l d2-d7/a2-a6,-(sp)
;       move.l  4+44(sp),a0     ; dst (ARGB framebuffer pointer)
;       move.l  8+44(sp),d0     ; dst_stride
;       move.l  12+44(sp),a1    ; alpha bitmap
;       move.l  16+44(sp),d1    ; alpha_stride
;       move.l  20+44(sp),d2    ; w
;       move.l  24+44(sp),d3    ; h
;       move.l  28+44(sp),d4    ; color (ARGB)
;
;       ; ... AMMX2 inner loop ...
;
;       movem.l (sp)+,d2-d7/a2-a6
;       rts

## Useful jsimd_ammx.c idioms to mirror
- (capture specific code patterns observed)
EOF
```

Replace the `(capture specific code patterns observed)` line with actual observations from jsimd_ammx.c.

- [ ] **Step 5: Commit**

```bash
git add ports/netsurf/.claude/agent-memory/ammx-pattern-notes.md
git commit -m "docs(netsurf): capture AMMX integration pattern from jsimd_ammx.c"
```

---

### Task 18: Write font_freetype_ammx.asm — the AMMX2 glyph compositor kernel

**Files:**
- Create: `ports/netsurf/ported/frontends/amiga/font_freetype_ammx.asm`

- [ ] **Step 1: Write the asm skeleton**

```asm
; font_freetype_ammx.asm — Apollo AMMX2 alpha glyph compositor for NetSurf.
; Composites an 8-bit alpha bitmap onto an ARGB32 framebuffer.
;
; void font_compose_glyph_argb32_ammx(
;     uint32_t *dst,           ; ARGB32 framebuffer at glyph origin
;     int32_t   dst_stride,    ; bytes per row in dst
;     const uint8_t *alpha,    ; 8-bit alpha glyph bitmap
;     int32_t   alpha_stride,  ; bytes per row in alpha
;     int32_t   width,
;     int32_t   height,
;     uint32_t  color);        ; ARGB foreground color
;
; Algorithm (per row):
;   for x in 0..width step 8:
;       load 8 alpha bytes from src
;       PCMP src, #0 -> mask (which output bytes to skip)
;       expand 8-bit alpha to 4x32 ARGB blend factors
;       PMULA dst, alpha -> blended pixels
;       STOREm3 #1 dst -> write only where alpha != 0 (V4 fast path)
;
; Hardware requirement: Apollo 68080 with AMMX2 enabled via
; amiport_ammx_init(). Calling this without successful init will
; Line-F trap.

    machine 68080

    section text,code

    xdef    _font_compose_glyph_argb32_ammx

_font_compose_glyph_argb32_ammx:
    movem.l d2-d7/a2-a6,-(sp)

    ; Load arguments. Stack layout after movem (44 bytes saved):
    ;   4+44(sp) = dst
    ;   8+44(sp) = dst_stride
    ;  12+44(sp) = alpha
    ;  16+44(sp) = alpha_stride
    ;  20+44(sp) = width
    ;  24+44(sp) = height
    ;  28+44(sp) = color (ARGB packed)
    move.l  4+44(sp),a0     ; a0 = dst row base
    move.l  8+44(sp),d4     ; d4 = dst_stride
    move.l  12+44(sp),a1    ; a1 = alpha row base
    move.l  16+44(sp),d5    ; d5 = alpha_stride
    move.l  20+44(sp),d6    ; d6 = width (pixels)
    move.l  24+44(sp),d7    ; d7 = height (rows remaining)
    move.l  28+44(sp),d3    ; d3 = ARGB color

    tst.l   d6
    beq     .done           ; width == 0
    tst.l   d7
    beq     .done           ; height == 0

.row_loop:
    move.l  a0,a2           ; a2 = dst x cursor for this row
    move.l  a1,a3           ; a3 = alpha x cursor for this row
    move.l  d6,d2           ; d2 = pixels remaining in this row

.pixel_loop:
    ; --- INNER LOOP STUB ---
    ;
    ; The actual AMMX2 8-pixel-per-iteration inner loop goes here.
    ; Reference: arczi84/NetSurf-MUI's jdcolor-ammx.asm for AMMX
    ; instruction encoding patterns. Reference: known-pitfalls.md
    ; AMMX section for the V4 PCMP+STOREm3 idiom.
    ;
    ; Pseudocode:
    ;   load   (a3)+,d0              ; 8 alpha bytes
    ;   pcmp   #0,d0,d1              ; d1 = $ff per byte where d0 != 0
    ;   ; expand alpha to per-pixel ARGB blend (PMULA over color)
    ;   pmula  d0,d3,e0              ; per-byte multiply alpha by color
    ;   storem3 e0,#1,(a2)           ; write only where alpha != 0
    ;   addq.l #32,a2                ; advance dst by 8 pixels (32 bytes)
    ;   subq.l #8,d2                 ; advance pixels-remaining
    ;   bgt.s  .pixel_loop
    ;
    ; SCALAR FALLBACK (pre-AMMX, for development) — write this first,
    ; verify correctness on A6000 (V4 will execute m68k scalar code
    ; just fine), then replace with AMMX2 once the C glue + dispatch
    ; works end-to-end.

    ; SCALAR PLACEHOLDER (replace with AMMX2 inner loop):
    moveq   #0,d0
    move.b  (a3)+,d0        ; d0 = alpha
    tst.b   d0
    beq.s   .skip_pixel
    ; Naive blend: dst = (color * alpha + dst * (255 - alpha) + 128) >> 8
    ; (per-channel; expand color to RGB via shifts, blend each channel)
    ; ... C-equivalent inline asm ...
    move.l  d3,(a2)         ; oversimplified: just write color (no blend)
.skip_pixel:
    addq.l  #4,a2
    subq.l  #1,d2
    bgt.s   .pixel_loop

    add.l   d4,a0           ; advance dst by stride
    add.l   d5,a1           ; advance alpha by stride
    subq.l  #1,d7
    bgt     .row_loop

.done:
    movem.l (sp)+,d2-d7/a2-a6
    rts

    end
```

This is a SCAFFOLD with the scalar placeholder — it produces visible (but ugly, no actual blending) output. The AMMX2 inner loop replaces the SCALAR PLACEHOLDER block in Step 4 below.

- [ ] **Step 2: Add vasm rule to ports/netsurf/Makefile**

Add to the Makefile:

```makefile
# Build AMMX kernel via vasm
%.o: %.asm
	$(VASM) $(VASMFLAGS) -o $@ $<

# Phase E adds the kernel object to the link
AMMX_OBJS = ported/frontends/amiga/font_freetype_ammx.o
```

We don't link `AMMX_OBJS` into the netsurf binary yet — that happens in Phase F when we wire `font_freetype.c`. For now we just want vasm to assemble the file.

- [ ] **Step 3: Assemble the kernel**

```bash
docker run --rm -v $(pwd):/work ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b \
  vasmm68k_mot -m68080 -Fhunk -quiet \
  -o /work/ports/netsurf/ported/frontends/amiga/font_freetype_ammx.o \
  /work/ports/netsurf/ported/frontends/amiga/font_freetype_ammx.asm
```

Expected: assembles clean. If syntax errors, fix per vasm error message.

- [ ] **Step 4: Replace scalar placeholder with AMMX2 inner loop**

This is the substantive AMMX work. Use:
- `arczi84/NetSurf-MUI` `jdcolor-ammx.asm` for vasm syntax precedent
- `.claude/rules/known-pitfalls.md` AMMX section — `STOREilm` is the V2-compatible primitive (3-instruction inner loop), `STOREm3 #1` is the V4-only fast path (2-instruction). Per spec we target V4 only, so use `STOREm3 #1`.

The AMMX inner loop (replacing the SCALAR PLACEHOLDER block) should look approximately like:

```asm
.pixel_loop:
    load    (a3)+,d0            ; 8 alpha bytes from src
    ; expand alpha bytes to per-pixel scale factors (PMULA prep)
    ; ... per-byte alpha multiply with color (PMULA d0,d3,e0)
    pmula   d3,d0,e0             ; e0 = (color.bytes * alpha) >> 8 + 0  -- per-byte
    ; storem3 #1 writes only bytes where d0 != 0 (skip transparent)
    storem3 e0,#1,(a2)
    addq.l  #32,a2                ; 8 pixels x 4 bytes = 32 bytes
    subq.l  #8,d2
    bgt.s   .pixel_loop
```

The exact PMULA / STOREm3 encoding requires cross-checking with the Apollo PRM (which has been ECONNREFUSED in prior sessions). If apollo-core.com is unreachable when you implement, fall back to vasm's instruction reference (`vasmm68k_mot -? | grep -i ammx`) or the AMMX section of `known-pitfalls.md` which has worked-out examples.

After replacing, re-assemble (Step 3 command). If vasm rejects the AMMX2 instructions, you may need to ensure `machine 68080` is set at the top (already done) and that vasm 1.8b supports the specific opcode (it should — added in 1.8 per release notes).

- [ ] **Step 5: Commit (the ASM file with placeholder, before AMMX inner loop)**

If Step 4 succeeds, commit the AMMX2 version. If you can't get AMMX2 working in this task (vasm encoding issues, etc.), commit the scalar placeholder and add a TODO that Phase F's hardware test will identify visual artifacts (won't blend, will just slam color over dst).

```bash
git add ports/netsurf/ported/frontends/amiga/font_freetype_ammx.asm \
        ports/netsurf/ported/frontends/amiga/font_freetype_ammx.o \
        ports/netsurf/Makefile
git commit -m "feat(netsurf): font_freetype_ammx.asm — AMMX2 glyph compositor kernel"
```

---

### Task 19: Standalone test driver for the AMMX kernel (A6000 hardware test)

**Files:**
- Create: `ports/netsurf/test/test_ammx_compose.c`
- Create: `ports/netsurf/test/Makefile`

The kernel is meaningless without verifying it actually composites correctly on A6000. This task builds a tiny standalone program that fills a small ARGB buffer with a synthetic alpha bitmap and the kernel, then dumps the result for eyeball verification.

- [ ] **Step 1: Write test_ammx_compose.c**

```c
/* test_ammx_compose.c — standalone test for font_compose_glyph_argb32_ammx.
 * Allocates a small ARGB framebuffer, fills it with background color,
 * composites a synthetic gradient alpha bitmap with foreground color,
 * dumps the result as a tiny PPM file for inspection.
 *
 * Run on A6000:
 *   amiport_ammx_init(); test_ammx_compose
 *   inspect /tmp/ammx-test.ppm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <amiport/ammx.h>

extern void font_compose_glyph_argb32_ammx(
    unsigned int *dst, int dst_stride,
    const unsigned char *alpha, int alpha_stride,
    int width, int height, unsigned int color);

#define W 32
#define H 16

long __stack = 65536;

int main(void)
{
    unsigned int *fb;
    unsigned char *alpha;
    int rc, x, y;
    FILE *out;

    rc = amiport_ammx_init();
    printf("amiport_ammx_init() = %d\n", rc);
    if (rc != 0) {
        printf("ABORT: AMMX2 not available; this test is hardware-required.\n");
        return 10;
    }

    fb = malloc(W * H * sizeof(unsigned int));
    alpha = malloc(W * H);
    if (!fb || !alpha) { printf("OOM\n"); return 10; }

    /* Fill fb with white background (0xFFFFFFFF) */
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
            fb[y*W + x] = 0xFFFFFFFF;

    /* Synthetic alpha: vertical gradient 0..255 across width */
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
            alpha[y*W + x] = (unsigned char)((x * 255) / (W-1));

    /* Composite foreground = pure red (0xFFFF0000 ARGB) */
    font_compose_glyph_argb32_ammx(fb, W * sizeof(unsigned int),
                                    alpha, W,
                                    W, H, 0xFFFF0000);

    /* Dump as PPM (skip alpha channel) */
    out = fopen("T:ammx-test.ppm", "w");
    if (!out) { printf("Cannot write T:ammx-test.ppm\n"); return 10; }
    fprintf(out, "P3\n%d %d\n255\n", W, H);
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            unsigned int p = fb[y*W + x];
            fprintf(out, "%d %d %d ", (p>>16)&0xFF, (p>>8)&0xFF, p&0xFF);
        }
        fprintf(out, "\n");
    }
    fclose(out);

    free(fb); free(alpha);
    printf("Wrote T:ammx-test.ppm (%d x %d)\n", W, H);
    printf("Expected: gradient from white (left) to red (right)\n");
    return 0;
}
```

- [ ] **Step 2: Write test/Makefile**

```makefile
TOOLCHAIN_BIN = ../../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
VASM = vasmm68k_mot

CFLAGS = -O0 -noixemul -m68040 -m68881 -std=gnu99 -Wall \
         -I../../../lib/posix-shim/include \
         -I../../../lib/vampire-sdk/include
LDFLAGS = -L../../../lib/posix-shim -lamiport

test_ammx_compose: test_ammx_compose.c font_freetype_ammx.o
	$(CC) $(CFLAGS) test_ammx_compose.c font_freetype_ammx.o -o $@ $(LDFLAGS)

font_freetype_ammx.o: ../ported/frontends/amiga/font_freetype_ammx.asm
	$(VASM) -m68080 -Fhunk -quiet -o $@ $<

clean:
	rm -f test_ammx_compose font_freetype_ammx.o

.PHONY: clean
```

- [ ] **Step 3: Build the test**

```bash
docker run --rm -v $(pwd):/work -w /work/ports/netsurf/test \
  ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b make
```

Expected: builds clean.

- [ ] **Step 4: Push to A6000 and run**

```bash
python3 -m amigactl --host 192.168.1.215 put ports/netsurf/test/test_ammx_compose T:test_ammx_compose
python3 -m amigactl --host 192.168.1.215 exec "T:test_ammx_compose"
python3 -m amigactl --host 192.168.1.215 get T:ammx-test.ppm /tmp/ammx-test.ppm
```

- [ ] **Step 5: Inspect the PPM file**

```bash
# Convert to PNG for easier viewing
docker run --rm -v /tmp:/tmp dpokidov/imagemagick \
  convert /tmp/ammx-test.ppm /tmp/ammx-test.png

open /tmp/ammx-test.png  # macOS
```

Expected: a 32x16 image showing gradient from white (left) to red (right). Each pixel correctly blended per its alpha value.

- [ ] **Step 6: If it doesn't look right, debug**

Common issues:
- Color is wrong (wrong byte order — ARGB vs BGRA): check Picasso96 native pixel format. Adjust kernel byte ordering.
- No blending happens (just slam color): the AMMX inner loop wasn't actually written / placeholder still in place. Revisit Task 18 Step 4.
- Crashes / Guru: AMMX2 instruction not understood (vasm version too old? `machine 68080` directive missing?). Check vasm version with `vasmm68k_mot -?`.
- Wrong width/height in output: stride math off in inner loop.

Capture any new pitfall via `/capture-learning`.

- [ ] **Step 7: Commit (test driver + verified-working kernel)**

```bash
git add ports/netsurf/test/
git commit -m "test(netsurf): standalone AMMX glyph compositor verification on A6000

Composites red-on-white gradient via font_compose_glyph_argb32_ammx,
dumps to T:ammx-test.ppm. Verified correct on A6000."
```

---

## Phase F — `font_freetype.c` + dispatch wiring

### Task 20: Write font_freetype.h (declarations)

**Files:**
- Create: `ports/netsurf/ported/frontends/amiga/font_freetype.h`

- [ ] **Step 1: Write the header**

```c
/* font_freetype.h — declarations for the FreeType+AMMX font backend.
 *
 * Phase 1: anti-aliased text rendering via FreeType for AmigaOS 3 /
 * Vampire 68080. Composites alpha glyphs via AMMX2 kernel.
 *
 * Routes: nsoption_bool(freetype_fonts) is true (default true on
 * AmigaOS 3, false on AmigaOS 4) -> font.c dispatches to here.
 */

#ifndef NETSURF_AMIGA_FONT_FREETYPE_H
#define NETSURF_AMIGA_FONT_FREETYPE_H

#include "netsurf/plot_style.h"
#include "netsurf/font.h"

extern struct gui_font_table *ami_font_freetype_table;

/* Initialize FreeType + glyph cache + AMMX. Returns false on failure. */
bool ami_font_freetype_init(void);
void ami_font_freetype_fini(void);

#endif
```

- [ ] **Step 2: Commit**

```bash
git add ports/netsurf/ported/frontends/amiga/font_freetype.h
git commit -m "feat(netsurf): font_freetype.h declarations"
```

---

### Task 21: Write font_freetype.c — init/fini, face cache

**Files:**
- Create: `ports/netsurf/ported/frontends/amiga/font_freetype.c`

- [ ] **Step 1: Write the file with init/fini + FreeType face cache (no glyph cache or AMMX yet)**

Reference `ports/netsurf/original/frontends/amiga/font_bullet.c` for what NetSurf's `gui_font_table` looks like and what each function must do.

```c
/* font_freetype.c — FreeType-backed font backend for AmigaOS 3 / Vampire.
 *
 * Hot path: per-character text() → glyph_cache lookup → on miss
 * FT_Load_Char + cache insert → AMMX glyph compositor.
 *
 * Init checks amiport_ammx_init() — if AMMX2 not available, init
 * fails and font.c falls back to font_bullet (the pre-existing
 * bitmap path). This preserves correctness for users without a
 * Vampire while the rest of the binary's "Vampire-required" check
 * still triggers in NetSurf init.
 */

#include "amiga/os3support.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "amiga/font_freetype.h"
#include "amiga/font.h"
#include "utils/log.h"
#include "utils/nsoption.h"
#include "netsurf/types.h"
#include "netsurf/font.h"

#include <amiport/ammx.h>
#include <amiport/glyph_cache.h>
#include <ft2build.h>
#include FT_FREETYPE_H

extern void font_compose_glyph_argb32_ammx(
    uint32_t *dst, int dst_stride,
    const uint8_t *alpha, int alpha_stride,
    int width, int height, uint32_t color);

static FT_Library g_ft_library;
static amiport_glyph_cache_t *g_glyph_cache;
static bool g_initialized = false;

/* Face cache — small hash mapping (family_hash, weight, italic) to FT_Face */
#define FACE_CACHE_SIZE 16
typedef struct { uint32_t key; FT_Face face; uint16_t px_size; } face_entry_t;
static face_entry_t g_face_cache[FACE_CACHE_SIZE];

/* TODO Step 4: write width/position/split/text + glyph cache lookup */

bool ami_font_freetype_init(void)
{
    int rc;

    if (g_initialized) return true;

    rc = amiport_ammx_init();
    if (rc != 0) {
        NSLOG(netsurf, ERROR,
              "amiport_ammx_init failed (rc=%d). FreeType backend disabled; "
              "falling back to font_bullet.", rc);
        return false;
    }

    if (FT_Init_FreeType(&g_ft_library) != 0) {
        NSLOG(netsurf, ERROR, "FT_Init_FreeType failed");
        return false;
    }

    g_glyph_cache = amiport_glyph_cache_create(512 * 1024);
    if (!g_glyph_cache) {
        NSLOG(netsurf, ERROR, "glyph cache allocation failed");
        FT_Done_FreeType(g_ft_library);
        return false;
    }

    memset(g_face_cache, 0, sizeof(g_face_cache));
    g_initialized = true;
    NSLOG(netsurf, INFO, "ami_font_freetype_init: ready (AMMX2 enabled)");
    return true;
}

void ami_font_freetype_fini(void)
{
    int i;
    if (!g_initialized) return;
    for (i = 0; i < FACE_CACHE_SIZE; i++)
        if (g_face_cache[i].face) FT_Done_Face(g_face_cache[i].face);
    amiport_glyph_cache_destroy(g_glyph_cache);
    FT_Done_FreeType(g_ft_library);
    g_initialized = false;
}

/* Stubs for the gui_font_table — implementations in subsequent steps */

static nserror nsfont_width(const struct plot_font_style *fstyle,
                             const char *string, size_t length, int *width)
{
    /* TODO Step 4 */
    *width = (int)length * 8;
    return NSERROR_OK;
}

static nserror nsfont_position(const struct plot_font_style *fstyle,
    const char *string, size_t length, int x,
    size_t *char_offset, int *actual_x)
{
    /* TODO Step 4 */
    *char_offset = length;
    *actual_x = (int)length * 8;
    return NSERROR_OK;
}

static nserror nsfont_split(const struct plot_font_style *fstyle,
    const char *string, size_t length, int x,
    size_t *char_offset, int *actual_x)
{
    /* TODO Step 4 */
    *char_offset = length;
    *actual_x = (int)length * 8;
    return NSERROR_OK;
}

/* The hot path — composites text via FreeType + glyph cache + AMMX */
nserror ami_font_freetype_text(struct RastPort *rp, int x, int y,
    const struct plot_font_style *fstyle,
    const char *string, size_t length)
{
    /* TODO Step 4: implement the hot path */
    (void)rp; (void)x; (void)y; (void)fstyle; (void)string; (void)length;
    return NSERROR_OK;
}

static struct gui_font_table table = {
    .width = nsfont_width,
    .position_in_string = nsfont_position,
    .split = nsfont_split,
    /* text() field name varies by NetSurf version; check struct */
};

struct gui_font_table *ami_font_freetype_table = &table;
```

- [ ] **Step 2: Add font_freetype.o + glyph cache + AMMX kernel to ports/netsurf/Makefile**

Modify the upstream NetSurf Makefile invocation to:
- Add `ported/frontends/amiga/font_freetype.c` to the source list (via env override or wrapper)
- Add `ported/frontends/amiga/font_freetype_ammx.o` (vasm output) to the link
- Link `-lglyphcache -lfreetype` from amiport's libs

The exact mechanism depends on how upstream NetSurf's Makefile is structured. If the upstream Makefile uses `*.c` glob, just dropping `font_freetype.c` into `frontends/amiga/` makes it pick up automatically. The vasm-built `.o` may need a manual addition.

- [ ] **Step 3: Build and confirm linking**

```bash
make -C ports/netsurf 2>&1 | tail -30
```

Expected: NetSurf binary builds, includes `font_freetype.o` and `font_freetype_ammx.o`.

```bash
docker run --rm -v $(pwd):/work ghcr.io/bdgscotland/amiport-toolchain-gcc13:vasm-1.8b \
  m68k-amigaos-nm /work/ports/netsurf/original/build/amiga/netsurf | \
  grep -E "ami_font_freetype|font_compose_glyph_argb32_ammx"
```

Expected: both symbols present in the binary.

- [ ] **Step 4: Implement the hot path (width / position / split / text)**

This is the substantive C work. Each function:

`nsfont_width`: walk codepoints, accumulate `glyph->advance_x_q16 >> 16`. Each codepoint: glyph_cache_lookup → on miss FT_Load_Char + insert.

`nsfont_position`: similar walk, return char offset and pixel x where target_x is reached.

`nsfont_split`: similar walk, return char offset and actual x at last word boundary before target_x.

`ami_font_freetype_text` (hot path):
1. Resolve face from `fstyle` → check face cache → on miss `FT_New_Face(g_ft_library, "FONTS:Truetype/<derived-name>.ttf", 0, &face)` + `FT_Set_Pixel_Sizes(face, 0, fstyle->size / FONT_SIZE_SCALE)` + insert
2. Lock the RastPort BitMap with `LockBitMapTags(rp->BitMap, LBMI_BASEADDRESS, &fb_base, LBMI_BYTESPERROW, &fb_pitch, TAG_END)` to get a direct framebuffer pointer
3. For each codepoint:
   - `glyph_cache_lookup(g_glyph_cache, face_id, cp, px_size, hint_flags, &glyph)` 
   - On miss: `FT_Load_Char(face, cp, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)`, copy `face->glyph->bitmap` into a `amiport_glyph_t`, `glyph_cache_insert(...)` 
   - `font_compose_glyph_argb32_ammx(fb_base + (x+glyph.bearing_x)*4 + (y+glyph.bearing_y)*fb_pitch, fb_pitch, glyph.bitmap, glyph.stride, glyph.width, glyph.height, color_argb)`
   - `x += glyph.advance_x_q16 >> 16`
4. `UnLockBitMap(rp->BitMap, locked_handle)` (or whatever the unlock API is — verify in graphics.library autodocs)

Implementation specifics around RastPort access on Picasso96 may surface unexpected pitfalls — capture each via `/capture-learning`.

- [ ] **Step 5: Build, FS-UAE smoke test**

```bash
make -C ports/netsurf
make install-emu TARGET=ports/netsurf
make emu
# In Amiga shell: WORK:netsurf
```

Expected: NetSurf launches, exits gracefully on FS-UAE because `amiport_ammx_init` returns non-zero (no Apollo). Falls back to font_bullet, body text still bitmap-only — but no crash, no missing symbols.

- [ ] **Step 6: Commit**

```bash
git add ports/netsurf/ported/frontends/amiga/font_freetype.c \
        ports/netsurf/Makefile
git commit -m "feat(netsurf): font_freetype.c hot path with FreeType+glyph cache+AMMX"
```

---

### Task 22: Add nsoption + patch font.c dispatch

**Files:**
- Create: `ports/netsurf/ported/frontends/amiga/font.c` (PATCH copy of original)
- Create: `ports/netsurf/ported/frontends/amiga/gui_options.c` (PATCH copy of original)

We can't edit `original/`. Instead, we copy the upstream files to `ported/`, modify them, and override the upstream Makefile to use the ported versions.

- [ ] **Step 1: Copy font.c and gui_options.c to ported/**

```bash
cp ports/netsurf/original/frontends/amiga/font.c \
   ports/netsurf/ported/frontends/amiga/font.c
cp ports/netsurf/original/frontends/amiga/gui_options.c \
   ports/netsurf/ported/frontends/amiga/gui_options.c
```

- [ ] **Step 2: Patch ported/frontends/amiga/font.c**

Find the existing `nsoption_bool(bitmap_fonts)` dispatch (somewhere in the init logic). Add a third branch:

```c
#include "amiga/font_freetype.h"

void ami_font_setdevicedpi(int id) {
    /* ... existing code ... */

    /* PATCH: Vampire-native FreeType+AMMX path takes precedence */
    if (nsoption_bool(freetype_fonts)) {
        if (ami_font_freetype_init()) {
            ami_nsfont = ami_font_freetype_table;
            return;
        }
        NSLOG(netsurf, WARNING,
              "freetype_fonts requested but init failed; falling back to bullet");
    }

    if (nsoption_bool(bitmap_fonts)) {
        ami_nsfont = ami_font_diskfont_init();
    } else {
        ami_nsfont = ami_font_bullet_init();
    }
}
```

(Adapt to actual function name and structure observed in upstream font.c.)

- [ ] **Step 3: Patch ported/frontends/amiga/gui_options.c**

Add the new option declaration where other `nsoption_bool` declarations live:

```c
NSOPTION_BOOL(freetype_fonts, true)  /* default true on AmigaOS 3 */
```

If there's a conditional `#ifdef __amigaos4__` for OS4-specific options, exclude it from OS4 (default false there).

- [ ] **Step 4: Override upstream Makefile to use ported/ files**

Add to `ports/netsurf/Makefile` to ensure the ported versions are picked up:

```makefile
# Override upstream font.c and gui_options.c with our patched versions
# by copying ported/ into the build directory before invoking upstream make
override-source:
	cp ported/frontends/amiga/font.c original/frontends/amiga/font.c.amiport
	mv original/frontends/amiga/font.c original/frontends/amiga/font.c.bak
	cp ported/frontends/amiga/font.c original/frontends/amiga/font.c
	# ... similar for gui_options.c ...
	# (or use a symlink approach if your filesystem supports it)

restore-source:
	mv original/frontends/amiga/font.c.bak original/frontends/amiga/font.c
	# ... similar for gui_options.c ...

all: override-source
	$(MAKE) -C original/ TARGET=amiga68k
	$(MAKE) restore-source
```

This violates the spirit of "original is read-only" — a cleaner approach is to use the build system to redirect compilation. If upstream NetSurf supports OUTDIR or VPATH, use those. If not, document the override-source dance in PORT.md.

ALTERNATIVE (cleaner): get NetSurf's Makefile to compile from a parallel source tree via `VPATH` (common in autotools-style builds). Investigate first; the `override-source/restore-source` dance is the fallback.

- [ ] **Step 5: Build, FS-UAE smoke test, then A6000 hardware test**

```bash
make -C ports/netsurf
make install-emu TARGET=ports/netsurf
make emu
# In Amiga shell: WORK:netsurf
```

Expected on FS-UAE: launches, log shows `freetype_fonts=true` requested, `amiport_ammx_init` failed, fallback to font_bullet engaged, body text bitmap-only.

```bash
# A6000 hardware test
make package TARGET=ports/netsurf  # builds the .lha
python3 -m amigactl --host 192.168.1.215 put ports/netsurf/netsurf-3.11-1.lha T:netsurf.lha
python3 -m amigactl --host 192.168.1.215 exec "lha x T:netsurf.lha SYS:Programs/"
python3 -m amigactl --host 192.168.1.215 exec "SYS:Programs/netsurf/netsurf"
# Open Aminet front page from Bookmarks; check serial debug log:
python3 -m amigactl --host 192.168.1.215 tail T:netsurf.log
```

Expected on A6000: `amiport_ammx_init: rc=0`, `ami_font_freetype_init: ready (AMMX2 enabled)`, body text renders ANTI-ALIASED (visible improvement vs. baseline).

- [ ] **Step 6: Capture screenshots — before/after**

Take a screenshot of body text from baseline (Phase D `.claude/agent-memory/baseline-jagged.png`) and post-Phase-F (`baseline-aa.png`). Visually compare.

- [ ] **Step 7: Commit**

```bash
git add ports/netsurf/ported/frontends/amiga/font.c \
        ports/netsurf/ported/frontends/amiga/gui_options.c \
        ports/netsurf/Makefile \
        ports/netsurf/.claude/agent-memory/baseline-aa.png
git commit -m "feat(netsurf): wire FreeType backend into font dispatch via nsoption

NetSurf body text now renders AA on Vampire V4/A6000 via FreeType +
AMMX2 alpha compositor. Falls back to font_bullet on non-Apollo (FS-UAE
verified). See baseline-jagged.png vs baseline-aa.png in agent-memory."
```

---

## Phase G — A6000 Hardware Verification

### Task 23: Browse three reference sites on A6000

**Files:** none (verification only)

- [ ] **Step 1: Browse Aminet front page**

```bash
# NetSurf already installed from Task 22 Step 5
python3 -m amigactl --host 192.168.1.215 exec "SYS:Programs/netsurf/netsurf http://aminet.net/"
sleep 30  # let it load
python3 -m amigactl --host 192.168.1.215 screenshot /tmp/netsurf-aminet.png  # if amigactl supports it
# OR have user manually screenshot the V4 monitor
```

Verify: AA body text visible, no crashes, no missing glyphs in standard ASCII range.

- [ ] **Step 2: Browse NetSurf homepage**

Same as Step 1 but `http://www.netsurf-browser.org/`.

- [ ] **Step 3: Browse en.wikipedia.org main article**

Same but `https://en.wikipedia.org/wiki/Amiga`.

- [ ] **Step 4: Capture serial-debug log**

```bash
python3 -m amigactl --host 192.168.1.215 get T:netsurf-session.log /tmp/netsurf-session.log
grep -E "amiport_ammx_init|font_freetype" /tmp/netsurf-session.log
```

Expected:
- `amiport_ammx_init` rc=0
- `ami_font_freetype_init: ready (AMMX2 enabled)`
- No "fallback to bullet" messages
- No Guru Meditation traces

- [ ] **Step 5: Document the test results in PORT.md**

Update PORT.md "Test results" section:

```markdown
## Test results

### Phase G hardware verification (2026-MM-DD on Apollo A6000)

| Site | Result | Notes |
|---|---|---|
| Aminet front page | PASS | AA text, ~3s load |
| netsurf-browser.org | PASS | AA text, ~5s load |
| en.wikipedia.org/wiki/Amiga | PASS | AA text, ~12s load |

Serial debug log confirmed AMMX2 path active for all sessions.
No fallback to font_bullet observed.
```

- [ ] **Step 6: Commit (PORT.md update)**

```bash
git add ports/netsurf/PORT.md
git commit -m "docs(netsurf): record Phase G A6000 hardware verification results"
```

---

## Phase H — CI baselines + ship

### Task 24: Generate FS-UAE visual regression baselines

**Files:**
- Create: `ports/netsurf/test-fsemu-cases.txt`
- Create: `ports/netsurf/test-fsemu-visual-cases.txt`
- Create: `ports/netsurf/test-fixtures/*.html` (5 reference fixtures)
- Create: `ports/netsurf/test-fixtures/*.png` (committed visual baselines)

- [ ] **Step 1: Write the 5 reference HTML fixtures**

```bash
mkdir -p ports/netsurf/test-fixtures
```

Create 5 minimal HTML files: `plain-para.html`, `mixed-headlines.html`, `bold-italic-mix.html`, `unicode-block.html`, `long-line.html`.

Each ~10 lines of HTML, no CSS dependencies, no images, no JS.

- [ ] **Step 2: Write test-fsemu-cases.txt (functional)**

```
TEST: NetSurf launches and exits cleanly with -h
CMD: WORK:netsurf -h
EXPECT_RC: 0
```

(NetSurf's CLI options are minimal — limit functional tests to launch/exit and version. Visual tests carry the real content.)

- [ ] **Step 3: Write test-fsemu-visual-cases.txt (5 SCRAPE cases)**

```
ITEST: Visual: plain paragraph renders
LAUNCH: WORK:netsurf file://WORK:test-fixtures/plain-para.html
KEYS: WAIT5000
SCRAPE
EXPECT_AT 5,1,Lorem ipsum
EXPECT_RC: 0
```

(Repeat for each fixture. Note FS-UAE doesn't emulate Apollo AMMX2 — these tests run the FALLBACK path. The visual tests confirm the binary boots and font dispatch reaches font_freetype.c then falls back; they do NOT verify AA rendering. AA verification is hardware-only per spec.)

- [ ] **Step 4: Run the visual tests, generate baselines**

```bash
make test-fsemu TARGET=ports/netsurf
make test-fsemu TARGET=ports/netsurf VISUAL=1
```

For each ITEST that produces a screen capture, save the resulting baseline PNG to `ports/netsurf/test-fixtures/baseline-<name>.png` and reference in test-fsemu-visual-cases.txt.

- [ ] **Step 5: Commit baselines and tests**

```bash
git add ports/netsurf/test-fsemu-cases.txt \
        ports/netsurf/test-fsemu-visual-cases.txt \
        ports/netsurf/test-fixtures/
git commit -m "test(netsurf): visual regression baselines for FS-UAE CI"
```

---

### Task 25: Wire ports/netsurf/ into top-level make + catalog

**Files:**
- Modify: `Makefile`
- Modify: `PORTS.md`
- Modify: `README.md`
- Modify: `data/catalog.json`
- Modify: `site/data/catalog.json`
- Create: `site/data/packages/netsurf.json`

- [ ] **Step 1: Add netsurf to Makefile build/test loops if not auto-discovered**

```bash
grep -n "test-ports\|build-ports" Makefile | head
```

Most amiport ports are auto-discovered via `ports/*/`. If netsurf needs explicit listing for some reason, add it.

- [ ] **Step 2: Add row to PORTS.md**

Find the `## Network` (or appropriate category) section and add:

```markdown
| netsurf | 3.11-1 | Vampire-native web browser with AA text | comm/www | NetSurf 3.11 (MUI fork) | Shipping |
```

- [ ] **Step 3: Add row to README.md ports table**

Find the Network section in README.md and add netsurf alphabetically.

- [ ] **Step 4: Add data/catalog.json entry**

```bash
cat data/catalog.json | jq '.ported[] | select(.name=="netsurf")'
# (currently empty — need to add)
```

Insert into `ported[]` array:

```json
{
  "name": "netsurf",
  "version": "3.11",
  "revision": 1,
  "category": "network",
  "description": "Vampire-native web browser with AA text",
  "shipped": "2026-05-XX",
  "measured_binary_kb": 0,
  "test_count": 0,
  "test_pass_rate": 1.0,
  "vampire_required": true
}
```

(Update `measured_binary_kb` and `test_count` after the build settles.)

- [ ] **Step 5: Sync site/data/catalog.json**

```bash
cp data/catalog.json site/data/catalog.json
```

- [ ] **Step 6: Create site/data/packages/netsurf.json**

Copy the schema from another network port (e.g., `site/data/packages/wget.json`) and adapt:

```json
{
  "name": "netsurf",
  "version": "3.11",
  "revision": 1,
  "size": 0,
  "sha256": "",
  "machine_size": 0,
  "machine_sha256": "",
  "download": "/packages/netsurf-3.11-1.lha",
  "published_at": "2026-05-XX",
  "updated_at": "2026-05-XX",
  "description": "Vampire-native web browser with anti-aliased text rendering",
  "readme": "<full readme text here>",
  "category": "network",
  "vampire_required": true
}
```

(Update sizes/sha256 after `make package`.)

- [ ] **Step 7: Run check-port-metadata**

```bash
make check-port-metadata 2>&1 | grep netsurf
```

Expected: PASS for all netsurf checks.

- [ ] **Step 8: Build the LHA + machine LHA, populate sizes/sha256**

```bash
make package TARGET=ports/netsurf
ls -la ports/netsurf/netsurf-3.11-1*.lha
sha256sum ports/netsurf/netsurf-3.11-1.lha
sha256sum ports/netsurf/netsurf-3.11-1-machine.lha
```

Update `data/catalog.json`, `site/data/catalog.json`, `site/data/packages/netsurf.json` with actual sizes and sha256 values.

- [ ] **Step 9: Stage LHAs into site/packages/**

```bash
cp ports/netsurf/netsurf-3.11-1.lha site/packages/
cp ports/netsurf/netsurf-3.11-1-machine.lha site/packages/
```

Per `.claude/rules/site-mirror-discipline.md`: every advertised LHA MUST exist in `site/packages/` BEFORE deploy.

- [ ] **Step 10: Run all validation**

```bash
make check-docs
make check-port-metadata
make check-arexx
```

Expected: all PASS.

- [ ] **Step 11: Commit**

```bash
git add Makefile PORTS.md README.md data/catalog.json site/data/catalog.json \
        site/data/packages/netsurf.json site/packages/netsurf-*.lha
git commit -m "feat(netsurf): catalog entry, site package, LHA staged for deploy"
```

---

### Task 26: Final A6000 ship-gate verification

**Files:** none (verification only)

- [ ] **Step 1: Install the official packaged LHA on a clean A6000 directory**

```bash
python3 -m amigactl --host 192.168.1.215 exec "delete RAM:netsurf-test all" 2>/dev/null
python3 -m amigactl --host 192.168.1.215 put site/packages/netsurf-3.11-1.lha RAM:netsurf-3.11-1.lha
python3 -m amigactl --host 192.168.1.215 exec "lha x RAM:netsurf-3.11-1.lha RAM:"
python3 -m amigactl --host 192.168.1.215 exec "RAM:netsurf/netsurf"
```

Verify the published LHA (not just the dev build) launches cleanly.

- [ ] **Step 2: Browse the three reference sites again with the official LHA**

Same as Task 23 Steps 1-3, but using the freshly extracted LHA.

- [ ] **Step 3: Capture screenshots for the .readme**

Take a clean screenshot of the AA body text rendering on Wikipedia. Save to `site/screenshots/netsurf-aa-body.png` for the website preview.

- [ ] **Step 4: Update site/data/packages/netsurf.json with screenshot reference**

```json
{
  ...
  "screenshots": ["/screenshots/netsurf-aa-body.png"]
}
```

- [ ] **Step 5: Commit the final verification artifacts**

```bash
git add site/screenshots/netsurf-aa-body.png site/data/packages/netsurf.json
git commit -m "ship(netsurf): A6000 verification with official LHA — ready for deploy"
```

- [ ] **Step 6: Hand off to amiport-publisher agent**

Per `.claude/rules/use-pipeline-agents.md`: never publish manually. Dispatch the `amiport-publisher` agent with:

> "Publish ports/netsurf 3.11-1 to amiport.platesteel.net. All gates pass: build clean, FS-UAE visual tests pass, A6000 hardware verification confirmed AMMX2 path active and AA body text renders on Aminet/NetSurf/Wikipedia. Site mirror staged at site/packages/. Catalog updated. Memory-checker and perf-optimizer NOT yet run — deferring those to Phase 1.5 per spec."

---

## Phase I — Knowledge corpus contributions

### Task 27: Capture all Phase 1 learnings to amiga-kb

**Files:** none (MCP operations)

- [ ] **Step 1: Review the implementing engineer's pitfall captures**

```bash
git log --all --grep="capture-learning\|amiga-kb" --since="2026-05-02" --oneline
```

Confirm all `/capture-learning` invocations made during the phases above were routed correctly.

- [ ] **Step 2: Add a synthesizing pitfall about Vampire-native ports**

```
amiga_add_pitfall:
  title: "Vampire-native amiport ports require amiport_ammx_init() at startup"
  description: "Any amiport port that links vasm-assembled AMMX kernels MUST
                call amiport_ammx_init() once at startup before any AMMX code
                runs. Failure to do so causes Line-F traps on context switch
                because E0-E23 register state isn't saved. The function lives
                in libamiport (lib/posix-shim/src/ammx_init.c) and wraps
                vampire.resource V_EnableAMMX(V_AMMX_V2). Returns 0 on
                success, non-zero error code otherwise. NetSurf is the
                first consumer (Phase 1, 2026-05). Pattern documented in
                ports/netsurf/.claude/agent-memory/ammx-pattern-notes.md."
  severity: medium
  related_apis: ["V_EnableAMMX", "amiport_ammx_init", "vampire.resource"]
  source_project: amiport
```

- [ ] **Step 3: Add an addendum to pc-game-porting-cookbook.md (if exists in KB)**

Document the AMMX glyph compositor as a reusable pattern other ports can adopt for any alpha-blend operation (sprite blits, UI compositing, image overlay).

- [ ] **Step 4: Run amiga_coverage to confirm new entries indexed**

```
amiga_coverage
```

Expected: pitfall count increased by N (one per `/capture-learning` plus the synthesizing one).

- [ ] **Step 5: No commit (MCP operations)**

---

## Self-Review

This is the engineer's own pass — not a subagent dispatch. After writing the plan, I verified:

**1. Spec coverage:**
- ✅ FreeType integration → Phase F (Task 21)
- ✅ Glyph cache → Phase C (Tasks 8-13)
- ✅ AMMX2 compositor → Phase E (Tasks 17-19)
- ✅ vasm in Docker → Phase A (Task 1)
- ✅ flype44 SDK vendor → Phase A (Task 2)
- ✅ amiport_ammx_init → Phase B (Tasks 4-7)
- ✅ NetSurf-MUI fork → Phase D (Task 14)
- ✅ font.c dispatch patch → Phase F (Task 22)
- ✅ A6000 hardware test → Phase G (Task 23)
- ✅ FS-UAE CI baselines → Phase H (Task 24)
- ✅ amiga-kb queries throughout → Pre-implementation + per phase

**2. Placeholder scan:** No "TBD", "TODO", or "implement later" outside intentional placeholder scaffolds (the asm SCALAR PLACEHOLDER and the TODO Step N markers within tasks pointing to subsequent steps within the same task — those are part of the bite-size step structure, not the no-placeholders rule violation).

**3. Type consistency:** `amiport_glyph_cache_t`, `amiport_glyph_t`, function signatures match across header (Task 8), implementation (Task 10), tests (Task 9), and consumer (Task 21). `font_compose_glyph_argb32_ammx` signature matches across asm (Task 18), test (Task 19), and consumer (Task 21).

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-05-02-netsurf-vampire-text-rendering.md`. Two execution options:

**1. Subagent-Driven (recommended)** — Dispatch a fresh subagent per task, review between tasks, fast iteration. Good for this plan because each task is independently verifiable and the subagents can pull context fresh from the spec without polluting the main session's window.

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints. Risks main session context bloat over the ~27 tasks.

Recommendation: **subagent-driven**, with the user reviewing the spec + plan first before any task starts. Also recommend moving execution into a dedicated worktree (`EnterWorktree` or `git worktree add ../amiport-netsurf-phase1 -b feature/netsurf-vampire-phase1`) so the multi-month effort doesn't tangle with main.

Which approach?
