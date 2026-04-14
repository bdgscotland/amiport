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

### Phase 3: HTTPS client -- HTTP/1.1 + AmiSSL TLS from day one (3 sessions)

**Goal:** write a full HTTPS-capable HTTP/1.1 client in
`ports/amigit/ported/http_client.c` with AmiSSL TLS integration
wired in from the very first connect. No plain-HTTP stepping
stone -- GitHub and every other git forge worth cloning is HTTPS
only (locked decision #5). Building a plain-HTTP client first
would be a testable artifact against nothing real.

**Scope merge note:** this phase is the combined Phase 3 (HTTP
plumbing) + Phase 7 (AmiSSL integration) from the original plan.
Collapsed 2026-04-14 after realizing nothing we want to talk to
uses plain HTTP, so deferring AmiSSL just means Phase 4-6 would
run against a synthetic `http://local-git-daemon/repo` that
proves nothing. Phases 4-onward renumber down by one as a
result; old Phase 7 no longer exists as a standalone.

**Reference implementation to copy, not invent:**
`ports/wget/ported/src/` ships HTTPS via manual-OpenLibrary
AmiSSL integration on the same A2000 + Vampire + X-Surf +
Roadshow + AmiSSL stack we're targeting. The `libamisslauto.a`
vs manual-open pitfall is already documented in
`.claude/rules/known-pitfalls.md`. Phase 3 copies wget's SSL
glue verbatim (modulo naming) -- do NOT roll a new AmiSSL
wrapper from scratch.

**Deliverables:**
- `ports/amigit/ported/http_client.c` / `.h` with:
  - `http_conn_t` -- opaque connection struct (socket fd, host,
    port, tls flag, AmiSSL `SSL *` handle, SSL_CTX *, keepalive
    state, read/write buffers)
  - `http_connect(conn, host, port, use_tls)` -- DNS resolve via
    bsdsocket-shim `getaddrinfo`, `socket()`, `connect()`. If
    `use_tls == true` (mandatory for `https://`): manual
    OpenLibrary AmiSSL, `SSL_CTX_new`, `SSL_CTX_set_verify`,
    `SSL_new`, `SSL_set_fd(ssl, sockfd)`, `SSL_connect`. If
    AmiSSL open fails: return a specific error so the caller can
    produce a friendly "HTTPS not available (AmiSSL not
    installed); run `amiport install amissl`" message.
  - `http_send_request(conn, method, path, headers, body, len)`
    -- format request line + headers + optional body. If TLS,
    route through `SSL_write`; otherwise raw bsdsocket `send()`.
  - `http_read_response_status(conn, &status)` -- parse HTTP/1.1
    status line. TLS-aware read.
  - `http_read_response_header(conn, name, value)` -- iterate
    headers one at a time. TLS-aware read.
  - `http_read_body(conn, buf, max)` -- read from body,
    respecting Content-Length. Chunked encoding is Phase 4. TLS-
    aware read.
  - `http_close(conn)` -- `SSL_free`, `SSL_CTX_free`,
    `CloseLibrary(AmiSSLBase)`, close socket, free buffers.
    Handles partial-init cleanup (e.g. OpenLibrary succeeded but
    SSL_connect failed).
- `ports/amigit/ported/amissl_glue.c` (or fold into http_client.c
  if it's short) -- the actual OpenLibrary/OpenAmiSSLTags/
  CloseLibrary boilerplate, isolated so the HTTP layer is
  testable without AmiSSL-specific symbols leaking through its
  interface. Follows the wget pattern exactly.
- `ports/amigit/Makefile` -- add `-L../../lib/bsdsocket-shim
  -lamiport-net` and `-L../../lib/amissl-sdk/lib -lamisslstubs`
  (NOT `-lamisslauto` -- that triggers the process-start crash
  documented in known-pitfalls). Add `-I../../lib/amissl-sdk/include`
  to CFLAGS. Add `ported/http_client.o` (and `ported/amissl_glue.o`
  if separate) to OBJECTS.
- Unit test TU: `tests/amigit-http-client/` -- vamos-compatible
  unit tests for the parser half (status line parsing, header
  parsing, Content-Length body reader) driven from string
  literals in memory. NO live network, NO AmiSSL in the unit
  tests -- the TLS branch is gated behind `use_tls` and the
  unit tests pass `use_tls=false` + a pre-populated buffer so
  the read/write calls go through a local fd pair. This proves
  the parser logic without needing bsdsocket or AmiSSL in vamos.
- Integration test strategy:
  - **vamos:** parser unit tests only. vamos cannot open
    bsdsocket.library or amisslmaster.library.
  - **FS-UAE:** IF the FS-UAE config has bsdsocket passthrough +
    AmiSSL installed, add one `TEST:` block that does a live
    HTTPS GET against `https://amiport.platesteel.net/` (known
    stable, our own infrastructure, no rate limits, no auth).
    If passthrough/AmiSSL aren't available in the test harness,
    document it and skip the live test -- real-hardware manual
    verification covers the gap.
  - **Real hardware:** manual smoke test -- `amigit ls-remote
    https://github.com/bdgscotland/amiport` from the A2000 +
    Vampire + X-Surf + Roadshow + AmiSSL stack. The
    `transport_https.c` action() is still a Phase 2 stub at this
    point, so "success" here is "http_client talks to github,
    returns a parseable response, then the stub fires
    not-implemented cleanly". The stub message on a real
    github.com response is MORE evidence the stack works than a
    vamos parser test could ever provide.

**Done when:**
- Unit tests green on vamos (parser-only, no network, no TLS).
- Memory-checker CLEAN across `http_client.c` and
  `amissl_glue.c`. Every malloc tracked for atexit cleanup; SSL
  handles freed on every exit path (including error paths where
  OpenLibrary succeeded but SSL_connect failed).
- If FS-UAE passthrough + AmiSSL available in the test harness:
  one live HTTPS GET to `amiport.platesteel.net` returns a
  parseable response.
- Real-hardware smoke: `amigit ls-remote https://github.com/...`
  on Duncan's Amiga reaches github, gets an HTTPS response, and
  the Phase 2 stub returns "not implemented" AFTER the network
  layer succeeded (not before).
- Graceful degrade: rename `amisslmaster.library` temporarily,
  retry, confirm the friendly "HTTPS not available" error.
- `make -C tests/libgit2 run` still prints 79/79.
- `make test-fsemu TARGET=ports/amigit` still prints 91/91 (or
  92/92 if the live FS-UAE HTTPS test was added).

**Risk vs. deferred-TLS ordering:** bundling TLS in from day one
means a bad response from github.com is ambiguous between
"HTTP parser is wrong" and "AmiSSL recv is wrong". Mitigant:
(a) wget's AmiSSL glue is a proven in-tree reference, not a new
invention, so the SSL side starts as a known-good copy; (b) the
parser unit tests run without AmiSSL in the loop (TLS gated
behind `use_tls`), so any parser failure can be reproduced in a
vamos unit test without touching the network; (c) if the live
test fails, the isolation tool is `wget https://...` from the
same Amiga -- if wget works and amigit doesn't, the bug is in
amigit's glue, not AmiSSL itself.

### Phase 4: pkt-line framing + chunked transfer encoding (1 session)

*(Was Phase 4 in the original plan. Unchanged scope.)*


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

*(Was Phase 5 in the original plan. Unchanged scope, except the
target URL is `https://` from the start, not `http://`, because
Phase 3 now has working TLS.)*

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

*(Was Phase 6 in the original plan. Target URL is `https://` from
the start -- the old "clone from plain-HTTP local git daemon as a
stepping stone" milestone is deleted; success criterion is now
"clone from real github.com over HTTPS".)*

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
- Full integration: `amigit clone https://github.com/bdgscotland/amiport`
  creates a working clone on disk. No plain-HTTP intermediate --
  TLS is already working as of Phase 3.
- Memory-checker: pack buffer lifecycle. Side-band-64k can
  interleave packets; buffer management is error-prone.

**Done when:**
- Clone from real github.com over HTTPS succeeds on Duncan's
  A2000 + Vampire + X-Surf + Roadshow + AmiSSL stack.
- The cloned repo passes `amigit log`, `amigit status`, and a
  follow-up `git status` on a Linux machine that checks out the
  same commit.
- 91/91 functional suite still green.

### Phase 7: Credential callback -- GitHub PAT auth (1 session)

*(Was Phase 8 in the original plan. Renumbered after Phase 3/7
merge.)*

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

### Phase 8: cmd_clone.c + amigit 0.2 release (1 session)

*(Was Phase 9. Renumbered after Phase 3/7 merge.)*


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

### Phase 9: cmd_fetch.c + cmd_pull.c (1 session)

*(Was Phase 10. Renumbered after Phase 3/7 merge.)*


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

### Phase 10: Stabilization pass (1-2 sessions)

*(Was Phase 11. Renumbered after Phase 3/7 merge.)*


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

### Phase 11: Push support -- cmd_push.c + amigit 1.0 (1-2 sessions)

*(Was Phase 12. Renumbered after Phase 3/7 merge. This is still
the 1.0 watershed release.)*


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

*(Updated 2026-04-14 after Phase 3/7 merge. Old phases 7-12
renumbered to 7-11.)*

| Phase | vamos OK? | FS-UAE OK? | Real hw needed? |
|---|---|---|---|
| 1 | yes (libgit2 Stage 5) | yes (amigit 87/87) | no |
| 2 | yes | yes | no |
| 3 | parser tests only | if bsdsocket+AmiSSL passthrough | **yes** (HTTPS+AmiSSL) |
| 4 | yes | yes | no |
| 5 | parser tests | if bsdsocket+AmiSSL passthrough | yes (real HTTPS) |
| 6 | parser tests | if bsdsocket+AmiSSL passthrough | yes |
| 7 | no | no | yes (PAT on real GH) |
| 8 | parser tests | yes for error paths | yes (ship smoke) |
| 9 | parser tests | yes for error paths | yes (ship smoke) |
| 10 | extensive | extensive | yes (final) |
| 11 | parser tests | no | yes (ship smoke) |

**Open question for Phase 3:** does the current FS-UAE test setup
have `bsdsocket.library` passthrough AND AmiSSL installed in the
emulated AmigaOS image? If yes, we can run live integration tests
against `https://amiport.platesteel.net/` from the emulator. If
no, Phase 3 onwards is strictly parser unit-tests + real-hardware
manual verification. This is the FIRST thing Phase 3 should
establish -- before writing any http_client code.

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

**Current phase:** Phase 3 (HTTPS client -- HTTP/1.1 + AmiSSL TLS
from day one, 3 sessions). Session 1 of 3 landed the parser,
socket backend, and AmiSSL glue in-tree. Remaining session work:
install AmiSSL into the FS-UAE system.hdf, add a live-HTTPS
TEST: block, and run a real-hardware smoke test on Duncan's
A2000 + Vampire + X-Surf + Roadshow + AmiSSL stack.
Last update: 2026-04-14 (Phase 3 session 1 landed).

**2026-04-14 plan change:** original Phase 3 (plain HTTP only)
and original Phase 7 (AmiSSL integration) MERGED into the
current Phase 3 after recognizing that nothing worth talking to
uses plain HTTP (GitHub, GitLab, Gitea, Codeberg all HTTPS-only,
and the PDR's locked decision #5 already said "HTTPS only"). Old
Phases 4-6 unchanged in scope. Old Phases 7-12 renumbered down
by one to 7-11. Old Phase 7 (standalone AmiSSL) no longer exists
as a separate phase. Old Phase 12 (1.0 release) is now Phase 11.
If a previous session's handoff mentions "Phase 8" or "Phase 12"
those refer to the OLD numbering -- see the current
`## Phase plan` section for authoritative current numbering.

**Last completed phase:** Phase 2 + Phase 3 sessions 1 and 2.
Phase 3 session 3 remaining work: debug OpenLibrary-returns-NULL
on the FS-UAE harness (amisslmaster.library fails to load even
with file verifiably accessible at multiple paths), add a live
HTTPS TEST block once the load issue is resolved, and run the
real-hardware smoke test on Duncan's A2000.

**Phase 3 session 2 summary (2026-04-14):**

Session 2 wired the infrastructure for FS-UAE live HTTPS
testing. Landed:

- New debug command `ports/amigit/ported/cmd_https_probe.c` (~170 lines).
  Usage: `amigit _https-probe <url>`. The underscore prefix hides
  it from public `--help` output; it exists to exercise
  `http_client` directly before Phase 5 wires it into
  `transport_https.c`'s `https_action()`. Parses `https://host[:port][/path]`,
  calls `http_connect(use_tls=1)` + `http_send_request` +
  `http_read_response_status`, and prints `probe: status=NNN` on
  success or a stage-specific diagnosis (DNS, CONNECT, TLS_MISSING,
  TLS_HANDSHAKE, SEND, RECV, PROTOCOL) on failure. Wired into the
  amigit dispatch table with extern in `amigit.h` and object in
  `ports/amigit/Makefile` OBJECTS.
- AmiSSL 5.27 OS3 libraries downloaded from
  `github.com/jens-maus/amissl/releases/5.27/AmiSSL-5.27-OS3.lha`
  and extracted into `build/amissl-install/ex/` (gitignored). The
  relevant files -- `amisslmaster.library` (4976 bytes) and the
  68020-40 flavor of `amissl_v362.library` (3.5 MB) -- are staged
  into `build/amiga/Libs/` and `build/amiga/Libs/AmiSSL/68020-40/`
  so that WORK: (the `build/amiga/` directory mount) exposes them
  to the emulated Amiga. The system.hdf is 6 MB and near-full,
  so we cannot write either library to the HDF's Workbench3.1:Libs;
  WORK:Libs is the storage.
- `scripts/test-fsemu.sh` User-Startup now unconditionally
  `MakeDir`s `RAM:Libs/` and `RAM:Libs/AmiSSL/68020-40/`, copies
  the two library files from `WORK:Libs/` into RAM, and runs
  `Assign LIBS: RAM:Libs ADD` so OpenLibrary can walk into RAM's
  copy. (Copying instead of ADDing WORK:Libs directly was
  defensive -- some AmigaOS versions have trouble with ADDed
  assigns that point at directory-backed volumes, so RAM is the
  safer target.) Both the HDF and directory-mode User-Startup
  branches were updated.
- `ports/amigit/test-fsemu-cases.txt` gains two new `_https-probe`
  tests: missing-URL (argv parse error) and invalid-URL (scheme
  check returns `invalid URL`). Both pass. The live-HTTPS test
  that drives `http_client` end-to-end against
  `https://amiport.platesteel.net/` is intentionally NOT
  committed in session 2 -- see the OpenLibrary blocker below.

**Build and test results (session 2):**

- `make -C tests/libgit2 run`: **79/79** (unchanged).
- `make -C tests/amigit-http-client run`: **14/14** (unchanged).
- `make test-fsemu TARGET=ports/amigit`: **93/93** (91 prior +
  2 new `_https-probe` argv tests).
- `ports/amigit/amigit` binary: **1,211,472 bytes** (+1,436 from
  session 1 for the new `cmd_https_probe.o`). No Makefile/objects
  churn beyond the one new TU.

**Session 2 blocker -- OpenLibrary("amisslmaster.library") returns NULL:**

This is the one open thing preventing a live-HTTPS test on
FS-UAE. What we know:

1. `amisslmaster.library` (4976 bytes, from AmiSSL 5.27 OS3) is
   physically staged at `build/amiga/Libs/amisslmaster.library`
   and copied to `RAM:Libs/amisslmaster.library` by the
   User-Startup. Both locations are visible from the Amiga side --
   the diagnostic tests we ran during debugging (before reverting
   them) confirmed:
   - `c:list RAM:Libs` shows `amisslmaster.library`
   - `c:list WORK:Libs` shows `amisslmaster.library`
   - `c:assign` dump shows `LIBS Workbench3.1:Libs + Workbench3.1:Classes + Ram Disk:Libs`
     -- the `ADD` walks to our staged copy correctly.
2. `amissl_glue.c` tries three OpenLibrary calls in order:
   - `OpenLibrary("amisslmaster.library", 0)` -- name-only lookup,
     version 0 (accept any), which should walk the LIBS: chain.
   - `OpenLibrary("WORK:Libs/amisslmaster.library", 0)` -- explicit
     path to the WORK: staging copy.
   - `OpenLibrary("RAM:Libs/amisslmaster.library", 0)` -- explicit
     path to the RAM copy.
   ALL THREE return NULL. The graceful-degrade path fires and
   the probe command prints
   `probe: HTTPS not available (AmiSSL not installed); run amiport install amissl`.
3. Kickstart is 3.1 (`~/Documents/FS-UAE/Kickstarts/kick3.1.rom`)
   which satisfies AmiSSL 5.27's documented `Requires: AmigaOS
   3.0+/68020+`. Machine is A1200 with 8 MB Fast RAM. Copy from
   WORK: to RAM: succeeds so the file is readable; the issue is
   strictly at the `OpenLibrary`/library-init level.

Hypotheses for session 3 to investigate (in priority order):

a. **Kickstart-32 ROM**: Kick 3.1 is 1993; amisslmaster.library
   5.27 might internally call a dos.library or utility.library
   function that only exists in Kickstart 3.1.4+ or 3.2. The
   library's init routine would fail silently, returning NULL.
   Test: try a newer Kickstart (3.1.4 from Hyperion, or Kick 3.2).
b. **Memory layout**: amigit is ~1.2 MB and uses libgit2+zlib+
   libnix. After all those libraries are loaded, there may not
   be enough contiguous memory for amisslmaster + its backend
   (3.5 MB). Test: a minimal C probe program (no libgit2) that
   just opens amisslmaster.library and reports the result.
c. **Init dependency**: amisslmaster.library itself may
   `OpenLibrary` on something at init time that we don't have
   (SSL has some cipher/RNG init hooks). Test: run AmiSSL's own
   `openssl version` binary from the extracted LHA (`AmiSSL/C/AmigaOS3/OpenSSL`);
   if that also fails to load amisslmaster, it's an environment
   issue, not an amigit issue.
d. **LIBS: multi-assign walk**: the `ADD` chain is confirmed
   visible via `c:assign`, but maybe AmigaOS's `OpenLibrary` does
   NOT walk the ADD chain -- it only searches the primary assign
   target. We also try explicit full paths (b and c above), which
   should bypass LIBS: resolution entirely. If those still return
   NULL, rule out path resolution.

None of these can be debugged without a minimal probe binary.
Session 3 should start with:
- Write a 50-line `tests/amissl-probe/probe.c` that does nothing
  but `OpenLibrary`, prints the result, and exits. Run it under
  the same FS-UAE harness. This isolates the issue from amigit.
- If the probe succeeds, the issue is amigit-specific (memory,
  link order, something being initialized before amigit's main).
- If the probe fails, the issue is environmental. Test hypothesis
  (a) by swapping the Kickstart ROM, then (c) by running the
  shipped OpenSSL binary directly.

**Pragmatic session 2 exit state:**

The live-HTTPS TEST block is NOT committed -- committing a
failing test that we don't understand would block the pre-commit
hooks and obscure session 3's diagnostic work. The infrastructure
(command, library staging, ASSIGN, URL parser, graceful-degrade)
IS committed and all 93 FS-UAE tests pass green. Session 3 can
flip the live test ON once the OpenLibrary mystery is resolved.

The real-hardware smoke test on Duncan's A2000 + Vampire + X-Surf
+ Roadshow + AmiSSL (session 3) is independently valuable: it
uses the installer-configured AmiSSL environment (not our
harness's stage-to-WORK:Libs hack), so it's the authoritative
test of whether our glue layer is correct. If the real-hardware
test succeeds, the FS-UAE issue is harness-specific and we can
decide whether to invest more in FS-UAE live testing vs. rely on
real hardware for this phase.

**Session 2 surprises / scope deltas:**

- `xdftool write` to the HDF failed silently for a 4976-byte
  library because the 6 MB system.hdf is near-full (AmigaOS 3.1
  + OS3.9 support tooling + Amiga Forever bits eat ~5.8 MB). The
  smallest library (amisslmaster) wouldn't fit; the big one
  (amissl_v362.library at 3.5 MB) has no chance. Switching to
  WORK:Libs / RAM:Libs staging was necessary.
- `c:list LIBS:amisslmaster.library QUICK` returns RC=20 even
  when the ADD chain IS pointing at the staged copy (confirmed by
  `c:assign` dump). This proves `list` resolves LIBS: to the
  PRIMARY assign only -- it does not walk the multi-assign chain.
  This was a misleading diagnostic; the real question is whether
  `OpenLibrary` walks the chain, and the evidence (even explicit
  full paths to both WORK: and RAM: staging failing) says we have
  a library-load issue, not a path-resolution issue.
- The user-startup debug echo we added to test-fsemu.sh (now
  reverted) proved the generated `User-Startup` is 686 bytes
  (our script + the stock content), xdftool write returns 0,
  and the HDF S/User-Startup file has the expected size. The
  file IS being executed -- we can see the `c:assign` dump
  showing the ADDed entry. So User-Startup and ASSIGN are fine.
  The failure is downstream at OpenLibrary itself.

**Phase 3 session 1 summary (2026-04-14):**

Three new files under `ports/amigit/ported/`:

- `http_client.h` -- public API. Opaque `http_conn_t` plus an
  `http_io_t` vtable (read/write/close fn pointers) so parser
  tests can run over memory buffers. Explicit HTTP_ERR_* codes
  including `HTTP_ERR_TLS_MISSING` so callers can graceful-degrade.
- `http_client.c` (~550 lines) -- pure-C parser half (status
  line, header iteration, Content-Length body reader) on top of
  the io vtable. Plus a socket backend (`socket_read`/`write`/`close`
  using amiport_recv/send/closesocket from bsdsocket-shim) and
  `http_connect()` which does `amiport_getaddrinfo` + `socket` +
  `connect`. The parser is vamos-testable; the socket backend is
  Amiga-only (#ifdef __AMIGA__).
- `amissl_glue.c` (~320 lines) -- manual `OpenLibrary
  ("amisslmaster.library")` + `InitAmiSSLMaster` + `OpenAmiSSL` +
  `SSL_CTX_new` + `SSL_new` + `SSL_set_fd` + `SSL_connect`. Uses
  **explicit** hostname verification via
  `X509_VERIFY_PARAM_set1_host` + `X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS`
  + SNI via `SSL_set_tlsext_host_name` -- not just `SSL_VERIFY_PEER`
  alone (chain-only verification would allow a valid cert for any
  trusted domain on any host). The AmiSSL master + backend are
  cached globally via `AmiSSLMasterBase`/`AmiSSLBase` and released
  by `amissl_glue_free_cached()` which is registered via `atexit`
  in `amigit.c`. Per-connection `SSL_CTX` + `SSL` are freed in the
  io->close callback with teardown order `SSL_shutdown` -> `SSL_free`
  -> `SSL_CTX_free` -> `closesocket`.

New test suite: `tests/amigit-http-client/`

- `test_http_client.c` -- 14 vamos-testable unit tests driven by
  an in-memory io vtable (memio_priv_t). Covers status 200/301/404/
  500/malformed/HTTP 2.0 rejected; single/multi/whitespace/malformed
  headers; body with Content-Length + zero-length; GET + POST request
  formatting. No bsdsocket, no AmiSSL -- the parser runs over string
  literals.
- `Makefile` -- matches `tests/zlib` / `tests/libgit2` pattern.
  Builds with `-O0 -m68020` + `-I` paths for posix-shim, bsdsocket
  -shim, amissl-sdk. Links the full production surface (libamiport +
  libamiport-net + libamisslstubs + libm) so http_client.c's socket
  backend and amissl_glue.c's TLS backend resolve even though the
  parser tests never call them. vamos runs with `-C 68020 -s 512 -m 4096`.

Wiring:

- `ports/amigit/Makefile` -- OBJECTS gains `ported/http_client.o`
  + `ported/amissl_glue.o`. CFLAGS gains `-I../../lib/bsdsocket-shim/include`
  + `-I../../lib/amissl-sdk/include`. LDFLAGS gains `-L../../lib/bsdsocket-shim
  -lamiport-net -L../../lib/amissl-sdk/lib -lamisslstubs`. Link order
  preserved so `-lamiport-020` from the existing chain wins.
- `ports/amigit/ported/amigit.c` -- includes `http_client.h` for the
  `amissl_glue_free_cached` extern and registers it via
  `atexit(amissl_glue_free_cached)` right after `atexit(shutdown_libgit2)`.

Deliberately NOT touched in Phase 3 session 1:
- `ports/amigit/ported/transport_https.c` -- still a Phase 2 stub
  (`https_action` returns `GIT_ERROR_NET "HTTPS transport not
  implemented yet"`). Phase 5 will wire http_client into
  `https_action()`. Phase 4 adds pkt-line + chunked transfer first.
- `lib/bsdsocket-shim/` -- parallel sessions own this (uncommitted
  `getaddrinfo.c` / `fcntl_socket.c`). The existing `libamiport-net.a`
  already exports `amiport_getaddrinfo` / `amiport_freeaddrinfo` /
  `amiport_fcntl` symbols, so we link against whatever is on disk
  without touching the sources.

**Build and test results:**

- `make -C tests/libgit2 run`: **79/79** (unchanged).
- `make test-fsemu TARGET=ports/amigit`: **91/91** (unchanged).
- `make -C tests/amigit-http-client run`: **14/14** (new suite).
- `ports/amigit/amigit` binary: **1,210,036 bytes** (+15,148 from
  Phase 2 baseline 1,194,888). Delta is the http_client + amissl_glue
  object code; AmiSSL symbols are runtime-resolved via `libamisslstubs`
  so there's no static bloat from the OpenSSL surface.
- memory-checker agent audit of `http_client.c` + `amissl_glue.c`:
  **CLEAN**. All partial-init paths honored, teardown order correct,
  graceful-degrade contract between `http_connect` and
  `amissl_glue_open_io` holds (on TLS failure the glue closes the
  sockfd and leaves io untouched; the caller frees io).

**Surprises / scope deltas:**

- **PDR said "copy wget's AmiSSL glue verbatim." wget has no
  such glue.** wget's shipped Phase 1 build is HTTP-only:
  `ports/wget/Makefile` does not link libamisslstubs or libamisslauto,
  `ports/wget/ported/src/openssl.c` is not in SOURCES, and the
  known-pitfalls "libamisslauto.a Is a Hard Runtime Dependency"
  entry records that wget's attempted Phase 2 crashed on real
  hardware and was reverted. The in-tree `ports/wget/ported/src/openssl.c`
  still exists as dead code and its inclusion of `<proto/amissl.h>`
  is the only artifact of the aborted integration. Result: Phase 3
  wrote the manual-OpenLibrary pattern from the AmiSSL 5.x SDK
  headers directly (`proto/amisslmaster.h` + the autodocs), not
  by copying wget.
- **The uncommitted lib/bsdsocket-shim/ parallel work already
  shipped `amiport_getaddrinfo`, `amiport_freeaddrinfo`,
  `amiport_fcntl` symbols into `libamiport-net.a`.** `nm` confirms
  they resolve at link time without any edits from this session.
  This is "treat libamiport-net.a as other sessions own this" in
  practice -- the archive on disk is load-bearing and we link
  against it unchanged.
- **Semgrep caught `SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL)`
  as CWE-295 "Improper Certificate Validation".** The rule
  trigger is the `NULL` callback argument, which is the OpenSSL
  default-verify path (NOT "no verification") -- but semgrep is
  right that `SSL_VERIFY_PEER` alone validates the chain without
  binding the hostname. The fix is threefold: (1) pass an explicit
  `amigit_verify_cb` that returns `preverify_ok` instead of NULL,
  (2) call `X509_VERIFY_PARAM_set1_host(vp, host, 0)` on the SSL
  object's verify params before `SSL_connect`, and (3) set
  `X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS`. This is the modern
  OpenSSL 1.0.2+ API and AmiSSL 5.x exposes it. Without (2), a
  github.com cert served on amiport.platesteel.net would pass
  verification.
- **Tests link `libamiport-net` and `libamisslstubs` even though
  the parser path never touches them.** http_client.c references
  `amiport_*` functions from its #ifdef __AMIGA__ socket backend
  and amissl_glue.c references SSL_* symbols from its TLS backend;
  both appear in the compiled objects regardless of whether the
  test runtime calls them. Linker needs all symbols resolved.
  Adding the `-L` / `-l` entries is cleaner than splitting
  http_client.c into "parser" and "backend" files -- a future
  refactor might do the split but it's not worth it for Phase 3.

**Phase 3 remaining work for next session (session 2 of 3):**

1. **Install AmiSSL into `build/system.hdf`.** The shipped FS-UAE
   test harness uses a minimal Workbench 3.1 HDF that does NOT
   have `amisslmaster.library`, `amissl_v3xx.library`, or the
   `AmiSSL/` catalog directory. FS-UAE already has bsdsocket
   passthrough (`bsdsocket_library = 1` in
   `toolchain/configs/amiport-test.fs-uae` and
   `scripts/test-fsemu.sh`), so raw TCP works -- only TLS is
   missing. Download AmiSSL 5.x from
   `github.com/jens-maus/amissl/releases`, extract the libraries
   into a staging dir, and `xdftool build/system.hdf write` them
   under `Libs/amisslmaster.library` + `Libs/AmiSSL/v3xx/amissl_v3xx.library`.
   Verify with `xdftool list | grep amissl`. Document the install
   in `scripts/test-fsemu.sh` comments.
2. **Add a live-HTTPS TEST: block to `ports/amigit/test-fsemu-cases.txt`.**
   Since `transport_https.c` is still a Phase 2 stub, the live
   test cannot yet be "full clone through libgit2". It needs a
   debug path that exercises `http_connect` + `http_send_request`
   + `http_read_response_status` directly. Easiest: add a hidden
   subcommand `amigit _https-probe <url>` (underscore prefix =
   debug, not in user docs) that opens the URL with use_tls=1,
   sends a GET, prints "status=NNN" or an error code. Then a
   TEST: block with `CMD: WORK:amigit _https-probe https://amiport.platesteel.net/`
   + `EXPECT_CONTAINS: status=200`. Session 2 can also throw in
   a graceful-degrade test (rename amisslmaster.library to
   amisslmaster.library.hidden, re-run, expect "HTTPS not
   available"), though that requires test-harness support for
   pre/post file manipulation in the HDF.
3. **Real-hardware smoke test on Duncan's Amiga** -- manual,
   not automated. Run `amigit _https-probe https://github.com/bdgscotland/amiport`
   on the A2000 + Vampire + X-Surf + Roadshow + AmiSSL stack.
   Success = http_client reaches github over HTTPS and prints a
   status code. That's more evidence the stack works than any
   vamos parser test could ever provide.

**Session 2 is expected to be smaller than session 1** -- mostly
shell scripting for the HDF install, one new small C file for the
debug subcommand, one new TEST block, and a few `strings | grep`
sanity checks. Session 3 is the real-hardware smoke + stabilization
/ known-pitfalls capture.

**Phase 2 summary (2026-04-14):**

Commit: `f26842a` -- feat(amigit): PDR-012 Phase 2 -- HTTPS
transport stub registration.

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

**Deliberate non-update (don't repeat Phase 3+):** `data/catalog.json`,
`site/data/packages/amigit.json`, `README.md`, and `PORTS.md` all
still read "87/87" for amigit. They are NOT updated to "91/91".
Rationale: Phase 2 is stealth scaffolding work, not a release --
the shipped amigit 0.1-6 LHA still represents the 87-test state,
no new LHA was cut, and the 4 new Phase 2 tests exercise
`GIT_ERROR_NOT_IMPLEMENTED` stubs rather than real functionality.
Updating the dashboard to 91 without rebuilding the shipped LHA
would desync the website from the package users actually
download. The correct time to roll catalog/site forward is at
the 0.2 release cut (after Phase 9 per the phase plan). Same
reasoning for `measured_binary_kb` (catalog still says 1054 KB;
current binary is ~1167 KB but not shipped). Phase 3+ should
follow the same pattern: update PDR checkpoint + in-repo build
artifacts, leave release metadata pinned to 0.1-6 until 0.2 cut.

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

Phase 3 is the first phase with real network code, AND the
first phase with TLS. Scope per the revised 2026-04-14 plan:

- New files: `ports/amigit/ported/http_client.c` + `.h` and
  (optionally, if readable) `ports/amigit/ported/amissl_glue.c`.
- `http_connect(conn, host, port, use_tls)` -- DNS + socket +
  connect via `lib/bsdsocket-shim/libamiport-net.a`. If
  `use_tls == true`, ALSO open AmiSSL (manual OpenLibrary per
  wget pattern), SSL_new / SSL_set_fd / SSL_connect. Graceful
  degrade if AmiSSL is missing with a friendly "run `amiport
  install amissl`" error.
- `http_send_request`, `http_read_response_status`,
  `http_read_response_header`, `http_read_body` (Content-Length
  only -- chunked is Phase 4). Each is TLS-aware: routes through
  SSL_read/SSL_write when `use_tls`, raw bsdsocket otherwise.
- `http_close` -- SSL_free, SSL_CTX_free, CloseLibrary, socket
  close, buffer free. Handles partial-init cleanup.
- Unit tests at `tests/amigit-http-client/` for the parser half
  (vamos-compatible, driven from string literals in memory, TLS
  OFF -- the parser runs over a local fd pair).

Things to know before starting:

1. **Don't reinvent AmiSSL glue.** `ports/wget/ported/src/`
   ships a working manual-OpenLibrary AmiSSL integration on the
   exact same target stack (A2000 + Vampire + X-Surf + Roadshow
   + AmiSSL). Copy its glue verbatim, then adapt the naming.
   Do NOT use `libamisslauto.a` -- it crashes at process start
   if AmiSSL is missing. See `.claude/rules/known-pitfalls.md`
   entry "libamisslauto.a Is a Hard Runtime Dependency".
2. **`libamiport-net.a`** is already in the tree and known-good
   (wget ships on it). Link `-L../../lib/bsdsocket-shim
   -lamiport-net` in the amigit Makefile. Also add
   `-L../../lib/amissl-sdk/lib -lamisslstubs` and
   `-I../../lib/amissl-sdk/include`.
3. **vamos cannot open `bsdsocket.library` OR
   `amisslmaster.library`.** Phase 3 unit tests must be
   pure-C parser tests driven from string literals, not live
   network tests. Live tests happen on FS-UAE with bsdsocket
   passthrough + AmiSSL installed in the emulated image, OR on
   real hardware.
4. **First thing to establish:** does the current FS-UAE test
   harness have bsdsocket passthrough AND AmiSSL installed? If
   no, Phase 3's FS-UAE test count stays at 91/91 and real
   hardware becomes mandatory for live verification. Check
   `scripts/test-fsemu.sh` and `toolchain/configs/amiport-test.fs-uae`
   for `bsdsocket_library` and `amissl` references, OR check
   whether `ports/wget/test-fsemu-cases.txt` has any live-HTTPS
   tests -- wget is the reference for "HTTPS tests in the amigit
   harness".
5. **The `transport_https.c` skeleton already has the
   subtransport plumbing.** Phase 3 adds a `http_client.c`
   consumer; Phase 5 wires it into `https_action()`. Do NOT
   modify `transport_https.c` in Phase 3 -- it stays as the
   Phase 2 stub until Phase 5.
6. **Real-hardware smoke test for Phase 3:** from Duncan's
   A2000, run `amigit ls-remote https://github.com/bdgscotland/amiport`.
   Success is "http_client.c reaches github over HTTPS, gets a
   parseable response, and the Phase 2 stub in transport_https.c
   fires not-implemented AFTER the TLS handshake succeeded".
   That's more evidence the stack works than any vamos unit test
   could ever provide.
7. **Memory-checker agent mandatory** for http_client.c + the
   AmiSSL glue. Partial-init cleanup is error-prone -- every
   exit path must free SSL_CTX, SSL handle, socket fd, and
   CloseLibrary AmiSSL in the right order. No process memory
   reclaim under `-noixemul`.
8. **KB gaps** (same as Phases 1-2): bsdsocket HTTP client
   pitfalls and AmiSSL runtime-load patterns are NOT in
   amiga-kb yet. wget is the in-tree reference. Worth capturing
   Phase 3 learnings to amiga-kb via `/capture-learning` as
   they emerge.

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
`/clear`. Read the revised Phase 3 "Deliverables" and "Done when"
sections (Phase 3 + old Phase 7 are merged as of 2026-04-14).
Before writing any code, check whether the FS-UAE test harness
has bsdsocket passthrough + AmiSSL installed (see Phase 3 hint
#4 above). Then copy `ports/wget/ported/src/` AmiSSL glue as the
reference implementation and start building
`ports/amigit/ported/http_client.c` with TLS support mandatory
from day one -- not stubbed.

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
