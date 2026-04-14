# PDR-012: amigit HTTPS networking (clone / fetch / pull / push to GitHub)

## Status

Proposed (2026-04-14). This is a multi-session track -- expect 8 to 12
focused sessions across ~4-5 weeks of elapsed time. A fresh session
should open this file FIRST, find the current phase from the "Session
checkpoint" section at the bottom, and start from there.

## Date

2026-04-14

## Problem

amigit 0.1-6 is a working local-only git client for AmigaOS 3.x. It
can init, add, commit, log, show, diff, branch, checkout, tag, and
status entire repositories -- but it cannot talk to any remote. You
cannot `amigit clone https://github.com/owner/repo`. You cannot
`amigit fetch origin`. You cannot `amigit pull origin main`. The
whole collaboration value of git is missing.

This means amigit is a useful proof-of-concept but not a useful tool.
To walk the history of a real codebase on a real Amiga, you have to
`scp` the `.git/` directory over from a Linux machine first. That is
not where the project has to stay.

## Target users

Retro computing enthusiasts running AmigaOS 3.x on accelerated hardware
(Vampire V2/V4, A1200 030+, A3000/A4000, FS-UAE A1200 with 68020) who
want to interact with public and private GitHub repositories directly
from their Amiga. Primary initial use cases:

1. **Clone a public GitHub repo read-only** -- browse the history of
   an open-source project without a Linux machine in the middle.
2. **Clone a private repo with a personal access token** -- work on
   your own projects from a real Amiga.
3. **Pull incremental updates** -- keep a local mirror of a repo
   up to date as upstream evolves.
4. **Push commits back** -- contribute changes from the Amiga.

## Decision

Implement a custom libgit2 smart-HTTP transport backend that uses
`lib/bsdsocket-shim/libamiport-net.a` for TCP and AmiSSL (runtime-
loaded via `OpenLibrary("amisslmaster.library", ...)`) for TLS. Wire
it up to libgit2's `git_clone`, `git_remote_fetch`, and
`git_remote_push` consumer APIs by un-pruning the network source files
that Phase 2 of the libgit2 port (see PDR-010) deleted. Ship the result
as amigit 0.2 (clone + fetch + pull, Tier 1) and amigit 1.0 (push,
Tier 2).

This is **Path B** from the ports/amigit/HANDOFF.md networking analysis
-- write a custom `git_smart_subtransport_definition` rather than try
to revive libgit2's upstream `transports/http.c` with a fragile
OpenSSL-symbol-shim against AmiSSL. Path B is a self-contained new
translation unit; Path A would require writing a ~150-symbol static
shim against a runtime-loaded library, which is the exact failure mode
the wget port deliberately avoids (`libamisslauto.a` vs manual
`OpenLibrary` -- known-pitfalls.md).

## Prerequisites -- already met as of 0.1-6

All four layers needed for this work already exist in the tree:

- **CPU baseline**: amigit is `-m68020` linked against `-m68020`
  libraries (Track A, PDR-010 / commit 8858057). AmiSSL requires
  68020+; we are already there.
- **Raw TCP**: `lib/bsdsocket-shim/libamiport-net.a` gives POSIX
  sockets over `bsdsocket.library`. Proven by `ports/wget/wget`
  (1.20.3-2 shipping). Connect, send, recv, close all working.
- **TLS**: `lib/amissl-sdk/` has the AmiSSL headers and stub libs.
  `ports/wget/` already demonstrates the manual-OpenLibrary
  integration pattern and the HTTP-fallback graceful degrade when
  AmiSSL is not installed. Do NOT use `libamisslauto.a` (crashes at
  process start if AmiSSL is missing -- known-pitfalls.md).
- **HTTP plumbing inspiration**: `lib/http-shim/libhttp-shim.a` is
  a working HTTP/1.0 GET client on top of bsdsocket-shim, used by
  `ports/amiport/` for package-index fetching. It is NOT sufficient
  for git smart-HTTP (need HTTP/1.1 keepalive + chunked transfer
  encoding + POST body streaming) but it proves the stack and gives
  us a reference for socket lifecycle, timeout handling, and
  progress callbacks.

## Key technical decisions (locked -- do NOT re-debate)

1. **Custom smart-HTTP transport backend (Path B), not revived
   libgit2 transports/http.c (Path A).** Rationale: Path A requires
   a static OpenSSL API shim against runtime-loaded AmiSSL. Path B
   is a single new TU that plugs directly into libgit2's
   `git_smart_subtransport_definition` extension point and talks to
   AmiSSL's native API directly. Path B is also portable to any
   future libgit2-consuming port.

2. **Native libgit2 consumer API (git_clone / git_remote_fetch /
   git_remote_push), not amigit-written replacements.** Rationale:
   upstream libgit2's `clone.c` / `fetch.c` / `remote.c` handle
   dozens of edge cases (interrupted fetch, resume, multi-pack,
   packfile verification, credential callbacks, progress reporting)
   that we would otherwise have to re-solve. Un-pruning those source
   files in Phase 1 is mechanical; writing replacements would take
   weeks and introduce bugs.

3. **HTTP/1.1 from scratch, not a third-party library.** Rationale:
   git smart-HTTP needs POST body streaming, chunked transfer
   encoding, keepalive, and side-band-64k parsing. No existing
   amiport HTTP library covers all four. Writing it ourselves is
   ~800 lines and is fully testable on vamos for the parser half
   and on real hardware for the network half.

4. **Smart-HTTP protocol only at 1.0. Not dumb-HTTP, not SSH, not
   git://.** Rationale: dumb-HTTP is deprecated by GitHub (clone
   rejected). SSH requires porting libssh2 (~3-4 weeks on its own).
   git:// port 9418 is niche and not supported by GitHub at all.
   Smart-HTTP is the one protocol GitHub accepts for clone/fetch/
   push, so it is the one we must ship.

5. **HTTPS only. No plain HTTP.** Rationale: GitHub redirects
   `http://` to `https://`, and public Git servers worth cloning
   all require TLS. Plain HTTP support would require handling
   redirects and is more code for a path nobody uses. If AmiSSL is
   not installed, fail with a clear "HTTPS not available (AmiSSL
   not installed)" error.

6. **Credential model: `ENV:GIT_HTTP_TOKEN` or stdin prompt.**
   Rationale: GitHub mandates personal access tokens for HTTPS
   since 2021. Reading from an AmigaDOS env var keeps secrets out
   of argv (AmigaDOS has no shell history but argv is visible to
   other tasks via `FindTask`). Prompting on stdin is the fallback
   for interactive use. No plaintext-on-disk credential store at
   1.0 -- defer to a future revision.

7. **Dual-flavor library build stays intact.** Phase 1 un-prunes
   libgit2 network sources INTO BOTH the `libgit2.a` (000) and
   `libgit2-020.a` (020) archives. The 000 flavor still compiles
   and the tests/libgit2 Stage 5 test suite still passes 79/79.
   The 000 flavor is still what every non-amigit port links against.
   The 020 flavor gets the bigger binary (network code adds weight)
   but that is the flavor amigit consumes, so the cost is isolated.

8. **Memory discipline is load-bearing.** AmigaOS with `-noixemul`
   has no process memory cleanup on exit. Every malloc'd buffer
   (HTTP request, HTTP response, pkt-line frame, pack chunk, URL
   parse, credential) must be tracked for atexit cleanup OR reused
   via a pool. Memory-checker agent is mandatory at every phase.

9. **No vamos for network tests.** vamos cannot open
   `bsdsocket.library`. Network tests happen on FS-UAE with
   bsdsocket passthrough OR on real hardware. Protocol-layer unit
   tests (pkt-line encoder, URL parser, HTTP header parser) are
   pure-C and run fine under vamos -- use them aggressively to get
   fast iteration.

10. **Real hardware is the ultimate test rig.** Duncan has A2000 +
    Vampire V2 500+ + X-Surf 100 + Roadshow. The 1.0 clone-from-
    github smoke test must run on that stack. FS-UAE + bsdsocket is
    a useful stepping stone but is NOT the final validation.

## Phase plan

Each phase is sized to fit 1 to 3 focused sessions. Every phase ends
with a clean commit (or small atomic commit chain) and a working test
surface. Phases are ordered so each one produces a shippable-if-needed
artifact, though only phases 9 and 12 correspond to public releases
(0.2 and 1.0 respectively).

### Phase 1: Un-prune libgit2 network source files (1 session)

**Goal:** re-add `src/libgit2/clone.c`, `fetch.c`, `remote.c`,
`refspec.c` (if not already present), and the transport dispatch
glue that libgit2 needs to call our registered backend. Keep
`transports/http.c`, `transports/smart.c`, `transports/ssh_*.c`,
and `streams/*.c` PRUNED -- we will NOT use upstream's HTTP stack.

**Deliverables:**
- Upstream files copied from libgit2 1.8.5 tarball into
  `lib/libgit2/src/libgit2/` (clone.c, fetch.c, remote.c, others as
  linker errors dictate).
- `lib/libgit2/src/libgit2/transports/smart.c` -- this file is
  REQUIRED for the smart-transport registration machinery. Un-prune
  it along with its header.
- `lib/libgit2/src/libgit2/transports/smart_pkt.c`,
  `smart_protocol.c` -- the pkt-line framing and protocol helpers.
  Un-prune these so our custom transport can use them.
- `ports/amigit/ported/amigit_libgit2_stubs.c` -- audit and trim.
  Remove stubs for `git_clone*`, `git_remote_create*`,
  `git_remote_fetch*` (they now resolve to real upstream code).
  Keep stubs for `git_transport_http`, `git_transport_ssh`,
  `git_stream_openssl_*` (we won't use upstream's streams).
- `lib/libgit2/Makefile` -- both 000 and 020 source lists updated
  to include the new files. `libgit2.a` and `libgit2-020.a` both
  build cleanly.

**Done when:**
- `make -C lib/libgit2 dual` succeeds with no new warnings.
- `make -C tests/libgit2 run` still prints 79/79 (000 archive still
  works for Stage 5 test suite -- remember to apply the vamos
  monkey-patches per HANDOFF.md).
- `make test-fsemu TARGET=ports/amigit` still prints 87/87 (020
  archive + amigit rebuilt, no functional regressions).
- Binary size: expected ~1.15-1.25 MB (up from 1.08 MB, +70-170 KB
  from the re-enabled network source files). Verify it's in that
  range; a bigger delta means something was accidentally un-pruned
  that shouldn't have been.

**Risks:**
- Upstream's `transports/smart.c` may reference `transports/http.c`
  symbols we're keeping pruned. Expect linker errors; resolve by
  registering our transport BEFORE the first `git_remote_connect`
  call and letting libgit2's dispatch pick it up.
- New soft-float references from the un-pruned code (see
  known-pitfalls: `__divsf3` / `__floatunsisf` from
  `patch_generate.c`). Re-run the override check; the existing
  stubs in `amigit_libgit2_stubs.c` should still cover it.

### Phase 2: Transport TU skeleton + dummy registration (1 session)

**Goal:** create `ports/amigit/ported/transport_https.c` with the
`git_smart_subtransport_definition` registration boilerplate and a
stub action handler that returns "not implemented yet" for every
service. Prove amigit can call `git_remote_lookup` and
`git_remote_connect` on an HTTPS URL without crashing -- the connect
call SHOULD fail cleanly with "not implemented" but must not segfault
or leak memory.

**Deliverables:**
- `ports/amigit/ported/transport_https.c` -- new file, ~200 lines.
  Implements `git_smart_subtransport_definition` and a stub
  `git_smart_subtransport_action` that returns
  `GIT_ERROR_NOT_IMPLEMENTED` for every service verb.
- `ports/amigit/ported/transport_https.h` -- public header:
  `int amigit_transport_https_register(void);` and
  `void amigit_transport_https_cleanup(void);` (atexit hook).
- `ports/amigit/ported/amigit.c` -- call
  `amigit_transport_https_register()` from `amigit_init_libgit2()`
  right after `git_libgit2_init()`.
- `ports/amigit/Makefile` -- add `ported/transport_https.c` to
  OBJECTS.
- New TEST block in `test-fsemu-cases.txt`: "remote with https URL
  fails with not-implemented, not a crash".

**Done when:**
- amigit still builds green, still 87/87 on FS-UAE.
- The new test passes: amigit sees an HTTPS URL, routes to our
  transport, gets a controlled failure, cleans up, exits 10.

### Phase 3: Generic HTTP/1.1 client -- connect + request + response headers (2 sessions)

**Goal:** write the HTTP-level plumbing in `transport_https.c` or a
new `ports/amigit/ported/http_client.c`. No TLS yet -- test against
a plain HTTP endpoint locally. This phase builds the foundation
without the AmiSSL complication.

**Deliverables:**
- `http_client.c` / `http_client.h` with:
  - `http_conn_t` -- opaque connection struct (socket fd, host,
    port, tls flag, keepalive state, buffers)
  - `http_connect(conn, host, port, use_tls)` -- DNS resolve,
    `socket()`, `connect()` via bsdsocket-shim. If `use_tls` is
    true, fail with ENOSYS for now (AmiSSL comes in Phase 7).
  - `http_send_request(conn, method, path, headers, body, len)` --
    format request line + headers + optional body; write via
    bsdsocket `send()`.
  - `http_read_response_status(conn, &status)` -- parse HTTP/1.1
    status line.
  - `http_read_response_header(conn, name, value)` -- iterate
    headers one at a time.
  - `http_read_body(conn, buf, max)` -- read from body, respecting
    Content-Length. Chunked encoding is Phase 4.
  - `http_close(conn)` -- close socket, free buffers.
- Unit test TU: `tests/amigit-http-client/` -- vamos-compatible
  unit tests for the parser half (status line parsing, header
  parsing) driven from string literals.
- Integration test: a `TEST:` block in amigit's test suite that
  runs against `http://example.com/` (if FS-UAE has bsdsocket
  passthrough) or skipped with a documented reason.

**Done when:**
- Unit tests green on vamos.
- If FS-UAE passthrough works: one smoke request to a known plain-
  HTTP endpoint returns a parseable response.
- Memory-checker: all HTTP client buffers tracked for cleanup.
- Test: 87/87 + new unit tests.

### Phase 4: pkt-line framing + chunked transfer encoding (1 session)

**Goal:** implement the two framing layers the smart-HTTP protocol
needs. pkt-line is git's 4-byte-hex-length-prefix framing.
Chunked is HTTP/1.1's `Transfer-Encoding: chunked` body format.

**Deliverables:**
- `ports/amigit/ported/pkt_line.c` / `.h`:
  - `pkt_line_encode(buf, max, payload, payload_len)` -> bytes_written
  - `pkt_line_decode(buf, len, &payload, &payload_len)` -> bytes_consumed
  - Handles flush packet (`0000`) and delim packet (`0001`).
  - Returns -1 for truncated/invalid input.
- Extension to `http_client.c`: when response says `Transfer-
  Encoding: chunked`, read body chunk-by-chunk, stripping the
  hex-size prefixes and trailing CRLFs transparently. Caller sees
  a continuous body stream.
- Unit tests: `tests/amigit-pkt-line/` and chunked decoder tests
  driven from string literals.

**Done when:**
- Unit tests green on vamos.
- Round-trip test: `pkt_line_encode` + `pkt_line_decode` returns
  the original payload.
- Chunked decoder: hand-crafted chunked body parses correctly,
  including zero-length terminator.

### Phase 5: Service discovery -- GET /info/refs?service=git-upload-pack (1 session)

**Goal:** implement the first real git smart-HTTP interaction. Send
the initial discovery request, parse the capability line, parse the
ref advertisement. Wire the result back into libgit2's
`git_remote_ls` path.

**Deliverables:**
- `transport_https.c` action handler for `GIT_SERVICE_UPLOADPACK_LS`:
  1. Build URL: `${base_url}/info/refs?service=git-upload-pack`
  2. HTTP GET with `Accept: application/x-git-upload-pack-advertisement`
  3. Parse response: service header line, capability line, ref
     advertisement, flush pkt.
  4. Hand ref list back to libgit2 via the subtransport stream
     interface.
- `git_smart_subtransport_stream.read()` implementation -- libgit2
  expects to pull bytes from our transport one at a time, we feed
  it from our buffered HTTP response.

**Done when:**
- `amigit remote-ls https://github.com/bdgscotland/amiport`
  (new debug command, maybe called `amigit ls-remote`) lists the
  refs of a public repo on real hardware.
- Memory-checker clean.
- Parser tests exercise malformed responses without crashing.

### Phase 6: Upload-pack body -- want/have negotiation + pack reception (2 sessions)

**Goal:** the big one. Send the POST request with want/have
negotiation. Parse the side-band-64k response. Feed pack data to
`git_indexer` for writing into the object database.

**Deliverables:**
- `transport_https.c` action handler for `GIT_SERVICE_UPLOADPACK`:
  1. Build URL: `${base_url}/git-upload-pack`
  2. HTTP POST with `Content-Type: application/x-git-upload-pack-
     request` and `Accept: application/x-git-upload-pack-result`
  3. Write pkt-line framed want list + have list + flush + done
     (libgit2 computes the contents; we just need to stream the
     bytes libgit2 hands us).
  4. Read response: side-band-64k demux separates pack data
     (band 1), progress messages (band 2), and errors (band 3).
  5. Hand pack bytes to the caller's read stream so
     `git_indexer_append` can consume them incrementally.
- Full integration: `amigit clone http://local-git-daemon/repo`
  creates a working clone on disk. (Plain HTTP only for now --
  HTTPS comes in Phase 7.)
- Memory-checker: pack buffer lifecycle. Side-band-64k can
  interleave packets; buffer management is error-prone.

**Done when:**
- Clone from a plain-HTTP git server succeeds.
- The cloned repo passes `amigit log`, `amigit status`, and a
  follow-up `git status` on the Linux machine that served it.
- 87/87 functional suite still green.

### Phase 7: AmiSSL integration (1-2 sessions)

**Goal:** wire TLS into `http_client.c` so Phase 5/6 work over
`https://` URLs. Manual `OpenLibrary("amisslmaster.library", ...)`
per the wget pattern. Graceful degrade if AmiSSL is not installed.

**Deliverables:**
- `ports/amigit/ported/amissl_glue.c` -- manual OpenLibrary,
  OpenAmiSSLTags, SSL_CTX_new, SSL_CTX_set_verify, SSL_new, SSL_set_fd,
  SSL_connect, SSL_read, SSL_write, SSL_free, CloseLibrary cleanup.
  Cross-reference `ports/wget/ported/src/` for the reference pattern.
- `http_client.c` -- when `use_tls == true`, route read/write
  through the AmiSSL SSL_read/SSL_write instead of raw
  bsdsocket recv/send.
- `ports/amigit/Makefile` -- add `-lamisslstubs` (NOT
  `-lamisslauto`), add `-I../../lib/amissl-sdk/include`.
- Error path: if AmiSSL open fails, return a specific error so
  the transport can produce a clear "HTTPS not available (AmiSSL
  not installed); run `amiport install amissl` or retry with a
  different protocol" message.

**Done when:**
- First successful real-hardware test: `amigit clone
  https://github.com/bdgscotland/amiport` on Duncan's
  A2000 + Vampire + X-Surf + Roadshow + AmiSSL stack.
- Graceful degrade test: rename amisslmaster.library temporarily,
  retry, confirm the friendly error.

### Phase 8: Credential callback -- GitHub PAT auth (1 session)

**Goal:** wire up GitHub personal access tokens. Read from
`GIT_HTTP_TOKEN` environment variable first; fall back to stdin
prompt on interactive input.

**Deliverables:**
- `transport_https.c` -- `on_401_retry` path: if the first request
  returns 401 Unauthorized, invoke libgit2's credential callback,
  retry with `Authorization: Basic base64(user:token)`.
- `ports/amigit/ported/credential.c` -- `amigit_credential_callback`:
  1. Check `GetVar("GIT_HTTP_TOKEN", ...)` first.
  2. If not set, check `IsInteractive(Input())` -- if yes, prompt
     on stdout: "GitHub username: ", read from stdin; "Personal
     access token (will not be echoed): ", read with echo-off.
  3. If not set and not interactive, fail with a clear error.
- Memory-checker: credential buffers zeroed before free.

**Done when:**
- Private repo clone with PAT works on real hardware.
- Public repo clone without a token still works (no auth attempted
  unless 401).
- Interactive prompt works (visual test / manual).

### Phase 9: cmd_clone.c + amigit 0.2 release (1 session)

**Goal:** ship a real `amigit clone <url> [path]` command, update
all metadata, cut the 0.2 release.

**Deliverables:**
- `ports/amigit/ported/cmd_clone.c` -- `amigit clone <url> [path]`:
  - URL parsing (http/https scheme, host, port, path, auth)
  - Destination path resolution (default: basename of repo URL)
  - Register our transport, call `git_clone`, handle errors
  - Progress reporting via `git_remote_callbacks.transfer_progress`
- `ports/amigit/ported/amigit.c` -- add `clone` to the dispatch table
- Test suite: new TEST blocks exercising clone error paths (missing
  URL, malformed URL, unreachable host, bad credentials, AmiSSL
  unavailable). Happy path needs real hardware or FS-UAE bsdsocket
  passthrough.
- `ports/amigit/Makefile` -- bump to `VERSION = 0.2` /
  `REVISION = 1`. Yes, bump to 0.2 not 0.1-7 because HTTPS clone
  is the "can I use git on my Amiga" threshold. This is a real
  feature release.
- Version-string update in amigit.h / amigit.c.
- All metadata sweep: PORTS.md, README.md, data/catalog.json,
  site/data/catalog.json, site/data/packages/amigit.json,
  amigit.readme (rewrite the "What works today" / "What does
  not work yet" sections -- clone is now in the works-today list).
- PORT.md: new "What changed in 0.2" section.
- HANDOFF.md: strike networking items, update state snapshot.

**Done when:**
- Real hardware clone of a public GitHub repo succeeds.
- FS-UAE functional tests still green (all 87 plus whatever new
  error-path tests were added).
- Release artifacts built: `amigit-0.2.lha`, `amigit-0.2-machine.lha`.
- Committed, documented, HANDOFF updated per the "strike done items
  in the same commit" rule.

### Phase 10: cmd_fetch.c + cmd_pull.c (1 session)

**Goal:** incremental updates. `fetch` downloads new commits without
merging. `pull` is fetch + fast-forward-only merge.

**Deliverables:**
- `cmd_fetch.c`: `git_remote_lookup`, `git_remote_fetch`, update
  remote-tracking refs.
- `cmd_pull.c`: call `cmd_fetch`, then `git_merge_analysis`. If
  fast-forward is possible, update HEAD via
  `git_reference_set_target`. If not, bail with "non-fast-forward
  merge required (not implemented, use git on another machine)".
  Real merge with conflict resolution is a 1.x feature.
- Test suite: real-hardware or FS-UAE integration tests for both
  commands.
- Ship as 0.2-2 or 0.2.1 (no new commands in 0.2 main release path
  -- fetch/pull follow immediately after to close the read-only
  loop).

**Done when:**
- Incremental fetch against a remote works on real hardware.
- Fast-forward pull updates HEAD correctly.
- Non-fast-forward case fails cleanly without corrupting the repo.

### Phase 11: Stabilization pass (1-2 sessions)

**Goal:** harden the networking path. Audit memory, audit error
paths, test edge cases that weren't covered by the happy-path
development.

**Deliverables:**
- memory-checker agent dispatch across all new TUs.
- perf-optimizer agent dispatch (less important than memory, but
  pack-reception is the hottest loop in the whole binary -- worth
  auditing).
- Edge case test coverage:
  - Malformed server response at every parser boundary
  - Server 500 error, 404, 403, 429 rate-limit responses
  - Network interruption mid-fetch (socket close, timeout)
  - Pack verification failure
  - Out-of-memory during pack reception
  - Huge repo (clone linux kernel -- no, too big. Something with
    >1000 commits and >10 MB of packs is the realistic bar.)
- Security audit: no credentials leaked to logs, no secrets in
  argv, PAT-in-URL accidentally logged, etc.
- Ship as 0.2.x stabilization revisions.

**Done when:**
- All the tests above pass.
- memory-checker CLEAN.
- A clone of a medium repo (500+ commits, 5+ MB of packs) succeeds
  cleanly from real hardware and the resulting working copy passes
  `amigit status` (clean tree).

### Phase 12: Push support -- cmd_push.c + amigit 1.0 (1-2 sessions)

**Goal:** the final piece. `amigit push origin main`. Wire up
`git_remote_push` with the receive-pack service instead of
upload-pack.

**Deliverables:**
- `transport_https.c`: implement `GIT_SERVICE_RECEIVEPACK_LS` and
  `GIT_SERVICE_RECEIVEPACK` handlers. Service URL is
  `${base}/info/refs?service=git-receive-pack` and
  `${base}/git-receive-pack` respectively. Body protocol is
  different but the framing and HTTP plumbing is the same.
- `cmd_push.c`: `git_remote_lookup` + `git_remote_push`.
- Credential handling: push always needs auth, so no
  no-credentials-needed path.
- Test suite: real-hardware push to a private test repo.
- 1.0 release: this is the watershed. Full metadata sweep, new
  README+readme reframing (amigit is now "usable git for AmigaOS"
  not "preview"), aminet-publisher dispatch for Aminet upload.
- HANDOFF.md: everything except the long-term items (SSH, merge,
  rebase) is shipped.

**Done when:**
- Real hardware push succeeds.
- GitHub web UI shows the pushed commit from the Amiga.
- Release artifacts built and shipped.

## Testing strategy by phase

| Phase | vamos OK? | FS-UAE OK? | Real hw needed? |
|---|---|---|---|
| 1 | yes (libgit2 Stage 5) | yes (amigit 87/87) | no |
| 2 | yes | yes | no |
| 3 | parser tests | if bsdsocket passthrough | no |
| 4 | yes | yes | no |
| 5 | parser tests | if bsdsocket passthrough | yes (real HTTPS) |
| 6 | parser tests | if bsdsocket passthrough | yes |
| 7 | no | depends on AmiSSL in emu | **yes** (AmiSSL) |
| 8 | no | no | yes (PAT on real GH) |
| 9 | parser tests | yes for error paths | yes (ship smoke) |
| 10 | parser tests | yes for error paths | yes (ship smoke) |
| 11 | extensive | extensive | yes (final) |
| 12 | parser tests | no | yes (ship smoke) |

**Open question for Phase 3:** does the current FS-UAE test setup
have `bsdsocket.library` passthrough? If yes, we can run integration
tests against a localhost HTTP server from the emulator. If no, Phase
3 onwards is strictly unit-tests + real-hardware manual verification.
This is the FIRST thing Phase 3 should establish.

## Risks

1. **libgit2 pruning cascade.** Phase 1 un-prunes files that may
   reference other pruned files. Expect linker errors, resolve
   iteratively. Worst case: more files need un-pruning than
   expected and the binary grows more than 200 KB.

2. **smart-HTTP protocol edge cases.** Upstream git clients handle
   ~15 years of server quirks (GitHub vs GitLab vs Gitea vs
   self-hosted). We may hit a non-standard server response that
   libgit2 handles gracefully but our custom transport doesn't.
   Mitigation: test against multiple servers in Phase 11.

3. **AmiSSL version compatibility.** AmiSSL 5 (OpenSSL 3.x
   runtime) is what wget uses. If the user has AmiSSL 4 or earlier,
   our integration may not work. Detect the version via
   `amisslmaster.library` OpenVersion and refuse to run below a
   known-good minimum.

4. **Pack size vs AmigaOS RAM.** A 50 MB pack (large repo) may not
   fit in free RAM while `git_indexer` is also running. Duncan's
   real-hardware setup has plenty of Vampire RAM but FS-UAE default
   configs might not. Document memory requirements per repo size
   tier.

5. **Security: credential leakage.** PAT in process argv, PAT
   logged to a file by debug tracing, PAT visible via
   `FindTask()`. Audit every malloc of credential data; zero before
   free; never log. Phase 11 includes a security audit step.

6. **GitHub rate limiting.** Unauthenticated clones hit a rate limit
   after ~60 requests per hour per IP. Test environments need to
   use authenticated requests or mirror a repo locally to avoid
   hitting the limit during dev.

7. **Broken pipe / connection reset mid-fetch.** The Amiga TCP
   stack on Roadshow is less forgiving than Linux; mid-fetch
   interruptions need graceful recovery, not a crash.

## Success criteria

The project is DONE when all of these are true:

1. `amigit clone https://github.com/bdgscotland/amiport` on Duncan's
   A2000 + Vampire V2 500+ + X-Surf 100 + Roadshow + AmiSSL 5 stack
   produces a working clone in under 5 minutes for a ~10 MB repo.
2. `amigit fetch origin` incrementally updates a clone.
3. `amigit pull origin main` fast-forwards HEAD when possible.
4. `amigit push origin main` commits changes back to a private
   GitHub repo (auth via `ENV:GIT_HTTP_TOKEN`).
5. GitHub web UI reflects the pushed commit with the correct
   author/committer identity.
6. Binary under 1.5 MB. Stack 256 KB. Memory use during a
   10-MB-pack clone under 4 MB.
7. 87+ FS-UAE tests still green plus whatever new tests phases
   3-11 added.
8. memory-checker CLEAN across all network TUs.
9. Public release cut as amigit 1.0, LHAs uploaded to Aminet via
   aminet-publisher.

## Session checkpoint (update in-place every session)

**Current phase:** Phase 3 (Generic HTTP/1.1 client -- connect + request +
response headers, 2 sessions).
Last update: 2026-04-14.

**Last completed phase:** Phase 2 -- transport TU skeleton + dummy
registration.

**Phase 2 summary (2026-04-14):**

Three new files in `ports/amigit/ported/`:

- `transport_https.h` -- public interface:
  `amigit_transport_https_register()` + `amigit_transport_https_cleanup()`.
- `transport_https.c` (~210 lines) -- `git_smart_subtransport_definition`
  with `rpc=1`, a subtransport struct whose `action()` returns `-1` after
  calling `git_error_set_str(GIT_ERROR_NET, "HTTPS transport not
  implemented yet (service=<verb>, url=<url>, scheduled for amigit 0.2
  per PDR-012)")`. `https_subtransport_cb` `calloc`s the subtransport
  object, `https_free` matches with `free`. `https_close` is a no-op
  for Phase 2 (nothing to tear down yet).
- `cmd_ls_remote.c` -- minimal Phase 2 debug command. Runs
  `git_remote_create_detached(&remote, url)` +
  `git_remote_connect(remote, GIT_DIRECTION_FETCH, NULL, NULL, NULL)`.
  Phase 5 will replace the body with real ref iteration.

Wiring:
- `ported/amigit.c` now calls `amigit_transport_https_register()`
  right after `atexit(shutdown_libgit2)` in `main()`. Failure is
  non-fatal (local-only commands still work).
- `ported/amigit.c` dispatch table gained an `ls-remote` entry
  pointing at `amigit_cmd_ls_remote`.
- `ported/amigit.h` forward-declares `amigit_cmd_ls_remote`.
- `ports/amigit/Makefile` OBJECTS adds `ported/cmd_ls_remote.o` and
  `ported/transport_https.o`.

Four new TEST blocks appended to
`ports/amigit/test-fsemu-cases.txt` under an "ls-remote command --
PDR-012 Phase 2 (HTTPS transport stub)" section:

1. `ls-remote` with no url -> RC=10, stdout contains "ls-remote:
   missing url argument".
2. `ls-remote --help` -> RC=0, stdout contains "usage: amigit
   ls-remote".
3. `ls-remote https://github.com/bdgscotland/amiport` -> RC=10,
   stdout contains "HTTPS transport not implemented yet".
4. Same URL -> RC=10, stdout contains "upload-pack-ls" (proves
   the service verb round-trip through our stub).

**Build and test results:**

- `make -C lib/libgit2 dual`: unchanged from Phase 1 (no libgit2
  edits in Phase 2).
- `make -C tests/libgit2 run`: **79/79** still green.
- `make test-fsemu TARGET=ports/amigit`: **91/91** (87 prior + 4
  new Phase 2 tests) -- no regressions.
- `ports/amigit/amigit` binary: **1,194,888 bytes** (+1,820 from
  Phase 1 baseline 1,193,068). Within noise.

**Surprises / scope deltas:**

- **libgit2 `git_transport_register` public-doc vs
  implementation disagreement.** `git2/sys/transport.h` documents
  the `prefix` parameter as "The scheme (ending in "://") to
  match, i.e. git://". The implementation at
  `lib/libgit2/src/libgit2/transport.c:157` does
  `git_str_printf(&prefix, "%s://", scheme)` -- it takes the bare
  scheme and appends `://` itself. Passing `"https://"` produces
  the internal prefix `"https:////"` which never matches any URL,
  so the dispatcher falls through to the static transports[]
  table and picks up `git_smart_subtransport_http` from
  `transport_stubs.c` instead. The stub then fails with "not
  available in amiport build" -- exactly the wrong error message
  for Phase 2.

  Code wins over docs: `amigit_transport_https_register()` now
  passes `"https"` (bare scheme). `transport_https.c` documents
  the gotcha in a block comment above the register function so
  the next person hitting the same rake pattern knows which
  source is authoritative.

  First-pass run was 88/91 with the stub message coming from
  `git_smart_subtransport_http` (upstream stub) instead of
  `https_action` (our stub); fixing the bare-scheme call flipped
  it to 91/91 on the second pass.

- **Test harness only captures stdout.** `amigit_error_exit()`
  writes to stderr, which is the correct default for a CLI but
  invisible to the ARexx/FS-UAE test harness's output capture.
  Phase 2 works around this with a local `ls_remote_fail()`
  helper that echoes the `git_error_last()` message to stdout
  BEFORE calling `amigit_error_exit()`. This is a throwaway --
  Phase 5 will replace `cmd_ls_remote.c` entirely with real ref
  iteration and the stdout echo goes with it.

- **Clang LSP diagnostics in Kak/nvim are cross-compile noise.**
  The local clang in the editor doesn't know about bebbo-gcc's
  cross-include paths (`-I../../lib/libgit2/include` etc.), so
  every amigit C file lights up with "git2.h not found" and
  "unknown type git_remote" spam. These are NOT real errors.
  The cross-toolchain build is clean. Future sessions: ignore
  clang LSP noise on anything under `ports/amigit/ported/`.

**Phase 3 hints (what the next session should know):**

Phase 3 is the first phase with real network code. Scope per the
plan:

- New files: `ports/amigit/ported/http_client.c` + `.h`.
- `http_connect(conn, host, port, use_tls)` -- DNS + socket +
  connect via `lib/bsdsocket-shim/libamiport-net.a`. TLS branch
  returns ENOSYS until Phase 7.
- `http_send_request`, `http_read_response_status`,
  `http_read_response_header`, `http_read_body` (Content-Length
  only -- chunked is Phase 4).
- `http_close`.
- Unit tests at `tests/amigit-http-client/` for the parser half
  (vamos-compatible, driven from string literals).

Things to know before starting:

1. `libamiport-net.a` is already in the tree and known-good
   (wget ships on it). Link `-L../../lib/bsdsocket-shim
   -lamiport-net` in the amigit Makefile for Phase 3.
2. vamos cannot open `bsdsocket.library`. Phase 3 unit tests must
   be pure-C parsers driven from string literals, not live
   network tests. Live tests happen on FS-UAE with bsdsocket
   passthrough or on real hardware.
3. The `transport_https.c` skeleton already has the subtransport
   plumbing. Phase 3 adds a `http_client.c` consumer; Phase 5
   wires it into `https_action()`.
4. KB gaps noted during Phase 2 (same as Phase 1): bsdsocket HTTP
   client pitfalls and AmiSSL runtime-load patterns are NOT in
   amiga-kb yet. The reference implementations are `ports/wget/`
   (real hardware, real AmiSSL) and `lib/http-shim/`
   (bsdsocket lifecycle, socket timeout handling).
5. Memory-checker agent mandatory for any TU that malloc's HTTP
   buffers -- no process memory cleanup on exit under
   `-noixemul`.

**Phase 1 summary (2026-04-14):**

Ten upstream libgit2 1.8.5 source files and three transport sources
copied into `lib/libgit2/src/libgit2/`:

- Top-level: `clone.c`, `clone.h` (replaced stub), `fetch.c`, `fetch.h`,
  `fetchhead.c`, `fetchhead.h`, `proxy.c`, `proxy.h`, `push.c`,
  `push.h`, `remote.c`, `remote.h` (replaced stub), `transport.c`
- Smart transport: `transports/smart.c`, `transports/smart.h`
  (replaced stub), `transports/smart_pkt.c`, `transports/smart_protocol.c`

`lib/libgit2/src/libgit2/transport_stubs.c` rewritten: removed
`git_smart__ofs_delta_enabled` (now owned by upstream
`smart_protocol.c`), kept `git_http__expect_continue` but bumped
`int` to `bool` to match upstream `http.h`, added stub
implementations for `git_smart_subtransport_http` / `_git` / `_ssh`
and `git_transport_local` so upstream `transport.c`'s static dispatch
table resolves at link time. All stubs return `GIT_ERROR` with a
clear "not available in amiport build" message. Include order:
`common.h` + internal `errors.h` before the public headers (the
warning fix for `git_error_set` implicit declaration).

`lib/libgit2/src/libgit2/transports/http.h` stub kept minimal but
the type bumped from `int` to `bool` to match the upstream `http.c`
declaration that settings.c indirectly pulls in.

`lib/libgit2/Makefile` updated:
- Added `TRANSPORT_SRCS` variable listing the three smart transport
  files explicitly (not wildcard-based -- avoids accidental
  un-pruning of upstream additions in future rebases).
- Added `TRANSPORT_OBJS` / `TRANSPORT_OBJS_020` to `ALL_OBJS` /
  `ALL_OBJS_020`.
- Added pattern rules for `src/libgit2/transports/%.o` (000 flavor)
  and `src/libgit2/transports/%.020.o` (020 flavor).

`ports/amigit/ported/amigit_libgit2_stubs.c` trimmed: removed the
`git_remote_*` and `git_clone__submodule` stubs (now provided by
real upstream code; keeping them caused `multiple definition`
linker errors). Kept `strnlen`, `difftime`, `select`,
`git_socket_stream__*`, `git_failalloc_*`, `__divsf3`,
`__floatunsisf`. Updated the file header to document the new
division of responsibility.

**Build and test results:**

- `make -C lib/libgit2 dual`: both `libgit2.a` (1,442,292 bytes) and
  `libgit2-020.a` (1,443,348 bytes) rebuild cleanly, no new warnings
  beyond the pre-existing `strnlen` / `missing braces` noise from
  libgit2's own headers.
- `make -C tests/libgit2 run`: **79/79** tests pass on the 000 archive
  (Stage 5 test suite, unchanged vs baseline).
- `make test-fsemu TARGET=ports/amigit`: **87/87** functional tests
  pass on FS-UAE with the 020 archive.
- `ports/amigit/amigit` binary: **1,193,068 bytes** (+114,176 from
  baseline 1,078,892). In the middle of the PDR's expected
  1.15-1.25 MB band -- no surprises in the cascade.

**Surprises / scope deltas:**

- No additional linker cascade beyond the predicted
  `git_remote_*` / `git_clone__submodule` duplicates. `transport.c`'s
  static dispatch table referenced `git_smart_subtransport_http`,
  `_git`, `_ssh`, and `git_transport_local`, but those were
  anticipated and stubbed up front in `transport_stubs.c`, so the
  build link was clean on the first attempt.
- The `TRANSPORT_SRCS` listing is explicit (not wildcard) to keep
  the "stay-pruned" set of transport sources locked in. Future
  rebases to a new libgit2 version must re-audit and update this
  list by hand.
- `git_smart__ofs_delta_enabled` moved definition owner from
  `transport_stubs.c` to upstream `smart_protocol.c`. The global is
  still a `bool` and still defaults to `true`, so there is no
  functional change to the `GIT_OPT_ENABLE_OFS_DELTA` handler.

**Phase 2 hints (what the next session should know):**

The smart transport is now available for registration via
`git_transport_register("https://", git_transport_smart, my_def)`.
`transport_find_by_url()` checks the registered list (linked list)
before the static table, so amigit's custom backend will preempt
the `git_smart_subtransport_http` stub even though both are
resolvable. The Phase 2 stub in
`ports/amigit/ported/transport_https.c` just needs to build a
`git_smart_subtransport_definition` pointing at a subtransport
whose `action()` returns `GIT_ERROR_NOT_IMPLEMENTED` with a clean
error message -- no network code yet.

Knowledge-base gaps reported during this session (see
`amiga_report_gap` section below): bsdsocket HTTP client pitfalls,
libgit2 pruning/un-pruning cascade patterns, AmiSSL runtime-load
integration pattern (wget port is the reference in-tree, not
captured in the KB yet).

**What to do next:** Open this file in a fresh session after
`/clear`. Read Phase 3's "Deliverables" and "Done when" sections.
Start building `ports/amigit/ported/http_client.c` as the
HTTP/1.1 request/response layer over `libamiport-net.a`. Keep the
TLS path stubbed for Phase 7.

When you finish a phase, update this checkpoint section with:
- The phase just completed
- A one-line summary of what landed
- The commit SHAs (space-separated)
- Any surprises or scope deltas discovered in the phase
- What the next phase should know that wasn't obvious from the
  original plan
- Bump "Current phase" to the next one

The next session opens this file, jumps to the checkpoint, and knows
where to start without re-reading the whole document.

## Fresh-session continuation prompt

Copy this into a new session after `/clear`:

```
Resume amigit HTTPS networking work. Start by reading
docs/pdr/012-amigit-https-networking.md -- it has the full
multi-phase plan, the technical decisions that are already
locked, and a "Session checkpoint" section at the bottom that
tells you which phase is next.

Current state before this session: amigit 0.1-6 is shipped
with local-only git (all 11 v1 commands, 87/87 FS-UAE tests,
-m68020 libraries via Track A). The three network-layer
libraries (bsdsocket-shim, amissl-sdk, http-shim) are already
in the tree and working. The dual-flavor library build
infrastructure is in place and libgit2 000/020 archives are
both built clean.

Read PDR-012 section "Session checkpoint" to find the current
phase. Start from there. If you finish a phase, update the
checkpoint and commit before starting the next phase. If the
phase is too big for one session, commit what you have, update
the checkpoint with "Phase N partial -- completed X, remaining
Y", and hand off.

Also verify the local vamos monkey-patches are still applied
(`make -C tests/libgit2 run` should print 79/79) and the amigit
functional suite still passes (`make test-fsemu TARGET=ports/
amigit` should print 87/87 or more, depending on which phase
added tests). If either regresses, investigate before making
new changes.

When Phase 12 completes, amigit 1.0 ships with clone / fetch /
pull / push to GitHub via HTTPS on real Amiga hardware. That
is the finish line.
```
