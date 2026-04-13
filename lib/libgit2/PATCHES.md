# libgit2 1.8.5 — Patches Applied for amiport

This file documents every modification made to upstream libgit2 1.8.5
source code for the amiport (bebbo-gcc / libnix / 68k AmigaOS 3.x) build.
Each patch is justified, narrow, and must be reapplied when rebasing on
a future libgit2 release.

Source-analyzer audit trail: dispatched 2026-04-13, verdict CAVEATS,
go recommendation. All patches below were identified from that audit
and verified against the actual upstream source.

Upstream pin: `libgit2/libgit2` tag `v1.8.5` (commit c7e6c72f).

## Patch list

### 1. `src/util/fs_path.c` — non-ASCII bytes in comment

**Problem:** The block comment at lines 43-50 contained UTF-8 characters
(`ä` U+00E4, `֍` U+058D) as examples of Unicode drive letters assignable
via Windows `subst`. bebbo-gcc 6.5.0b silently corrupts translation units
containing UTF-8 bytes (known-pitfalls.md: "UTF-8 Characters in Comments
Break bebbo-gcc Preprocessor").

**Fix:** Replaced the comment body with an ASCII-only description that
retains the technical information (U+00E4, U+058D as codepoints rather
than literal characters) and references the amiport pitfall.

**Scope:** Comment only; no executable-code change.

---

### 2. `src/util/utf8.h` — non-ASCII bytes in doc comment

**Problem:** Comment at lines 30-34 demonstrated UTF-8 byte sequences
using literal characters (`¢` U+00A2, `𐍈` U+10348). Same bebbo-gcc
hazard.

**Fix:** Rewrote the byte-sequence examples as (codepoint, byte count)
descriptions in pure ASCII. Technical content preserved.

**Scope:** Comment only; no executable-code change.

---

### 3. `src/util/git2_util.h` — reduce GIT_BUFSIZE_DEFAULT on AmigaOS

**Problem:** `GIT_BUFSIZE_FILEIO` (= `GIT_BUFSIZE_DEFAULT` = 65536) is
used as a local stack buffer in at least four places:
- `src/util/filebuf.c:68` (append path)
- `src/util/futils.c:901` (cp_by_fd)
- `src/libgit2/blob.c:106` (blob content read)
- `src/libgit2/odb.c:216` (odb loose object read)

With amiport's standard `__stack = 262144` and ~4 KB hidden AmigaOS
overhead per dos.library call, a 64 KiB local array is too close to
the stack limit -- especially in libgit2's deeply-recursive tree and
pack-walk code paths. Real AmigaOS Guru Meditation is the likely
outcome on non-trivial repos (known-pitfalls: "Large Local Buffers
Cause Guru on Real AmigaOS").

**Fix:** Added `#if defined(__AMIGA__)` branch that reduces
`GIT_BUFSIZE_DEFAULT` to 4096 on AmigaOS, leaving 65536 on other
platforms. Applied at the macro-definition level (not per call site)
so it also catches any future `GIT_BUFSIZE_*` user transparently.

**Scope:** 1 macro definition; all 4 known consumers inherit the fix.
Performance impact negligible (68k filesystem+DMA overhead dominates
over syscall count).

---

### 4. `src/util/rand.c` — AmigaOS entropy branch

**Problem:** libgit2's `getseed()` for non-Win32 calls `getentropy()`,
tries `/dev/urandom`, then falls back to mixing `getppid()`, `getpgid()`,
`getsid()` into a time-based seed. None of those four symbols exist in
libnix or the amiport posix-shim. Linker failure.

`getentropy` is also pitfall-relevant: there is no kernel entropy source
on AmigaOS (known-pitfalls: "No /dev/urandom on AmigaOS").

**Fix:** Added an `#elif defined(__AMIGA__)` branch that skips the
`/dev/urandom` probe entirely, and an `#if !defined(__AMIGA__)` guard
around the three missing process-hierarchy calls. In their place, the
AmigaOS branch mixes in two stack-local-variable addresses (which vary
per invocation due to AmigaOS task allocation). This matches the
CPython 3.11 port's entropy pattern (see known-pitfalls: "No
/dev/urandom on AmigaOS").

The resulting entropy quality is suitable for libgit2's actual use
case: hash randomization for idxmap/offmap collision mitigation. Not
cryptographic-grade, but libgit2 does not use `git_rand_*` for
cryptographic operations -- object hashing goes through sha1dc/rfc6234.

**Scope:** Non-Win32 `getseed()` only. The Win32 branch and the rest
of the file (xoshiro256** state, splitmix64, git_rand_next) are
untouched.

---

### 5. `src/pcre/config.h` — hand-written PCRE config (new file)

**Problem:** libgit2 bundles PCRE 8.x in `deps/pcre/`. PCRE's `.c`
files `#include "config.h"` which is normally CMake-generated.

**Fix:** Added `lib/libgit2/src/pcre/config.h` (new file, not a patch
to upstream) with the minimal feature set:
- `SUPPORT_PCRE8=1`
- No JIT, no UTF, no UCP (ASCII-only build)
- `LINK_SIZE=2`, standard limits
- `NEWLINE=10` (LF)

Matches the oniguruma port pattern.

**Scope:** New file at `src/pcre/config.h`. No upstream PCRE source
modified.

---

### 6. `src/util/git2_features.h` — hand-written feature flags (new file)

**Problem:** libgit2 normally generates `git2_features.h` from
`git2_features.h.in` via CMake. amiport does not use CMake.

**Fix:** Added `lib/libgit2/src/util/git2_features.h` (new file,
not a patch to upstream) with the amiport disable matrix:
- `GIT_ARCH_32=1`
- `GIT_REGEX_BUILTIN=1`
- `GIT_SHA1_COLLISIONDETECT=1`
- `GIT_SHA256_BUILTIN=1`
- All transports, threading, iconv, nsec stat, entropy sources off

Derived from `git2_features.h.in` at v1.8.5.

**Scope:** New file. No upstream source modified.

---

### 7. `src/util/amigaos_compat.h` — amiport shim force-include (new file)

**Problem:** libnix lacks many POSIX functions libgit2 expects (pread,
pwrite, realpath, lstat, ftruncate, readlink). The amiport posix-shim
provides `amiport_*` equivalents with `#define pread amiport_pread`
macros, but including `<amiport/unistd.h>` in every libgit2 source
file is impractical.

**Fix:** Added `lib/libgit2/src/util/amigaos_compat.h` (new file) that
includes the three amiport headers providing the #define macros, gated
on `#ifdef __AMIGA__`. Injected into every translation unit via the
Makefile's `-include` flag.

**Scope:** New file + Makefile `-include` directive. No upstream source
modified.

---

## Source files EXCLUDED (not copied from upstream)

For completeness, the pruning applied during the Stage 2 source copy
(these files were NEVER placed in `lib/libgit2/src/`, so there is no
patch to reapply -- just note them when re-syncing against a new
upstream release):

**From `src/libgit2/`:**
- `clone.c`, `clone.h`, `fetch.c`, `fetch.h`, `fetchhead.c`,
  `fetchhead.h`, `proxy.c`, `proxy.h`, `push.c`, `push.h`,
  `remote.c`, `remote.h`, `transport.c`
- Entire `src/libgit2/streams/` directory (TLS backends)
- Entire `src/libgit2/transports/` directory (wire transports)
- `git2.rc` (Windows resource)
- `experimental.h.in` (CMake template)

**From `src/util/`:**
- `unix/process.c` (fork/exec -- the reason we chose libgit2 over git)
- Entire `win32/` directory

**From `src/util/hash/`:**
- `common_crypto.c` (macOS), `mbedtls.c`, `openssl.c`, `win32.c`
  (kept only `builtin.c`, `collisiondetect.c`, `sha1dc/`, `rfc6234/`)

**From `src/util/allocators/`:**
- `debugalloc.c`, `failalloc.c`, `win32_leakcheck.c`
  (kept only `stdalloc.c` for the production allocator)

**From `deps/`:**
- `zlib/`, `chromium-zlib/` (we link lib/zlib/libz.a instead)
- `llhttp/` (transport-layer HTTP parser, dead without transports)
- `ntlmclient/`, `winhttp/` (Windows auth / transport)
- `pcre/pcre_jit_compile.c` (365 KB, no 68k JIT target)
- `pcre/pcre_ucd.c` (209 KB Unicode data, ASCII-only build)

## Re-sync procedure

When upgrading libgit2 to a newer release:

1. Verify which of the 5 upstream source patches above still apply
   cleanly to the new version (the relevant files may have been
   refactored).
2. Review `git2_features.h.in` for new `#cmakedefine` lines; mirror
   any additions in `src/util/git2_features.h`.
3. Re-run `.claude/rules/library-pipeline.md` Stage 1 (source-analyzer)
   against the new source tree. Compare its pitfall list to the one
   that drove this file.
4. Re-run memory-checker and perf-optimizer (Stages 6-7).
5. Full FS-UAE test suite re-pass.
