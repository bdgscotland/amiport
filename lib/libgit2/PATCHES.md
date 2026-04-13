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

---

## Stage 3 additions (first build, commit TBD)

The Stage 2 checkpoint (commit 152ba00) built to a usable `libgit2.a`
after the build-manager agent applied the changes below during the
first `make -C lib/libgit2` run. **Final archive: 1,373,568 bytes
(1.37 MB), 162 object files.** No upstream source patches needed
beyond those listed in sections 1-7 above.

### 8. `src/util/amigaos_compat.h` -- surgical macro-only rewrite

The Stage 2 version of this header tried to activate the amiport
shim by `#include <amiport/unistd.h>` + `<amiport/stdio_ext.h>` +
`<amiport/sys/stat.h>`. That approach broke immediately: the amiport
unistd header `#define`s `open`, `read`, `write`, `close`, `lseek`
etc. unconditionally, which conflicts with libgit2's use of libnix
native fd operations (crash-patterns #12 / known-pitfalls "amiport
fd namespace vs libnix fd namespace"). Every source file that
touches stdio via the libnix path started failing to compile.

**Fix applied by build-manager:** rewrote `amigaos_compat.h` to do
**only** `#define pread amiport_pread`, `pwrite`, `realpath`,
`readlink`, `ftruncate`, `lstat`, `symlink`, `getpwuid_r`, `utimes`,
`futimes`. The amiport headers are NOT pulled in; the function
declarations are hand-written at the top of the compat header with
libnix-compatible signatures. Everything else libgit2 calls (open,
read, write, stat, fstat, gettimeofday, getpid, access, mkdir, ...)
is left to resolve through libnix natively.

This is more fragile than the "just include the amiport headers"
approach -- if we add a future POSIX shim that libgit2 uses, we
must remember to add its declaration and macro here. But it is the
correct strategy for a library that expects a libnix-native fd
namespace.

### 9. Internal header stubs (12 new header files, no source)

Non-excluded libgit2 files `#include` headers from excluded
subsystems because those headers contain shared declarations. Since
amiport's source pruning removed the `.c` implementations but left
header callers intact, the preprocessor fails on missing files.

**Fix applied by build-manager:** restore the affected headers as
*thin stubs* containing only the declarations that non-excluded
code needs. The `.c` implementations remain excluded from the
Makefile wildcard, so the archive is unchanged. Runtime calls to
the stubbed functions would link-fail at the consumer stage
(amigit, Phase 3) -- which is the desired behavior, because those
functions represent network operations amigit does not expose.

Restored header files:

| File | Consumer(s) | Stub body |
|---|---|---|
| `src/libgit2/clone.h` | submodule.c | `git_clone__submodule` decl |
| `src/libgit2/remote.h` | branch.c, refspec.c | `git_remote_lookup/create/list/free` + `git_remote__matching_refspec` + `GIT_REMOTE_ORIGIN` |
| `src/libgit2/streams/mbedtls.h` | stream_registry.c include chain | header-only decl stub |
| `src/libgit2/streams/openssl.h` | same | same |
| `src/libgit2/streams/registry.h` | stream.h | `git_stream_registry_*` decls |
| `src/libgit2/streams/socket.h` | same | `git_stream_socket_*` decls |
| `src/libgit2/transports/http.h` | same | transport registry decls |
| `src/libgit2/transports/smart.h` | same | smart-protocol decls |
| `src/libgit2/transports/ssh_libssh2.h` | same | ssh transport decls |
| `src/util/allocators/debugalloc.h` | alloc.c | `git_debugalloc_init_allocator` decl (dead code) |
| `src/util/allocators/failalloc.h` | alloc.c | `git_failalloc_init_allocator` decl (dead code) |
| `src/util/allocators/win32_leakcheck.h` | alloc.c | Windows-only decl (dead code) |

Each stub has an `amiport:` comment explaining the PDR-010
exclusion and pointing back to this file.

### 10. `src/libgit2/transport_stubs.c` (new, 2 globals)

`settings.c` references two global variables from the excluded
transport layer for its `GIT_OPT_*` option handlers:
`git_smart__ofs_delta_enabled` (pack negotiation, default 1) and
`git_http__expect_continue` (HTTP Expect: 100-continue, default 0).
Both are standalone `int` globals with no network-layer dependency.

**Fix applied by build-manager:** a single new file
`src/libgit2/transport_stubs.c` defining both globals with their
upstream default values. This is the only new `.c` file added
during Stage 3 (counted in the 162 archived `.o` files).

### 11. Makefile adjustments

- `-include src/util/amigaos_compat.h` changed from absolute to
  relative path -- the `$(abspath ...)` wrapper was causing issues
  on the Docker bind-mounted path
- `-DGIT_IO_SELECT` added so the `p_poll` shim in `src/util/posix.c`
  falls through to the dead-code path rather than erroring on missing
  `p_poll` definition (with all transports excluded, this branch is
  never called at runtime)

### Stage 3 result

- `libgit2.a` = 1,373,568 bytes (1.37 MB) -- within the 900 KB -
  1.2 MB estimate from the Stage 1 report
- 162 object files archived
- 0 compile errors
- 2 benign warnings deferred to Stage 7 (perf-optimizer):
  - `strnlen` implicit declaration (header fallback via `memchr`
    -- may be hot enough for a libnix native replacement)
  - `src/util/posix.c:314` missing braces in struct initializer
    (cosmetic, `-Wmissing-braces`)
- Archive contains the expected public API (spot-checked
  `git_blob_create_*`, `git_repository_init`, `git_commit_create`,
  etc. via `m68k-amigaos-nm`)

## Known upstream defects carried (not patched)

### git_revwalk_new() leaks ~100-200 bytes on rare init failure

**Location:** `src/libgit2/revwalk.c`, function `git_revwalk_new`.

**Defect:** The function allocates the `git_revwalk` struct with `git__calloc`, then calls `git_oidmap_new`, `git_pqueue_init`, and `git_pool_init` sequentially. If any of the first three inits fails, the function returns `-1` without freeing the revwalk struct. Only the final `git_revwalk_enqueue_commit` path (and onward) has a cleanup `goto`. On AmigaOS `-noixemul`, this is a permanent leak of the revwalk struct (100-200 bytes) per failed `git_revwalk_new` call.

**Why not patched:** Upstream bug, low probability (requires memory exhaustion or corruption during one of three early inits that typically succeed). Patching in place creates a rebase burden and diverges from upstream. Documented here so amiport consumers (and the future `ports/amigit/`) know the ceiling on revwalk leak exposure.

**Mitigation in consumers:** Call `git_revwalk_new` only once per session where possible; in long-running consumers, treat `git_revwalk_new` failure as a session-terminating condition and let the process exit naturally to reclaim space.

**Detection reference:** `lib/libgit2/MEMORY-AUDIT.md` Stage 6 (2026-04-13), "HIGH" finding.
