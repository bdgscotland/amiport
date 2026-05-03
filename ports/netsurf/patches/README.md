# ports/netsurf/patches

Patches against the upstream `ports/netsurf/original/` submodule
(`https://github.com/arczi84/NetSurf-3.11-MUI.git`).

We do NOT have push access to the upstream repo, so our build-system
patches live here and get applied at build time. Three options on the
table for next session to choose between:

1. **Patch-and-go (current):** apply via `git apply` from this dir
   before each build. Pros: lightweight commits, clean upstream
   tracking. Cons: patches need maintenance if upstream moves.
2. **De-submodule:** vendor `ports/netsurf/original/` as a regular
   subdirectory. Pros: full control, no patch dance. Cons: heavy
   commit (~150K LOC), loses upstream-tracking metadata.
3. **Fork to bdgscotland:** create our own NetSurf-3.11-MUI fork on
   GitHub, push our patches there, update `.gitmodules` to point at
   the fork. Pros: clean submodule + push access. Cons: fork
   maintenance, slower upstream sync.

User decides which path to take when the submodule patches grow
beyond ~5-10 small fixes.

## Current patches

### 01-amiport-netsurf-mui-buildsystem.patch

Three buildsystem fixes to make the netsurf-mui-3.11 fork buildable
in our amiport bebbo-gcc 13.3 + libnix Docker environment. The fork's
upstream `Makefile.config` was hardcoded for the original author's
build environment (`/opt/netsurf/`, `/mnt/d/opt/netsurfy/`, clib2
toolchain). Three .files changed:

1. **`Makefile.config`** — wholesale replacement. Drops author's
   `/opt/netsurf/`, `/mnt/d/opt/netsurfy/` paths, clib2 references,
   and hardcoded `CC := /opt/netsurf/.../m68k-unknown-amigaos-gcc`
   override. Sets only NetSurf feature flags (no toolchain vars
   - those come from wrapper Makefile env). Disables `NETSURF_USE_UTF8PROC`
   (we don't have libutf8proc), `NETSURF_USE_OPENSSL`, `NETSURF_USE_AMISSL`,
   `NETSURF_USE_DUKTAPE`, `NETSURF_USE_LIBICONV_PLUG`, `NETSURF_USE_NSSVG`
   (we don't have libsvgtiny). Enables `NETSURF_USE_CURL`, `NETSURF_USE_PNG`,
   `NETSURF_USE_JPEG`, `NETSURF_USE_BMP`, `NETSURF_USE_GIF`,
   `NETSURF_USE_NSLOG`, `NETSURF_USE_NSPSL`, `NETSURF_USE_FS_BACKING_STORE`.

2. **`Makefile`** — switches JPEG detection from upstream's
   `feature_switch JPEG ... -ljpeg-ammx` (which hardcoded a custom
   AMMX-optimized libjpeg variant the netsurf-mui author shipped)
   to standard `pkg_config_find_and_add_enabled JPEG libjpeg JPEG`,
   so JPEG comes from our `lib/pkgconfig/libjpeg.pc` like every
   other lib in the dep stack.

3. **`frontends/mui/Makefile`** — disables `-DTURBOJPEG` (we use
   stock libjpeg, not the TurboJPEG variant the author had bundled)
   and drops a hardcoded `-I/mnt/e/usr/local/amiga/m68k-amigaos/clib/extra/ns-include`
   path that referenced the netsurf-mui author's machine layout.

The patch also includes a saved copy of the original `Makefile.config`
at `Makefile.config.upstream-author.bak` for reference.

## Apply

```
cd ports/netsurf/original
git apply ../patches/01-amiport-netsurf-mui-buildsystem.patch
```

Or, if the wrapper Makefile is upgraded to apply patches automatically,
the build target should `git apply` before building and `git checkout .`
after to reset the tree.

## Build-state caveat

This patch was generated 2026-05-02 against the submodule SHA pinned
in `.gitmodules`. If the submodule pointer moves (`git submodule update`
pulls newer netsurf-mui commits), the patch may need rebasing. Check
`Makefile.config.upstream-author.bak` against the current upstream
`Makefile.config` before applying.

## What this patch does NOT cover

The 2026-05-02 session 7 reconnaissance also surfaced (and FIXED in our
worktree, NOT in this patch yet):

- `lib/libdom/include/dom/bindings/hubbub/{errors,parser,utils}.h` —
  exposed the libhubbub-binding headers from libdom's internal `src/`
  (NetSurf's `content/handlers/html/private.h` includes `<dom/bindings/hubbub/parser.h>`)
- `lib/libjpeg/include/jconfig.h` — exposed the libjpeg config header
  (jpeglib.h includes it)
- `lib/libpng/include/pnglibconf.h` — exposed the libpng config header
  (png.h includes it)
- `lib/pkgconfig/libjpeg.pc` — added pkg-config descriptor for libjpeg

Those changes belong in the lib/ trees (not the submodule) and are
committed separately.

## What's BLOCKING next session

`SDI_compiler.h` — the System Development Includes header set, used
heavily by NetSurf-MUI's `frontends/mui/include/macros/vapor.h` (and
many other MUI frontend files). SDI is on Aminet at `dev/c/SDI_headers.lha`,
free to redistribute. Need to vendor it (similar to how amiport vendors
amissl-sdk). Aminet research first per `.claude/rules/use-pipeline-agents.md`.
