# PDR-010: amigit — a Local-Only Git for AmigaOS 3.x via libgit2

## Status

Proposed

## Date

2026-04-12

## Problem

There is no version control system for AmigaOS 3.x on 68k. Amiga developers
default to mailing tarballs, manual backup copies, or editing on a cross-host
over a network share — all of which lose the benefits modern developers take
for granted (history, branching, bisect, blame, patch review).

Previous community attempts to port git died in the dependency audit. The one
public effort, `sgit` (sba1/simplegit), is **AmigaOS 4 / PPC only** — no 68k
OS3.x build exists. Vanilla git is structurally infeasible for 68k because its
command dispatch model is built on `fork()`/`execvp()` with 53 call sites in
`run-command.c` alone, and there is no compile-time bypass (see the
dependency-auditor's 2026-04-12 report for the full analysis).

The amiport pipeline can credibly ship a local-only git-compatible tool **if**
we sidestep the fork/exec execution model. The technically clean path is
**libgit2** wrapped in a purpose-built Amiga CLI.

## Target Users

- Amiga developers on modern accelerators (TF1260, Warp 060, Vampire V2/A6000,
  PiStorm) writing code on the machine directly
- Cross-porters who want to commit their amiport work from the Amiga side for
  portability testing rather than only from the host
- Retrocomputing hobbyists maintaining long-term personal projects who want
  history and branching without leaving AmigaOS
- amiport itself: having a local VCS on the target platform closes the loop on
  "a developer could realistically work entirely on-Amiga for CLI software"

Not the target: users who need network-connected git (clone/fetch/push from
GitHub, GitLab, etc.). That scope is out of reach without libcurl+libssh2 ports
that do not exist for OS3.

## Decision

Build **`amigit`**: an Amiga-native VCS tool that speaks the git repository
format (`.git/objects/`, `.git/refs/`, packfiles, the full git on-disk spec)
but runs as a single-process, no-fork, no-network CLI on AmigaOS 3.x.

### Technical approach

1. **Port zlib to `lib/zlib/`** — zlib is a mandatory dep (git's pack format is
   deflate-compressed) and is a no-regrets addition to amiport regardless of
   whether amigit ships. It unlocks many future ports (zip/unzip, PNG tools,
   man pages, etc.).
2. **Port libgit2 to `lib/libgit2/`** with transports and threading disabled:
   `-DUSE_SSH=OFF -DUSE_HTTPS=OFF -DUSE_HTTP_PARSER=builtin
   -DUSE_THREADS=OFF -DBUILD_CLAR=OFF -DREGEX_BACKEND=builtin
   -DUSE_BUNDLED_ZLIB=OFF`. libgit2 is MIT-licensed, ~180 KLOC, designed from
   the start for embedding — the opposite architecture of vanilla git.
3. **Write `ports/amigit/`** — ~1–2 KLOC Amiga-native CLI wrapper that maps
   `argv` to libgit2 function calls. No subprocess spawning, no editor shelling,
   no pager piping. Commit messages come in via `-m` or `-F file`. Output can
   be piped to `amiport less` manually if the user wants paging.

### Hardware target

**68040/68060 only**, **≥ 16 MB Fast RAM recommended**, **SFS/PFS3 strongly
recommended**. This is a deliberate scope cut away from the amiport default
(68020/4 MB), documented explicitly in the .readme.

Justification:
- **68040/60 for speed** — zlib inflate on 68020/14 MHz is prohibitively slow
  for even small packfiles; 68040/25 is the minimum for a usable experience
- **16 MB Fast RAM** — git operations load trees, indexes, and often whole
  pack-index files into memory. 8 MB is the hard floor; 16 MB is the working
  set for realistic repos
- **SFS/PFS3** — git creates deeply nested paths (`.git/objects/ab/cdef...`),
  tracks a case-sensitive ref namespace, and benefits from symlink support.
  OFS/FFS filename length limits (30 / 107 chars) and case-insensitivity create
  correctness problems we'd have to paper over; SFS/PFS3 make them disappear

This is the first amiport port to officially require an accelerator tier. The
hardware-expansion discussion in PDR-009 explicitly anticipates this.

### Functional scope — what amigit ships

**Read-side (repo inspection):**
- `amigit init [--bare] [path]`
- `amigit status [--short]`
- `amigit log [--oneline] [-n N] [<rev>]`
- `amigit show <rev>` / `amigit show <rev>:<path>`
- `amigit diff [<rev>] [<rev>]` / `amigit diff --cached`
- `amigit cat-file -t <rev>` / `amigit cat-file -p <rev>`
- `amigit ls-files` / `amigit ls-tree <rev>`
- `amigit rev-parse <ref>`
- `amigit blame <path>` (if libgit2's blame fits in the scope budget)
- `amigit branch [-a | -d <name> | <new>]`
- `amigit tag [-d <name> | <new> [<rev>]]`

**Write-side (authoring):**
- `amigit add <path>...`
- `amigit rm <path>...`
- `amigit mv <old> <new>`
- `amigit commit -m <msg>` / `amigit commit -F <file>` / `amigit commit --amend`
- `amigit checkout <ref>` / `amigit checkout -b <new>`
- `amigit reset [--soft | --mixed | --hard] <rev>`
- `amigit merge <ref>` — **fast-forward and trivial 3-way only**; conflicts
  produce standard conflict markers in the working tree and an unclean index
  for the user to resolve with a separate editor
- `amigit restore <path>` / `amigit stash push` / `amigit stash pop`

**Config:**
- `amigit config user.name <value>` / `amigit config user.email <value>`
- Reads `.git/config`, `$HOME/.gitconfig`, `/S/gitconfig`

### Functional scope — explicitly out

- **All network transports.** No `clone`, `fetch`, `pull`, `push`, `remote
  add`, `submodule`. Repos must be copied to the Amiga via some other mechanism
  (ADF transfer, bsdsocket file copy, smbfs, etc.)
- **Hooks.** No pre-commit, post-commit, pre-push, update, etc. All hook
  invocations are stubbed to no-ops. `.git/hooks/` is ignored
- **Editor invocation.** No `$EDITOR` fallback for commit messages. `-m` or
  `-F` required
- **Pager invocation.** No automatic `less` piping. User pipes manually
- **External diff/merge drivers.** Only libgit2's built-in patience/myers diff
  and its built-in 3-way merge
- **Submodules.** Ignored entirely — subrepos are left as untracked content
- **`git grep -P`.** No PCRE. Plain regex only (use the `grep` port for rich
  pattern matching)
- **i18n.** English only
- **SHA-256 repo format.** SHA-1 only at v1. SHA-256 is a v2 consideration
- **GPG signing.** No commit/tag signature verification or creation

### Semantic adaptations for AmigaOS

These need deliberate design calls, documented in the eventual ADR:

- **Executable bit.** AmigaOS uses s/p/a/r/w/e/d protection bits, not a single
  POSIX executable bit. Policy: track blob mode as `100644` on checkout
  (regardless of Amiga bits), accept `100755` from existing repos without
  changing working-tree permissions. `core.fileMode = false` semantics by
  default. Lossy but consistent.
- **Symlinks.** On SFS they work. On FFS they don't. Policy: if the target
  filesystem reports no symlink support, check out symlinks as regular files
  containing the link target as text (git's standard fallback, same as Windows
  default). `core.symlinks = false`.
- **Case-insensitive ref namespace.** `refs/heads/Feature` and
  `refs/heads/feature` collide on OFS/FFS. On SFS they don't. Policy: always
  lowercase ref names when writing; reject mixed-case ref creation with an
  error. Minor deviation from upstream git semantics, documented.
- **Path length.** Git's `.git/objects/aa/bbbbbb...` paths are 42 chars + root
  — fits comfortably under SFS but hits the 30-char OFS limit. Hard requirement
  on SFS/PFS3 documented in the .readme.
- **Stdin/stdout encoding.** git assumes UTF-8. AmigaOS consoles are ISO-8859-1.
  Commit messages stored verbatim. No iconv.
- **Line endings.** `core.autocrlf = false` always. Amiga text files are LF.
  If users import from DOS/Windows sources they keep CRLF.
- **CR/LF in pack files.** Pack files are binary — no concern.
- **Time zones.** Amiga system clock is local-time. Store commits with a `+0000`
  offset and the local-time timestamp, or read timezone from an envvar
  (`TZ=GMT`). libgit2's commit API takes a `git_time` struct with offset — we
  just have to fill it correctly.

### Library build decisions

- **zlib 1.3.1** (current stable). MIT-compatible. Build with `-O0` initially
  per the pitfall; measure before promoting to `-O2`.
- **libgit2 1.8.x** (last stable before their CMake+OpenSSL-by-default
  transition in 1.9). Needs audit: recent versions started assuming more POSIX
  semantics. 1.8.x still has clean `NO_*` knobs.
- **CMake not used** — libgit2 upstream is CMake, but amiport's toolchain is
  Make-based. We generate a static `Makefile` by hand that mirrors
  libgit2's CMakeLists, same pattern as `lib/oniguruma/`.
- **SHA-1 backend:** libgit2 ships `sha1dc` (collision-detecting); use that,
  do not link any external crypto. Matches git's `block-sha1` strategy.
- **Regex backend:** libgit2's `regex_bundled` (PCRE-lite built-in). Do not
  link system regex.
- **HTTP parser:** libgit2's `http_parser_bundled`. Irrelevant at runtime since
  transports are off, but some code paths include the header.
- **-O0 default for both libraries.** bebbo-gcc 6.5.0b has documented codegen
  bugs at `-O1`/`-O2` on 68k. Oniguruma already had to revert to `-O0`. Start
  safe, measure, promote only after the full test suite confirms correctness.

## Rationale

### Why libgit2 over vanilla git

The dep-auditor's verdict on vanilla git: **INFEASIBLE**, not because of
libraries but because of `fork()`/`execvp()`. Even with every optional dep
disabled (`NO_CURL NO_OPENSSL NO_EXPAT NO_LIBPCRE NO_ICONV NO_GETTEXT
NO_PTHREADS ...`), `run-command.c` has 53 fork sites with no compile-time
bypass. Building a `fork()` shim on `CreateNewProc()` would be a multi-month
research project against a ~140 KLOC upstream with no guarantee of arriving
somewhere usable.

libgit2 was designed explicitly to be embedded — GitHub ships it in
`git-gui`-alikes, in-process IDE integrations, and language bindings. It uses
callbacks, not subprocess dispatch. Hook invocation, pager piping, and editor
shelling **do not exist in libgit2** — they're the caller's responsibility.
That means we get them free, or rather we get to omit them free.

Cost: we write a CLI wrapper. Benefit: we sidestep the entire blocker class.

### Why not Fossil

The aminet-researcher's output gestured at Fossil as a "more portable
alternative." It is more portable in the sense of having fewer deps (SQLite +
zlib), but it is not a git port. Committing to Fossil means the Amiga
developer's repo format is incompatible with every other computer they own.
The whole point of porting a VCS to Amiga is interop with the 99% of the
world's code that lives in git repositories.

Fossil is a good future port for its own sake — it's an excellent tool. But it
doesn't satisfy the user problem this PDR addresses.

### Why not sgit (SimpleGit)

sba1/simplegit is libgit2-based and OS4-native, which is exactly the direction
this PDR proposes, just for the wrong target. Porting sgit from OS4/PPC to
OS3/68k would mean:
- Retargeting the build system away from clib2+OS4-specific APIs
- Retargeting libgit2 away from OS4's newlib+pthreads
- Rewriting sgit's (already-thin) CLI against bebbo-gcc + libnix + amiport
  shims

At that point we are doing the amigit work anyway and gaining very little from
the sgit starting point. The libgit2 port is the vast majority of the effort
and that port is different on 68k vs PPC. We should study sgit's CLI as a
reference implementation (what commands did sba1 expose?) but not fork it.

### Why 040/060 only

The 68020 baseline is inappropriate for git. zlib inflate on a 14 MHz 68020 is
slow to the point of unusability even on small working trees. Git's
content-addressed storage means every object access is an inflate. A 68040/25
is the minimum realistic floor, and 060/50 is where the tool starts feeling
like a tool instead of a tech demo.

This is the first amiport port to set a minimum CPU above the project default.
Precedent is important: future ports (media players, compilers, larger
interpreters) will need the same. Establishing the pattern now — with a
well-justified requirement, documented in the .readme, surfaced in the catalog
metadata, and reflected in the package browser — makes those later ports
easier.

### Why not support network transports

- No AmigaOS 3.x libcurl port exists, and the one in Geek Gadgets is ancient,
  unmaintained, and incompatible with modern TLS
- amiport's `http-shim` is HTTP/1.0 GET-only. Smart HTTP is POST + chunked
  transfer-encoding + long-poll. The delta between what we have and what we'd
  need is a full HTTP/1.1 client
- No AmigaOS 3.x libssh2 port exists. Porting one requires crypto primitives
  we don't have and would need AmiSSL (with its `libamisslauto` hard-dep
  pitfall) or mbedTLS
- The git dumb-HTTP protocol is deprecated upstream and no longer served by
  GitHub, GitLab, Bitbucket, or Codeberg

Deferring network transports entirely is the right call for v1. If there is
demand for a v2, amiget's bsdsocket-shim work and any future HTTP/1.1 port
become dependencies; cloning from Codeberg over http (if they still support
smart-HTTP-over-HTTP) is the first target.

## Success Criteria

### v1 ship criteria

- [ ] `lib/zlib/` builds clean at `-O0` against bebbo-gcc, linked from
      `libz.a`, tests pass in vamos and FS-UAE
- [ ] `lib/libgit2/` builds clean at `-O0` against bebbo-gcc, linked from
      `libgit2.a`, libgit2's own test suite (reduced subset — no network, no
      threading) passes in vamos and FS-UAE
- [ ] `ports/amigit/` binary under 3 MB
- [ ] `amigit init && amigit add . && amigit commit -m "." && amigit log`
      round-trip works on an empty repo
- [ ] A real-world repo (the amiport repo itself, pulled to Amiga via smbfs
      or adfmount) can be `amigit log`'d, `amigit diff`'d, `amigit show`'d,
      and `amigit blame`'d without crashes
- [ ] `amigit status` correctly identifies modified/added/untracked/deleted
      files in an amiport working tree on SFS
- [ ] `amigit commit` on top of an existing history produces a commit object
      that is bit-identical to the same operation on the host (SHA-1 match —
      the sanity check that we got every normalization, LF handling, and
      tree-ordering rule right)
- [ ] FS-UAE test suite >= 25 cases covering all subcommands + error paths
- [ ] Memory-checker pass with zero CRITICAL findings (amigit leaks on exit
      are acceptable per pitfall — libgit2 cleanup is called in atexit)
- [ ] Perf-optimizer pass applied; no CRITICAL/HIGH remaining
- [ ] Real-hardware validation on Duncan's A2000 + Vampire V2 500+ (from
      user-memory: known good test bed) before amiport publication
- [ ] .readme clearly states: 68040/060 required, 16 MB Fast RAM recommended,
      SFS/PFS3 required, no network operations
- [ ] PORT.md documents every semantic adaptation (exec bit, symlinks, case,
      line endings) so future maintainers understand the tradeoffs

### Non-criteria (explicitly)

- Does **not** need to pass libgit2's full upstream test suite. The threading,
  network, and POSIX-semantic tests will fail and that's expected.
- Does **not** need to interop with `git clone` — the user is responsible for
  getting the repo onto the Amiga
- Does **not** need to support every libgit2 C API — only the surface `amigit`
  actually calls

## Alternatives Considered

### Vanilla git with `NO_*` flags and a `fork()` shim

Rejected. The dep-auditor's analysis shows `run-command.c` has 53 fork call
sites spread across hook invocation, pager piping, editor shelling, the
`git-<subcommand>` dispatch model, credential helpers, and merge/diff drivers.
There is no `NO_FORK` compile flag. Building a shim on `CreateNewProc()` is a
months-long research project with very high execution risk against a
~140 KLOC moving target.

### Fossil

Rejected as a **replacement** for this PDR (but worth porting separately).
Fossil is portable and has fewer deps (SQLite + zlib + built-in HTTP server),
but it does not solve the user problem of **interoperating with git
repositories**, which is the reason anyone on an Amiga wants a VCS in 2026.

### Port sgit from AmigaOS 4 / PPC

Rejected. sgit is itself a libgit2 wrapper, so porting it means (a) porting
libgit2 to 68k — the hard part — and then (b) retargeting sgit's thin CLI
layer from clib2 + OS4 APIs to libnix + amiport shims. Step (a) is 95% of the
effort. Step (b) is easier if we write our own CLI from scratch against our
own shim conventions. Do read sgit's source as a reference for what the CLI
should expose, but do not fork from it.

### Bypass libgit2 and write our own repo parser from scratch

Rejected. Git's on-disk format — pack file encoding, delta compression,
bitmap indexes, reachability algorithms, index v2/v3/v4 formats, commit graph
files — is a significant body of work to implement correctly. libgit2 is
18 years of bug fixes we'd otherwise have to reproduce. A hand-rolled
read-only viewer is conceivable as a 1 KLOC weekend project, but not a
read-write VCS.

### Wait for mbedTLS + libcurl + libssh2 ports, then ship vanilla git

Rejected as a v1 path. Each of those three is a significant port in its own
right (months of work), and at the end we still hit the fork/exec blocker.
The dependency chain is too long, and the destination is a tool that doesn't
work anyway.

### Ship amigit without libgit2 — thin read-only viewer

A plausible v0.5: a tool that can only `amigit log`, `amigit show`, `amigit
cat-file`, `amigit ls-tree`, `amigit diff <a> <b>`. No write path. Tiny code
footprint (~3 KLOC + zlib), no libgit2 dependency. Useful for Amiga
developers who mirror a repo to the machine and want to browse history.

Deferred, not rejected. If the libgit2 port encounters unrecoverable
blockers (codegen bugs, size bloat, a critical API that can't be disabled),
fall back to this as v0.5 and ship it as `amigit-ro`. Document now as a
credible fallback so the effort is not wasted if libgit2 fails.

## Phased Delivery Plan

**Phase 1 — zlib.** Set up `lib/zlib/`, run the amiport pipeline, build `libz.a`,
unit tests (inflate/deflate round-trip on small and large buffers), vamos +
FS-UAE verification. Ship as a standalone library — any future port needing
compression uses it. **Success gate:** libz.a links into a trivial test
program, decompresses a known gzip file correctly, passes memory-checker.

**Phase 2 — libgit2.** Set up `lib/libgit2/`, hand-write the Makefile based on
its CMakeLists, disable every optional subsystem, build `libgit2.a`. Run
libgit2's reduced test suite (no network, no threading) on both vamos and
FS-UAE. Measure binary size and working-set memory. **Success gate:**
`libgit2.a` links, a trivial libgit2 test program can `git_repository_init`
and `git_repository_open` on an FS-UAE working directory without crashing.

**Phase 3 — amigit CLI.** Write `ports/amigit/` against the libgit2 API.
Start with the read-side commands (log, show, status, diff, cat-file,
ls-tree, rev-parse). Then the write-side (init, add, commit, branch,
checkout, tag, merge). **Success gate:** the round-trip bullet in Success
Criteria passes.

**Phase 4 — Real repo testing.** Copy the amiport repo to Amiga, run amigit
against it, verify blame/log/diff/show output against the host's git (SHA-1
object IDs must match exactly). Fix all discrepancies — each one is a
semantic adaptation bug. **Success gate:** a full round-trip commit on
amiport's repo produces an object that `git fsck` accepts on the host.

**Phase 5 — Pipeline reviews.** memory-checker, perf-optimizer, hardware-
expert (for the Fast RAM assumptions), code review per `/review-amiga`.
Apply all CRITICAL/HIGH findings. Rebuild, retest.

**Phase 6 — Publish.** amiport-publisher + amiget installability + PORTS.md +
README.md. Announce in Aminet and on eab.abime.net / Amigans.net.

**Total estimated scope:** 3–5 weeks of agent-driven work assuming no
showstoppers in Phase 2 (libgit2 codegen issues are the main risk).

## Open Questions

- **libgit2 version pin.** 1.8.x or 1.9.x? 1.9 starts assuming more POSIX;
  1.8 is the last "classic" release. Defaulting to 1.8.4 but the build-manager
  should verify during Phase 2 analysis.
- **blame support.** libgit2's blame implementation is heavy. Is it worth
  the binary-size cost? Decision deferred to Phase 3 after measuring.
- **Stash support.** Stash uses a fake commit + special refs. Feasible via
  libgit2 but adds surface area. Deferred decision.
- **Working-tree diff vs index diff.** libgit2 computes diffs via its own
  algorithms; do we need to match git's output byte-for-byte? Policy: unified
  diff format, but not byte-identical whitespace handling. Document.
- **How does the user transfer a repo to the Amiga in the first place?**
  Not amigit's problem, but we should document options in PORT.md:
  (a) tar+adfmount, (b) smbfs to a host share, (c) `amiget` once it exists,
  (d) physical media.

## References

- Dependency audit (dep-auditor agent, 2026-04-12) — the full INFEASIBLE
  analysis for vanilla git
- Aminet research (aminet-researcher agent, 2026-04-12) — sgit, Fossil, and
  alternative VCS gap analysis
- [libgit2 documentation](https://libgit2.org/) — upstream API reference
- [libgit2 GitHub](https://github.com/libgit2/libgit2) — source and CMake
  config to translate to Makefile
- [sba1/simplegit](https://github.com/sba1/simplegit) — reference
  implementation for OS4/PPC, CLI design inspiration
- [git upstream](https://github.com/git/git) — `Makefile` (for the `NO_*`
  flag catalogue), `Documentation/technical/` (for the on-disk repo format
  spec, referenced when validating bit-identical commit SHAs)
- crash-patterns.md — bebbo-gcc `-O1`/`-O2` codegen warnings (why we default
  to `-O0` for lib/libgit2/)
- PDR-009 — hardware expansion context (why 040/060-only is the right
  baseline for this class of port)
- known-pitfalls.md `libamisslauto.a Is a Hard Runtime Dependency` — why we
  don't pull crypto libs we don't need
