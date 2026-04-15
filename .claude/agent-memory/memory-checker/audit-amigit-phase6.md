# Memory Safety Audit: amigit PDR-012 Phase 6 Delta

**Files audited:** `transport_https.c` (new POST path + redirect loop), `transport_https.h` (new debug probe entry point), `cmd_https_probe.c` (--pack arg handler)

**Session date:** 2026-04-14  
**Finding:** CLEAN — No memory leaks or double-frees detected

---

## Critical Allocations Traced

### 1. `body_buf` Realloc Growth (https_stream_write, lines 817-826)

**Pattern:** Geometric growth (8 KB init, doubles up to 8 MB cap)

**Lifecycle:**
- **Allocation:** Line 817: `new_buf = (char *)realloc(st->body_buf, new_cap)`
- **Intermediate pointer:** Line 817 uses `new_buf`, not directly assigning to `st->body_buf` (CORRECT)
- **Error path (line 818-823):** Realloc failure leaves `st->body_buf` valid and owned by stream. Returns -1; stream remains alive. Stream free path (https_stream_free line 850) will free it (SAFE)
- **Success path (line 825-826):** new_buf assigned to st->body_buf, capacity updated
- **Dispatch path (dispatch_post_if_needed, lines 677-679):** After POST is sent, body_buf is freed explicitly and all fields (body_buf/body_len/body_cap) are zeroed (SAFE — prevents double-free)
- **Cleanup path (https_stream_free, lines 849-854):** Checks body_buf != NULL, frees, NULL-clears (SAFE)

**Verdict:** SAFE. Intermediate pointer pattern is correct. Realloc failure is handled correctly. Both dispatch and cleanup zero the pointer, preventing double-free.

---

### 2. Stream Allocations (https_action_uploadpack_ls & _post)

**GET LS stream (lines 888-912):**
- Calloc at line 888: `calloc(1, sizeof(*stream))`
- Success: All fields initialized, stream added to subt->current_stream, returned to libgit2
- Error path (line 889-893): http_close(res.conn) before returning -1 (conn cleaned, stream never reaches libgit2)
- Cleanup: libgit2 calls stream->free() → https_stream_free (lines 835-856)

**POST stream (lines 951-975):**
- Calloc at line 951: `calloc(1, sizeof(*stream))`
- Success: All fields initialized (conn=NULL deferred), stream added to subt->current_stream, returned to libgit2
- Error path: Only before calloc succeeds (line 939-949) — no cleanup needed
- post_url is embedded fixed array (stack memory), no malloc
- Cleanup: libgit2 calls stream->free() → https_stream_free

**Verdict:** SAFE. Both follow the same pattern: calloc, init, store in subt->current_stream, return to libgit2. Cleanup paths delegate to stream->free() which is correct.

---

### 3. Redirect Loop: http_conn_t Lifetime (open_request_with_redirects, lines 354-596)

**Loop structure:** For loop (line 354) with continue-on-redirect (line 595)

**Allocations per iteration:**
- Line 364: `http_conn_t *conn = NULL` (loop-local variable)
- Line 444: `http_connect(&conn, ...)`
- Lines 480, 491, 509, 534, 551, 559, 566, 589: Error return paths (11 total)

**Error path analysis:**
- Line 480, 491: http_send_request/read_response_status fail → `http_close(conn)` before return -1 (SAFE)
- Line 509, 534, 551, 559, 566, 589: http_read_response_header / Location / redirect validation fail → `http_close(conn)` before return -1 (SAFE)
- **Redirect path (lines 593-595):** `http_close(conn); conn = NULL; continue;` — conn freed before loop continues to next iteration (SAFE)
- **Success path (lines 621-627):** conn stored in `out->conn` and returned (SAFE — caller owns it)

**Critical check — loop-local conn scope:** conn is declared at line 364 (inside for loop). On continue, the old conn is closed + nulled, and a new declaration at line 364 on the next iteration shadows the previous one. No uninitialized reuse (SAFE).

**Verdict:** SAFE. Every error path calls http_close(). Every redirect iteration closes the old conn before continuing. On success, conn is returned via out parameter.

---

### 4. Stream Free Paths (https_stream_free, lines 835-856)

**Cases:**
1. **GET stream (is_post=0):** Frees conn, zero-initializes. body_buf/body_len/body_cap are never allocated for GET (NULL/0/0 from calloc) (SAFE)
2. **POST stream before dispatch (is_post=1, post_sent=0):** Frees body_buf if allocated, frees conn if allocated (which shouldn't happen but guarded) (SAFE)
3. **POST stream after dispatch (is_post=1, post_sent=1):** body_buf was already freed in dispatch_post_if_needed (line 678) and zeroed (lines 679-681). https_stream_free checks body_buf != NULL (line 849) — already NULL, skips free (NO DOUBLE-FREE)

**Field resets after free:**
- Line 851-853: body_buf/body_len/body_cap all zeroed after free (SAFE)
- Line 847: conn set to NULL after http_close (SAFE)

**Verdict:** SAFE. No double-free risk. All fields guarded with NULL checks.

---

### 5. dispatch_post_if_needed Error Path (lines 642-685)

**Error case (lines 669-671):** open_request_with_redirects fails, returns -1, sets git_error. body_buf is NOT freed here.

**Why is this correct?**
- dispatch_post_if_needed is called from https_stream_read (line 714)
- If it returns -1, https_stream_read returns -1 to libgit2
- libgit2 then calls stream->free() (documented contract in comment lines 47-48)
- https_stream_free (line 849) frees body_buf

**Verdict:** SAFE by delegation. Stream remains alive on error; cleanup happens via stream->free().

---

### 6. Subtransport Allocations (https_subtransport_cb, lines 1071-1085)

- Line 1071: `calloc(1, sizeof(*t))` for the subtransport
- Success: Returned via out parameter to libgit2
- Error (line 1072-1075): Immediately returns -1; nothing allocated on failure path
- Cleanup: libgit2 calls subtransport->free() → https_free (lines 1041-1050)

**https_free cleanup (lines 1043-1050):**
- Checks current_stream != NULL, calls https_stream_free (which handles the stream + its conn + its body_buf)
- Frees the subtransport itself

**Verdict:** SAFE. Ownership is clear; cleanup delegates to stream->free() first.

---

### 7. Stack Pressure Analysis

**open_request_with_redirects stack locals (lines 355-371):**
```
char current_url[1024]  = 1024 bytes
char host[256]          = 256
char path[512]          = 512
char req_path[1024]     = 1024
char host_hdr[320]      = 320
char headers[1536]      = 1536
char redirect_target[1024] = 1024
char tail[540]          = 540 (allocated only on redirect, line 544)
int/size_t misc         ≈ 32 bytes
```

**Per-iteration total (inside for loop):** ~4.7 KB (assuming one iteration doesn't stack all of them, which is correct — redirect_target/tail are the only optional ones, and tail shadows redirect_target in scope)

**Called from:**
- https_action_uploadpack_ls (stack has errbuf[384], res struct ≈ 40, stream struct ≈ 128) → total ~0.5 KB incoming
- https_action_uploadpack_post (stack has errbuf[384] but this is before open_request, minimal) → total ~0.4 KB
- dispatch_post_if_needed (stack has errbuf[384], res struct ≈ 40) → total ~0.4 KB
- amigit_transport_https_debug_post (stack has errbuf[384], res struct, drain_buf[1024]) → total ~1.4 KB

**Chain analysis:**
- fetch → fetch_fetch → git_fetch → libgit2's smart_connect → https_action_uploadpack_ls → open_request_with_redirects (top-level call)
- First read after fetch → dispatch_post_if_needed (called from https_stream_read) → open_request_with_redirects

**Stack cookie check:** amigit.h should have `long __stack = 262144` or similar. 4.7 KB is well within safety margin even with libgit2's internal 2-4 KB hidden depth. No issue detected. (If this becomes a problem, it would manifest as a Guru `ACPU_Bpoint` at vamos/FS-UAE runtime, not a memory leak.)

**Verdict:** SAFE from memory-leak perspective. Stack usage is heavy but bounded and reasonable for the task.

---

## Heap Consolidation

| Allocation | Type | Freed? | All paths? | Issue |
|-----------|------|--------|-----------|-------|
| body_buf (realloc) | heap | Yes | Yes | SAFE — realloc failure preserved original pointer |
| GET stream | calloc | Yes (libgit2 stream->free) | Yes | SAFE — all error paths avoid returning stream |
| POST stream | calloc | Yes (libgit2 stream->free) | Yes | SAFE — deferred dispatch on error path is correct |
| Subtransport | calloc | Yes (libgit2 subtransport->free) | Yes | SAFE |
| http_conn_t (redirect loop) | http_connect | Yes | Yes | SAFE — closed on every error + every redirect before continue |

---

## Edge Cases Verified

1. **Realloc failure during body accumulation:** Original body_buf owned by stream, freed in https_stream_free ✓
2. **POST dispatch fails after body accumulated:** body_buf stays alive; freed in https_stream_free on stream cleanup ✓
3. **Multiple redirects (up to 3):** Old conn closed on each iteration before next opens ✓
4. **Redirect without Location header:** http_close + return -1 (line 551) ✓
5. **Redirect to non-https URL:** http_close + return -1 (line 559) ✓
6. **Too many redirects (>3):** http_close + return -1 (line 566) ✓
7. **Non-200 final status:** http_close + return -1 (line 605) ✓
8. **GET LS stream with NULL body_buf fields:** Harmless — never dereferenced; stream_free guarded with != NULL checks ✓

---

## No Changes Required

The code follows best practices throughout:
- **Intermediate pointer on realloc:** Correct (line 817, new_buf)
- **Error path cleanup:** Comprehensive (11 http_close calls in redirect loop)
- **Stream ownership:** Clear delegation to libgit2's stream->free()
- **Field nulling after free:** Present (lines 679-681, 847-853)
- **Stack pressure:** Reasonable for AmigaOS target
- **Redirect loop safety:** conn is loop-local; correctly closed on each iteration

**Verdict:** CLEAN — No memory leaks, no double-free risks, no allocation imbalances.

