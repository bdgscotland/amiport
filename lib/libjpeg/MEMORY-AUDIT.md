# lib/libjpeg memory-checker audit (2026-05-02)

**Verdict:** APPROVED. No critical findings.

**Library:** IJG libjpeg 9f, IJG license. 44 hand-written .c files
(37 LIBSOURCES + jmemnobs.c minus the FLOAT DCT variants). Built
whole-archive `-O0` default with per-file `-O1` on 5 hot-path files
(jdhuff, jidctint, jidctfst, jdcolor, jdsample) per Stage 7.

## Allocation inventory

libjpeg uses a per-cinfo memory pool manager (`jmemmgr.c`). All
allocations route through `cinfo->mem->alloc_small` / `alloc_large`
/ `alloc_sarray` / `alloc_barray`. The pool is freed by
`jpeg_destroy_decompress` / `jpeg_destroy_compress` via
`cinfo->mem->self_destruct`.

| Site | Type | Pattern | Status |
|---|---|---|---|
| jmemmgr.c:276-325 alloc_small | malloc via jpeg_get_small | Per-pool, amortized | Freed by free_pool walking pool list |
| jmemmgr.c:357-368 alloc_large | malloc via jpeg_get_large | Per-allocation, dedicated malloc | Freed by free_pool |
| jmemmgr.c:398-432 alloc_sarray | rows + pointer arrays | 1 small + N large per request | Both layers freed by destroy |
| jmemnobs.c jpeg_get_small/free_small | malloc/free wrappers | Transparent passthrough | OK |
| jmemnobs.c jpeg_get_large/free_large | malloc/free wrappers | Transparent passthrough | OK |

`self_destruct` walks all pools (from JPOOL_NUMPOOLS-1 down to 0)
and frees every allocation -- including the mem_mgr struct itself.
Idempotent via `cinfo->mem != NULL` guard.

## Setjmp / longjmp safety

Consumer pattern:
```c
struct jpeg_decompress_struct cinfo;
struct test_err_mgr err;
cinfo.err = jpeg_std_error(&err.pub);
err.pub.error_exit = my_error_exit;  /* longjmps */
if (setjmp(err.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);  /* SAFE after longjmp */
    return;
}
jpeg_create_decompress(&cinfo);
/* ... API calls that may longjmp ... */
jpeg_destroy_decompress(&cinfo);  /* SAFE on normal path */
```

Verified safe because:
1. `cinfo` struct is left intact by longjmp
2. `jpeg_destroy_decompress` is idempotent via the `cinfo->mem != NULL` guard
3. Pool walking by `self_destruct` is state-independent -- frees
   everything regardless of how far the decode proceeded
4. Consumer-provided buffers (jpeg_mem_src / scanline rows) are
   NEVER freed by libjpeg

Stress test `stress_setjmp_recovery_50_times` exercises 50 consecutive
truncated-input recoveries with no leak.

## Multiple concurrent decompress structs

libjpeg supports multiple cinfo instances. No shared global state.
The `jaricom.c` arithmetic-coding constants are read-only `static
const` tables; `jutils.c` is stateless. Test
`multiple_concurrent_decompress_structs` verifies two cinfo instances
operate independently.

## DCT_FLOAT_SUPPORTED disabled

The FLOAT DCT path (`jfdctflt.c`, `jidctflt.c`) is not compiled in.
If consumer requests `JDCT_FLOAT` at runtime, `jddctmgr.c`'s switch
dispatches to a JERR_NOT_COMPILED case which calls `error_exit`
(longjmps). Test `jdct_float_request_safe` verifies graceful
handling.

## Soft-float / FPU pulls

Verified via `m68k-amigaos-nm`:
- `__divsf3` / `__divdf3` / `__floatunsisf` -- ZERO
- `pow` / `exp` / `log` / `sqrt` / `floor` -- ZERO

## Static globals

`jaricom.c` arithmetic coding tables -- read-only `static const`.
`jpeg_natural_order` in jutils.c -- read-only `static const`. Both fine.

## Findings summary

| Check | Result |
|---|---|
| malloc/free balance | All pool allocations freed by self_destruct |
| Realloc safety | No realloc calls (pool manager doesn't grow in place) |
| Double-free | Not possible (cinfo->mem NULL guard makes destroy idempotent) |
| Use-after-free | Not possible (caller can't access pool internals) |
| Static globals | Read-only constants only |
| Soft-float / libm pulls | Zero (verified via nm) |
| Struct-by-value returns >8 bytes | Zero (all 44 TUs spot-checked) |
| Stack safety | Test passes at 512 KB cookies |
| setjmp recovery | Safe -- destroy after longjmp is well-defined |
| Multiple cinfo concurrency | No shared mutable state |

## Recommendation

`libjpeg.a` is safe to link from `ports/netsurf` and any downstream
consumer. Document the consumer cleanup discipline (destroy struct
on every exit path including setjmp recovery) and the JDCT_ISLOW /
JDCT_IFAST requirement (no JDCT_FLOAT available).
