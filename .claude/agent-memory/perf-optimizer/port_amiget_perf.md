---
name: port_amiget_perf
description: Performance findings for ports/amiget 1.0 — sha256_update byte loop, db_reload per-call, HTTP recv buffer, cmd_search lowercase copies reviewed 2026-04-05
type: project
---

# Performance Review: amiget 1.0

Category 4 (network). Primary bottleneck is network I/O. CPU-bound path is SHA-256 verification.

## HIGH Findings

1. [SHA256] sha256_update byte-by-byte loop — copies data one byte at a time into ctx->data,
   calling sha256_transform every 64 bytes. For a 45KB file = 720 transform calls, each
   transform triggered via the inner loop overhead. Replace with memcpy + bulk dispatch.
   Est: ~25% SHA-256 speedup on 68020.

2. [DB] db_reload() called on every amiget_db_find() call — cmd_list() calls db_find() once
   per manifest entry (up to 128x), re-reading S:amiget.db from disk each time (fopen+fgets
   per call). Add a db_dirty flag to cache the in-memory state between calls within a single
   amiget invocation. Est: eliminates 127 redundant disk reads in cmd_list with 128 packages.

3. [HTTP] HTTP_RECV_BUF = 2048 — marginal for 68k networking. Zorro II bus to network card
   is ~2MB/s; with 7MHz 68000 each recv() syscall costs ~200-400 cycles of overhead.
   Recommend 4096 or 8192 for the body receive loop. The static buffer can grow safely since
   it is already static in http_get_one(). Header receive loop is fine as-is (small headers).

## MEDIUM Findings

4. [SEARCH] cmd_search() builds lowercase copies of every package name and description on
   every search — 128 packages * 3 manual tolower loops (name, desc, term) = 384 scan passes
   per search. Term lowercase copy is wasted if no packages match early. Move term_lower
   computation outside the loop. Also: the manual uppercase-check (>= 'A' && <= 'Z') is
   correct but verbose; tolower() is equivalent and compiles to the same code.

5. [JSON] parse_package() uses strcmp() chain for 12 keys on every field — no early exit.
   A first-char dispatch reduces strcmp calls from 12 to ~3 for most keys. Worth doing if
   manifest grows beyond 128 packages with many fields.

## LOW / Not Worth Doing

- sha256_transform() inner loop: all ROTRIGHT() expand to (a>>b)|(a<<(32-b)). On 68020 the
  compiler can use ROR/ROL instructions. The loop itself is ~25 instructions per round * 64
  rounds. Not further optimizable without 68k assembly.
- find_header() in http.c: linear scan of headers. Headers are small (<2KB). Not a bottleneck.
- config.c: single-shot startup. Not a bottleneck.

## Overall

I/O bound (network). SHA-256 is the meaningful CPU cost (~0.5-1s on 68020 for a 45KB LHA).
The db_reload per-call is the most egregious issue for cmd_list/upgrade with many packages.
