# amigit -- post-v1 handoff and evolution roadmap

**For:** the next session continuing amigit evolution after the 0.1 v1
release (commit `8faf039`, 2026-04-13).
**Replaces:** the prior Phase 3c handoff brief (kept in git history via
`0ade298` and earlier).

Read this file first in a fresh session, then the files it points to.
The v1 release is shipped -- don't re-do it. This doc is a planning
document for where to go next, with honest effort estimates.

## Current state snapshot

- **amigit 0.1 shipped** at commit `8faf039` on `origin/main`.
- 81/81 FS-UAE functional tests green (`make test-fsemu TARGET=ports/amigit`).
- 79/79 libgit2 Stage 5 tests still green on vamos
  (`make -C tests/libgit2 run`).
- memory-checker CLEAN, perf-optimizer CLEAN.
- Binary: 1,085,544 bytes (1.06 MB), `-m68000 -O0 -noixemul`.
- LHA packages:
  - `ports/amigit/amigit-0.1.lha` (510,782 bytes, sha256 fefcab64...)
  - `ports/amigit/amigit-0.1-machine.lha` (450,837 bytes, sha256 53da172b...)
- Website entries: `PORTS.md`, `README.md`, `data/catalog.json`,
  `site/data/catalog.json`, `site/data/packages/amigit.json`.

## Still-required local vamos monkey-patches (NOT in git)

From the Phase 3 handoff -- these MUST be re-applied after any
`pip upgrade amitools` or fresh clone-install of amitools. They are
prerequisites for `make -C tests/libgit2 run` to pass 79/79.

**File:** `~/.pyenv/versions/3.14.4/lib/python3.14/site-packages/amitools/vamos/lib/DosLibrary.py`

**Patch 1:** raise DOS lock table from 1024 to 65536. Near line ~99:
```python
self.lock_mgr = LockManager(ctx.path_mgr, self.dos_list, ctx.alloc, ctx.mem, max_locks=65536)
```

**Patch 2:** `SetFileDate` must not raise `FileNotFoundError`. Wrap the
`os.utime` call in the `SetFileDate` method (~line 539-558) in
`try/except (FileNotFoundError, OSError)` that sets
`ERROR_OBJECT_NOT_FOUND` and returns DOSFALSE.

After both patches:
```bash
rm ~/.pyenv/versions/3.14.4/lib/python3.14/site-packages/amitools/vamos/lib/__pycache__/DosLibrary.cpython-314.pyc
```

Verify with `make -C tests/libgit2 run` -- should print 79/79 in ~2s.

## v1 scope delivered

All 11 commands (10 PDR-010a v1 + `version`):

| Command | Status |
|---|---|
| `version` | shipped |
| `init [--bare] [path]` | shipped |
| `status [-s]` | shipped |
| `log [-n N] [--oneline]` | shipped |
| `show <ref>` | shipped |
| `diff [--cached]` | shipped |
| `add <path>...` | shipped |
| `commit -m <msg>` | shipped (single-word msg only -- see limitations) |
| `checkout <ref>` | shipped |
| `branch [-l\|-d] [name]` | shipped |
| `tag [-l] [name]` | shipped (lightweight only) |

## Known limitations from v1 (and how to attack them)

Ranked by user value / effort ratio.

### Cheap wins (hours to 1 day)

1. **`-O1` promotion of port TUs** -- perf-optimizer (2026-04-13) audited
   all 13 TUs SAFE for -O1. No struct-by-value returns > 8 bytes, no
   recursion, no float division in amigit's own code. Binary may shrink
   slightly and log/status may run marginally faster.
   - Risk: low. Re-run full 81-test suite after the flag flip.
   - Effort: 5 minutes + 1 FS-UAE test cycle (~7 min).

2. **`-F <file>` flag for commit messages** -- solves the single-word
   commit-message limitation by reading from a file. libgit2's
   `git_commit_create_v` doesn't care about argv quoting; the message
   string can have any bytes including newlines.
   - Usage: `amigit commit -F T:commitmsg.txt`
   - Effort: 1-2 hours, including argparse + the inrepo-setup wrapper
     for testing.
   - Test plan: add 3 new TEST blocks (happy path, missing file, empty
     file) to `test-fsemu-cases.txt`.

3. **`fputs` + `putchar` in `cmd_log.c`** -- replace the `printf` per
   commit with `fputs(sha); putchar(' '); fputs(summary); putchar('\n');`.
   Saves ~30-50% of the print-path cycles per commit. Marginal for
   small repos, noticeable for logs with hundreds of commits.
   - Effort: 15 minutes + test run.

4. **Consolidate `is_help_flag` to `amigit.c`** -- 11 duplicated copies
   waste ~500 bytes of binary and pressure the A1200's 256-byte I-cache.
   Declare non-static in `amigit.c`, extern in `amigit.h`, remove the
   static copies from each `cmd_*.c`.
   - Effort: 20 minutes + test run.

5. **Local-only `clone` command (`file://` URL)** -- `git_clone` with a
   local path (or `file://` URL) copies a repo from one local directory
   to another. No network. Exercises the libgit2 clone machinery as a
   dry run for future real networking.
   - Requires re-enabling `src/libgit2/clone.c` in the `lib/libgit2/`
     build (it's currently excluded in Phase 2 pruning). Also needs
     `git_remote_create_anonymous` to resolve.
   - Stub updates: drop `git_clone__submodule` stub from
     `ported/amigit_libgit2_stubs.c` if it pulls in too much; keep the
     `git_remote_*` stubs since local clone doesn't use remotes.
   - Effort: ~1 day including library rebuild, stubs adjust, new cmd TU,
     tests.

6. **`remote list/add/remove` commands** -- re-enable `git_remote` in
   the library build, drop the `git_remote_*` stubs, add a new
   `cmd_remote.c`. Useful for setting up remotes even before networking
   works (`git config -e`-style setup).
   - Effort: ~1 day.

### v1.1 features (days to 1-2 weeks)

7. **Fast-forward merge (`amigit merge <ref>` when HEAD is ancestor)**
   -- the simplest merge case: target commit is a descendant of current
   HEAD, just update HEAD. libgit2 makes this one call
   (`git_merge_analysis` + `git_reference_set_target`). No conflict
   machinery needed.
   - Full merge with conflicts is v1.2+.
   - Effort: ~2 days.

8. **Annotated tags (`git_tag_create`)** -- upgrade `cmd_tag.c` to
   support `-a -m <msg>` for annotated tags with a signature. Same
   single-word message constraint until `-F <file>` lands.
   - Effort: half a day.

9. **Commit history search (`log --grep`, `log -S`)** -- libgit2 has
   `git_revwalk` + an iteration filter. Walk commits, grep messages,
   print matches.
   - Effort: 1-2 days.

10. **Pager auto-detect** -- if `IsInteractive(Output())` is true AND
    the output is long, pipe into AmigaDOS `More` via a temp file.
    Needed for log/show on real commits.
    - Effort: 1-2 days (+ ensuring the pager path doesn't break the
      test harness's stdout capture).

### v1.2 features (weeks)

11. **Full merge with conflict markers** -- `git_merge_commits` +
    conflict writing to the working tree. Non-trivial UX work.

12. **Rebase (non-interactive)** -- `git_rebase_init` + iteration.
    Requires merge machinery first.

13. **Stash** -- `git_stash_save` + pop. Depends on merge.

14. **Submodules** -- `git_submodule_*` APIs. UX question: how do users
    init submodules without network?

## The big-ticket items: CPU bump and networking

These are linked. Let me walk through each honestly.

### CPU bump: `-m68000` -> `-m68020`

**Current state:** amigit and all its dependencies (libgit2.a, libz.a,
libamiport.a) are built `-m68000`. This is a hard constraint for vamos
smoke testing, since vamos's default CPU is 68000 and anything with
68020+ instructions crashes with `ALERT: code=00068020`.

**What bumping to -m68020 would change:**
- libgit2 at 68020 can use 32-bit word ops natively, `muls.l` long
  multiplies in 1-2 cycles instead of subroutine calls, `extb`/`extw`,
  register-indirect indexing modes with scaled offsets. Expect
  ~1.5-2x speedup on compute-bound paths (diff content scanning,
  xdiff delta, pack object parsing).
- zlib inflate already benefits the most from 68020+ because inflate
  is the hot path for reading objects. ~2x speedup realistic.
- libgit2 hash (SHA-1, SHA-256) already has 32-bit integer ops that
  map efficiently to 020's `lsl.l #N` shifts.
- Binary size barely changes -- 68020 instructions are mostly the
  same length.

**What it costs:**
- **vamos smoke test breaks.** `make test TARGET=ports/amigit` would
  crash unless we set `VAMOS_CPU = 68020` (already a defined env var
  in common.mk; wget already uses it). Documented precedent.
- **The `lib/libgit2/` and `lib/zlib/` rebuilds cascade.** You'd need
  to either:
  - **(A)** Rebuild lib/libgit2/ and lib/zlib/ fully at -m68020 and
    lose -m68000 compatibility everywhere. Breaks any future port
    that wants to stay 68000-clean. Not recommended.
  - **(B)** Add a dual-CPU build flavor: `libgit2-020.a`, `libz-020.a`
    alongside the current 000 versions. Doubled build time, doubled
    disk footprint, same test surface. The Makefile template:
    ```make
    libgit2-020.a: $(OBJS_020)
    libgit2.a: $(OBJS_000)
    ```
    Each object rule compiles twice with different CFLAGS.
  - **(C)** Ship dual amigit binaries: `amigit-000` (current) and
    `amigit-020` (new, faster, requires A1200+). Package as two
    separate LHAs: `amigit-0.2.lha` (000) + `amigit-0.2-020.lha`
    (020). amiport install picks the right one at install time.

**Recommendation:** Option **B + C combined**. Treat 68020 as a
secondary build flavor, keep 68000 primary. Dual LHA.

**Effort:** ~1 full day to set up the dual-flavor build pipeline for
lib/libgit2/, lib/zlib/, and lib/posix-shim/. Plus running the full
81-test suite on the 020 variant (which means adding a
`VAMOS_CPU = 68020` override to the test harness conditionally).
Plus publisher updates.

**Tangible user benefit:** Honestly marginal for v1 operations.
I/O-bound commands (init, status, commit, log for small repos) won't
speed up much. Where you'd see it: `diff` on large files, `log` on
repos with hundreds of commits, and any future blame/grep commands.

**Where CPU bump IS non-negotiable:** networking. AmiSSL requires
-m68020 (per `ports/wget/Makefile` which sets this explicitly).
Meaning: **if you want clone/fetch/push over HTTPS, you have to bump
the CPU.** That's the forcing function.

### Networking: what it actually takes

The prior art is already in the repo. Phase 4 ("Network transports")
was scoped in PDR-010 as a future milestone, and several pieces exist:

- `lib/bsdsocket-shim/libamiport-net.a` -- POSIX sockets over
  `bsdsocket.library`. Proven by wget 1.20.3. This is layer 1: raw
  TCP. **Already done.**
- `lib/amissl-sdk/` -- AmiSSL headers and stub libs for HTTPS.
  **Already exists.** wget links against it. Known pitfall from
  wget port: do NOT use `libamisslauto.a` (auto-opening constructor
  crashes if AmiSSL not installed). Do manual `OpenLibrary("amisslmaster.library", ...)`
  at runtime so non-AmiSSL users get a graceful "HTTPS not available"
  fallback.
- `lib/http-shim/` -- HTTP/1.0 GET client library. Useful for
  `amiport fetch` style metadata lookups. Not sufficient for git
  smart-HTTP (which needs POST + streaming + chunked responses).

**What's missing and how hard each piece is:**

**Layer 2: libgit2 HTTPS smart-protocol transport**

This is the bulk of the work. git uses "smart HTTP" protocol: fetch
calls `GET /info/refs?service=git-upload-pack` to discover refs,
then `POST /git-upload-pack` with a pack-negotiation stream.

Two implementation paths:

**Path A: Re-enable libgit2's built-in HTTPS transport with
OpenSSL/mbedTLS backend.**
- libgit2's `src/libgit2/transports/httpclient.c` is disabled in
  Phase 2 pruning. Re-enabling requires linking against OpenSSL
  (`-lcrypto -lssl`) or mbedTLS at build time.
- **Problem:** We don't have OpenSSL for 68k. We have AmiSSL, which
  IS OpenSSL 3.x runtime-loaded via OpenLibrary. But AmiSSL's API
  surface is a subset of raw OpenSSL, and libgit2's httpclient
  expects direct `SSL_*`, `BIO_*`, `X509_*` symbols resolved at
  link time. You can't statically link against a runtime library.
- Workaround: write a shim that exposes the OpenSSL symbols libgit2
  wants and delegates each to AmiSSL's runtime-opened functions.
  That's ~150 symbol shim -- substantial plumbing but mechanical.
- **Effort:** 1-2 weeks for the shim + integration + testing.

**Path B: Write a custom libgit2 smart-HTTP transport backend.**
- libgit2 exposes `git_transport_smart` + `git_smart_subtransport`
  as extension points. You register a backend function, libgit2
  calls it for each protocol operation, you do the HTTP yourself.
- **Advantage:** You control the HTTP stack entirely. No link-time
  OpenSSL dependency. Use AmiSSL directly for TLS via its native API.
- **Disadvantage:** You rewrite the git smart-HTTP state machine
  from scratch. Stream read/write, ref discovery parsing, side-band
  protocol, progress reporting, content-length vs chunked, ...
- Reference: https://libgit2.org/libgit2/#HEAD/group/transport --
  `git_smart_subtransport_definition` is the registration point.
- **Effort:** 2-3 weeks for a working read-only fetch, +1 week for
  push (receive-pack). Includes writing an HTTP/1.1 client that
  handles chunked transfer encoding and keepalive.

**Recommendation:** Path B. Cleaner architectural fit, no fragile
OpenSSL-ABI shim, and the result is a self-contained transport TU
that other libgit2-consumer ports can reuse.

**Layer 3: Authentication**
- GitHub requires personal access tokens (PATs) for HTTPS as of 2021.
- libgit2 supports credential callbacks via `git_credential_userpass_plaintext_new`.
- User flow: first `amigit clone`, prompts for token (or reads from
  `ENV:GIT_HTTP_TOKEN`), sends it in HTTP Basic Auth header.
- **Effort:** 1-2 days on top of Layer 2.

**Layer 4: SSH (libssh2 port)**
- This is the other half of networking. Most people with "real" git
  workflows use SSH (`git@github.com:user/repo.git`), not HTTPS+PAT.
- libssh2 is ~15 KLOC of pure C with no known AmigaOS port. Depends
  on OpenSSL-or-mbedTLS + zlib (have that).
- Needs a lib/libssh2/ full port through the library pipeline:
  source-analyzer, test-designer, memory-checker, etc.
- Then needs a libgit2 SSH transport backend (same extension point
  as Path B above but for SSH).
- **Effort:** 3-4 weeks for the libssh2 port, +2 weeks for the
  libgit2 SSH transport integration.
- **Alternative:** skip SSH entirely for v1.x, ship HTTPS+PAT only.
  Revisit SSH in v2 once the user base asks for it.

**Layer 5: git:// dumb TCP protocol**
- Port 9418, no TLS, no auth, read-only. Very few servers expose
  this anymore but it's trivially implementable from bsdsocket alone.
- Not worth doing for its own sake, but if you want to prove the
  libgit2 transport plumbing works before tackling HTTPS, this is
  the cheapest smoke test.
- **Effort:** 3-4 days for a bare-minimum clone-from-git-daemon.

### Networking: realistic tiers

**Tier 1 -- HTTPS read-only (clone, fetch)**
- Requires: `-m68020` CPU bump, lib/bsdsocket-shim (done),
  lib/amissl-sdk integration, Path B custom HTTPS transport,
  libgit2 `clone.c`/`fetch.c`/`remote.c` re-enabled in lib/libgit2/
  build, credential callback.
- User-visible: `amigit clone https://github.com/user/repo`,
  `amigit fetch origin`, `amigit pull origin main` (as a
  fetch+fast-forward-merge combo).
- **Effort: 3-4 weeks focused work.** The long pole is the
  custom smart-HTTP transport backend.

**Tier 2 -- HTTPS push (receive-pack)**
- Tier 1 + POST /git-receive-pack with pack streaming.
- Requires credential callback for push auth.
- User-visible: `amigit push origin main`.
- **Effort: +1 week on top of Tier 1.**

**Tier 3 -- SSH (libssh2 port + libgit2 SSH transport)**
- All-purpose git access via `git@host:repo.git` URLs.
- Requires lib/libssh2/ full port + libgit2 SSH backend.
- **Effort: +3-4 weeks on top of Tier 2.**

**Tier 4 -- git:// dumb TCP**
- Niche. Skip unless someone wants it specifically.

### Honest recommendation

If you want evolution momentum without committing 2 months, the order
that maximizes value is:

1. **Cheap wins (items 1-6 above).** Week of focused work. Ship 0.2.
2. **CPU bump to dual-flavor 68000/68020.** 1 day infrastructure. Ship
   0.3 as "A1200-and-up gets a faster amigit".
3. **Tier 1 HTTPS (Path B custom transport).** 3-4 weeks. Ship 1.0 as
   "networking comes to amigit". This is the watershed release.
4. **Tier 2 push.** 1 week. Ship 1.1.
5. **Tier 3 SSH.** Defer until user demand is proven post-1.1.

Each of those is a coherent release with a clear user-visible
improvement. The cheap wins give you momentum, the CPU bump sets up
the networking prerequisite, and Tier 1 lands the actual dream.

Total time to a fully-networked `amigit clone https://github.com/...`:
**roughly 6-8 weeks of focused work**, assuming one long session per
working day and the library pipeline doesn't discover another
`__divsf3`-style surprise.

If you want to shortcut and ship HTTPS faster: the item-1 and
item-2 work is optional; you could jump straight from the current 0.1
to Tier 1 HTTPS in a single 3-4 week push. The tradeoff is higher
risk since you'd be doing the CPU bump + networking in the same
release.

## Open questions for a future session

1. **License clarification.** amigit is "amiport-native" and I marked
   the catalog entry as "GPL-2.0 (libgit2) + MIT (amiport code)".
   libgit2 is actually GPLv2-with-linking-exception, NOT pure GPLv2 --
   that's important for linking. Verify the license chain before
   public distribution (Aminet, amiport site publication).
2. **Aminet submission.** The v1 LHA exists but is NOT on Aminet.
   Before submission, run the aminet-publisher agent which will:
   - Validate the .readme format (40-char Short, ASCII only)
   - Check for hallucinated `Replaces:` field
   - Upload via FTP to `ftp://main.aminet.net/new/`
3. **Real hardware verification.** The user has a real A2000 + Vampire
   V2 500+ + X-Surf 100 + Roadshow setup. Nobody has actually run
   amigit on silicon yet -- everything is FS-UAE A1200/68020. First
   real-hardware run may expose Vampire AC68080-specific surprises
   (the V2 Gold core has some 68020/060-superset instructions that
   differ from pure 060).
4. **EAB "is this actually first?" post.** Before claiming "first git
   client for OS3" publicly, a 10-minute post to english-amiga-board
   coding forum asking "am I missing prior art?" is cheap insurance.
5. **HANDOFF.md cleanup policy.** The prior Phase 3 handoff brief is
   still in git history. Should we establish a convention of
   archiving old handoffs to `ports/amigit/handoffs/` or just letting
   git history hold them?

## Reference implementations and patterns

When evolving amigit, these are the canonical patterns:

- **CPU bump template:** `ports/wget/Makefile` shows -m68020 + AmiSSL
  + bsdsocket-shim link pattern.
- **Library pipeline template:** `lib/libgit2/Makefile` (prune rules),
  `lib/zlib/Makefile` (dual-file HOTPATH_CFLAGS promotion).
- **Custom libgit2 transport backend:** libgit2 upstream has
  `tests/libgit2/transport/` and `tests/libgit2/remote/transports.c`
  as registration examples. Study before starting Tier 1.
- **AmiSSL runtime OpenLibrary pattern:** `ports/wget/ported/src/` --
  search for `OpenLibrary.*amisslmaster` to see the manual library
  lifecycle (not the auto version).
- **Command TU template:** `ports/amigit/ported/cmd_status.c` for
  CWD-repo commands, `ports/amigit/ported/cmd_init.c` for
  user-path commands.

## Regression test commands (known-good)

```bash
# libgit2 Stage 5 (vamos, needs monkey-patches)
make -C tests/libgit2 run                     # 79/79 in ~2s

# amigit full functional suite (FS-UAE, ~7 min)
make test-fsemu TARGET=ports/amigit           # 81/81

# vamos smoke test of a single amigit command
VAMOS_MEM=4096 toolchain/scripts/vamos -s 256 ports/amigit/amigit version
```

## Fresh-session continuation prompt

Copy this block verbatim into a fresh session after `/clear`:

```
Resume amigit evolution. Start by reading ports/amigit/HANDOFF.md
-- it has the current state (v1 shipped at commit 8faf039),
the roadmap options with honest effort estimates, and the
known-good test commands. Phase 3 (v1 release) is complete.

Current head: 8faf039 on origin/main. amigit 0.1 is shipped with
all 11 v1 commands, 81/81 FS-UAE tests green, memory-checker CLEAN,
perf-optimizer CLEAN, LHA packages built.

The vamos monkey-patches required for libgit2 Stage 5 regression
testing are still in place locally (raise DOS lock cap to 65536,
handle SetFileDate FileNotFoundError); re-apply from HANDOFF.md
section "Still-required local vamos monkey-patches" if 
`make -C tests/libgit2 run` fails with a stack trace involving
LockManager or utime.

Ask me what I want to tackle next. The options, ranked by effort
and value, are in HANDOFF.md "Known limitations" + "CPU bump"
+ "Networking" sections. Common choices:

- Cheap wins pass: -O1 promotion, -F commit message from file,
  log printf -> fputs, is_help_flag consolidation, local-file
  clone command. About 1-2 days. Ships as amigit 0.2.
- CPU dual-flavor build: 68000 + 68020 variants. 1 day
  infrastructure. Ships as 0.3. Sets up networking prerequisite.
- Full Tier 1 HTTPS networking: custom git smart-HTTP transport
  backend (libgit2 extension point) + AmiSSL integration + 68020
  CPU bump + lib/libgit2 re-enable of clone.c/fetch.c/remote.c.
  3-4 weeks of focused work. Ships as amigit 1.0.

Before starting any non-trivial work, verify the local vamos
monkey-patches are still applied (run `make -C tests/libgit2 run`
-- should print 79/79), and verify the v1 test suite still passes
(`make test-fsemu TARGET=ports/amigit` should print 81/81). If
either regresses, investigate before making changes.
```

## Phase 3c session artifacts (historical record)

Commits that built v1:

| Commit | Description |
|---|---|
| `0ca6a44` | test(lib): libgit2 Stage 5 tests 79/79 green (vamos-limited) |
| `9519f3b` | docs(lib): libgit2 Stage 6 memory audit CLEAN with 1 HIGH |
| `94fada7` | perf(lib): libgit2 Stage 7 hotpath promotion + Stage 8 |
| `d5f80d1` | docs(lib): libgit2 Stage 9 -- doc updates for Phase 2 |
| `e4d0466` | feat(port): amigit 0.1 Phase 3a proof-of-life (version + init) |
| `490839c` | docs(port): amigit Phase 3 session handoff brief |
| `d2ca06f` | feat(port): amigit 0.1 Phase 3b read-side commands |
| `0ade298` | docs(port): amigit Phase 3c session handoff brief |
| `8faf039` | feat(port): amigit 0.1 v1 release -- first working git client for AmigaOS 3.x |

Knowledge base additions (shared amiga-kb via `amiga_add_pitfall`):

Phase 2 (libgit2): 6 pitfalls (vamos lock cap, strnlen/difftime gaps,
khash -lm requirement, force-include replication, libgit2 commit
message newline, libgit2 init refcount, vamos readdir gap).

Phase 3b (read commands): 2 pitfalls (libgit2 volume path handling
`X:foo` rewrite, `amiport_realpath` POSIX `.` handling).

Phase 3c (write commands): 2 pitfalls (libgit2 patch_generate
mathieeesingbas crash via __divsf3, cmd_commit empty-initial-commit
guard).
