---
name: port_amigit_perf
description: Performance findings for ports/amigit — Phase 5+6 transport_https.c audit (PDR-012)
type: project
---

## amigit 0.1 transport_https.c — Phase 6 perf audit (2026-04-14)

Build flags: `-O1 -m68020 -std=gnu99`. Binary links libgit2-020.a + libz-020.a.

### Verdict: CLEAN (no HIGH/CRITICAL findings)

The transport_https.c hot path is entirely I/O-bound. TLS handshake + TCP
round trips + pack object download dominate all timing. CPU-side code is
negligible.

### Hot path (clone/fetch)

1. `https_stream_write` -- accumulates want/have pkt-line body via realloc
   growth. memcpy append per call. Runs 2-20x for fresh clone.
2. `https_stream_read` -- drains pack response body via http_read_body.
   Runs N times until EOF.
3. `open_request_with_redirects` -- runs once (twice with redirect).

### What was checked and found clean

- `ieq_ascii` / `to_lower_ascii` / `contains_ci`: All trivially inlined
  by -O1. Called in header loop (~6-10 headers per response), cheap.
- `contains_ci` O(n*m) inner loop: needle is "chunked" (7 chars). With
  ~6-10 headers, worst-case is < 1000 char comparisons per HTTP response.
  Not hot.
- `https_stream_write` growth loop: `new_cap *= 2` compiles to LSL.L.
  Constant-divide `MAX_CAP / 2` compiles to right-shift. Clean.
- Stack frame in `open_request_with_redirects`: 5212 bytes of locals.
  Well within __stack = 262144 (stack budget: 5212 + 8192 << 262144).
  Not a safety issue.
- `memcpy(st->body_buf + st->body_len, buffer, len)`: single bulk call,
  no byte loop. Optimal.

### Applied fix (LOW)

- `open_request_with_redirects` line ~403: replaced
  `snprintf(host_hdr, sz, "%s", host)` with `strcpy(host_hdr, host)`
  for the port==443 common case. Saves printf machinery (~hundreds of
  cycles). Runs once per request, not in a loop -- genuinely LOW but
  trivial to apply.

### Notes for future sessions

- No candidates for -O2 promotion in this TU. The bottleneck is
  http_client.c (SSL I/O) and libgit2's pack indexer, both outside
  this file.
- If perf is ever measured as a concern on real hardware, profile
  http_client.c's recv loop and the libgit2 SHA computation path
  (already promoted to -O1 with -fno-strict-aliasing in libgit2's
  HOTPATH_CFLAGS) rather than this TU.
