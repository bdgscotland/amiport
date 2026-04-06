---
name: port_wget_perf
description: Performance findings for ports/wget 1.20.3 — download loop fflush, dlbuf sizing, progress modulo, eta_to_human_short sprintf, html-parse pool, hash_string multiply reviewed 2026-04-05
type: project
---

# Performance Review: wget 1.20.3

Category 4 (network). Primary bottleneck is network I/O (bsdsocket recv). CPU cost is dominated
by the download write loop, progress bar rendering (called every 16KB), and for -r mode, the
HTML parser and hash table URL tracking.

## CRITICAL / Stack Safety

- __stack = 65536. Recursive download path: main -> retrieve_url -> http_loop -> fd_read_body.
  fd_read_body allocates dlbuf on the heap (good), but skip_short_body (http.c:965) allocates
  char dlbuf[513] on the stack. That's fine at 513 bytes, not a concern.
  Stack is safe at 65536.

## HIGH Findings

1. [I/O] retr.c:211-216 — fflush after every write_data call. Called on EVERY 8KB network buffer
   received. On AmigaOS, fflush() forces a dos.library Write() syscall on the output file handle.
   This doubles the effective syscall cost per buffer: one fwrite (buffered) + one fflush (forced
   DOS write). For a 1MB download at 8KB chunks = 128 fflush calls. The VMS comment at line 203
   notes this is known to hurt performance.
   Fix: wrap in #ifndef __AMIGA__ alongside existing #ifndef __VMS block.

2. [PROGRESS] progress.c:1050 — bp->tick % (progress_size * 2 - 6) in create_image inner path.
   progress_size is typically ~50 (80-column terminal minus decorations). tick is an int.
   DIVU on 68020 = 27 cycles. Called every 200ms (REFRESH_INTERVAL), so ~5x/sec.
   Not in the actual data read loop, LOW real impact but easy fix: since progress_size is constant
   per bar instance, precompute (progress_size * 2 - 6) into a field.

3. [PROGRESS] progress.c:972 — % (orig_filename_cols + MAX_FILENAME_COLS) in scrolling filename
   path. Also a DIVU. Triggered only when filename exceeds MAX_FILENAME_COLS, so low-frequency.

## MEDIUM Findings

4. [PROGRESS] progress.c:948 — memset(bp->buffer, '\0', BUF_LEN) at start of every create_image
   call. BUF_LEN = bp->width + 100, typically ~180 bytes. Called every 200ms. The memset is
   immediately followed by carefully filling every byte of the buffer via sprintf/memcpy/memset
   calls (the bar is fully regenerated). The initial memset is wasted work. Replace with a
   single final NUL terminator after the last written byte (p is already tracked).
   Est: saves ~180 byte memset * 5x/sec = marginal, but cleans up the logic.

5. [PROGRESS] progress.c:1270-1274 — eta_to_human_short has 3 branches with DIVU each (secs/60,
   secs%60, secs/3600, (secs/60)%60). Each division is ~27 cycles on 68020. The function already
   caches last result when secs==last (good). Under cache hit the divisions are skipped. LOW impact
   given the 900ms ETA_REFRESH_INTERVAL cache.

6. [HASH] hash.c:648-649 — hash_string: inner loop is `h = (h << 5) - h + *p`. The (h<<5)-h
   is equivalent to h*31. On 68020 this compiles to two shifts and a subtract (good, no MULU).
   The compiler at -O2 should generate: LSL.L #5,h / SUB.L h,tmp — this is fine. NOT an issue.

7. [HTML] html-parse.c:831 — POOL_INIT with 256-byte stack buffer. This is per tag/attribute,
   used to avoid heap allocation for normal-sized HTML content. The 256 bytes is small but safe.
   For pages with many attributes exceeding 256 bytes, GROW_ARRAY triggers xmalloc. Acceptable.

8. [HTTP] http.c:697-720 — resp_header_locate: linear scan through all headers using c_strncasecmp
   per header. A typical HTTP response has 10-20 headers. Called many times (see grep output:
   Content-Length, Connection, Transfer-Encoding, Set-Cookie, Content-Type, Location,
   Last-Modified, Content-Range, Content-Encoding, etc.) = ~15 header lookups * 20 headers
   = 300 c_strncasecmp calls per response. At ~60 cycles each = ~18000 cycles. ONE-SHOT at
   response parse time. Not a hot path for normal downloads.

9. [CONNECT] connect.c:886 — LAZY_RETRIEVE_INFO: hash_table_get called in fd_read/fd_write
   when transport_map tick doesn't match. For plain HTTP (no SSL), transport_map is NULL and
   the fast path `if (!transport_map) info = NULL` fires immediately. Not an issue for
   non-SSL downloads.

## LOW / No-op

- sock_read/sock_write (connect.c:773-793): Simple recv()/send() wrappers with EINTR retry.
  Clean, no overhead to remove.
- fd_read_hunk / fd_read_line: uses peek+read pattern. Each call does 2 syscalls per line of
  HTTP header. For 20 headers = 40 bsdsocket calls. One-shot per response, not a concern.
- dlbufsize = max(BUFSIZ, 8*1024): BUFSIZ on libnix is 512, so dlbufsize = 8192 via the max().
  Already correct. A 16KB buffer would be marginally better for fast 10Mbit links but the
  improvement would be sub-percent.
- dot_draw/dot_update: called per buffer read but only outputs a dot every 1024 bytes. The
  logputs() per-dot is tiny overhead.

## Stack Safety Confirmation
- __stack = 65536 (main.c:48)
- VAMOS_STACK = 512 KiB (Makefile, correct for network + recursion)
- Largest local: char dlbuf[SKIP_SIZE+1] = 513 bytes (http.c:965), well within budget.
- No macro-hidden large locals found.

## Summary

Primary bottleneck: network I/O (recv throughput is the ceiling, not CPU).
Most impactful CPU fix: fflush removal (#1) — eliminates an OS syscall on every 8KB write.
Progress bar: easy cleanup but marginal real benefit.
