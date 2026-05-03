# lib/libnslog memory-checker audit (2026-05-02)

**Verdict:** APPROVED. No critical findings.

**Library:** netsurf-browser/libnslog v0.1.3 @ commit `bedff21`. 4 TUs:

- `src/core.c` (~202 LOC, +1 amiport vsnprintf NULL fix)
- `src/filter.c` (~405 LOC)
- `src/filter-lexer.c` (3-line wrapper around generated `filter-lexer.inc`)
- `src/filter-parser.c` (~1685 LOC bison output)

## Allocation inventory

| Component | Allocations | Pattern | Status |
|-----------|-------------|---------|--------|
| core.c | 2 alloc sites | `strdup`/`malloc` for category names (lazy normalisation), `calloc` for cork chain entries | Balanced |
| filter.c | 6+ alloc sites | `calloc` per filter node (each filter kind), `strdup` for string params | Balanced |
| filter-parser.c | malloc/free via yyalloc/yyfree | Standard bison output | Parser cleanup automatic |
| filter-lexer.c | None | 3-line wrapper, no allocation | Clean |

Zero `realloc` in the entire library (no realloc-loses-pointer trap).

## Reference-counting discipline (filter.c)

`nslog_filter_unref` at line 218 uses the canonical pattern:

```c
if (filter != NULL && filter->refcount-- == 1) {
    /* recursive unref of children, then free(filter) */
}
```

`refcount-- == 1` correctly tests pre-decrement value -- equivalent to
"this caller was the last reference; free the node now." Unref(NULL) is
safe (returns NULL, never dereferences).

Children of binary ops (AND/OR/XOR) and unary ops (NOT) are unreffed
recursively BEFORE the parent is freed. Verified at lines 229-233.

## Cork chain lifecycle

- `nslog__log_corked()` calloc's `sizeof(struct) + measured_len + 1`
  bytes per buffered message. OOM path returns silently without state
  change (no leak).
- `nslog_uncork()` walks the chain, frees every entry, sets
  `nslog__corked = false` (one-shot transition).
- `nslog_cleanup()` calls `nslog_uncork()` to drain any residual chain
  before tearing down filters/categories.

## Cleanup discipline (`nslog_cleanup`)

- Drains cork chain via `nslog_uncork()` (idempotent).
- Clears active filter via `nslog_filter_set_active(NULL, NULL)`
  (decrements refcount).
- Walks `nslog__all_categories` list, frees each `cat->name`.
- Does NOT free the `nslog_category_t` structs themselves -- those are
  caller-owned via the `NSLOG_DEFINE_CATEGORY` macro.

## vsnprintf NULL probe -- amiport fix at core.c:137

Original upstream:
```c
int slen = vsnprintf(NULL, 0, pattern, ap);
```

This crashes on libnix per crash-patterns #5 (libnix vsnprintf does NOT
support NULL destination, writes to address zero on 68000). Replaced
with a 1024-byte stack probe in our copy:
```c
char probe[1024];
int slen = vsnprintf(probe, sizeof(probe), pattern, ap);
```

`nslog__log` is non-recursive; the 1024-byte stack allocation is safe
under all tested cookie sizes (256 KB and above). Verified by stress
test `probe_buffer_no_stack_overflow_2k` which formats a 2 KB message
through this path.

## Practical risk for downstream consumers

Worst-case leak if `nslog_cleanup()` is omitted: ~500 bytes to 2 KB
per process invocation (active filter tree + category names + corked
chain residue). On AmigaOS `-noixemul`, this is permanent until reboot.

Mandatory: **call `nslog_cleanup()` before program exit.** Documented
in `lib/libnslog/README.md`.

## Test ASSERT-failure leak caveat

The `ASSERT_*` macros return early from a failing test without running
cleanup. Acceptable for unit-test purposes (vamos host process exit
reclaims memory) but not representative of a real-world consumer leak
profile.

## Findings summary

| Check | Result |
|---|---|
| malloc/free balance | All 8 allocation sites have matching free paths |
| Realloc safety | No realloc anywhere -- no realloc-loses-pointer risk |
| Double-free | Refcount guard at line 218 prevents |
| Use-after-free | Unref returns NULL; callers can't reuse |
| Static globals reset | Documented one-shot for cork; categories retained |
| Soft-float pulls | Zero (verified via `m68k-amigaos-nm`) |
| Struct-by-value returns >8 bytes | Zero (all aggregates returned via pointer) |
| Stack safety | 1024-byte probe in non-recursive context, safe at 256 KB cookies |
| AmigaOS-specific traps | None detected (no Lock/Open/etc.; pure libc) |

## Recommendation

`libnslog.a` is safe to link against from `ports/netsurf` and any
downstream NetSurf-Vampire Phase D-prime consumer. No further fixes
required. Document the `nslog_cleanup()` requirement in consumer
PORT.md / README files.
