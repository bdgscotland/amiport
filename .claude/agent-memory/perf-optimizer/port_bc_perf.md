---
name: port_bc_perf
description: Performance findings for ports/bc 1.07.1 — divide/modulo in hot loops, malloc pressure in execute loop, fpush/fpop per-call malloc, MUL_BASE_DIGITS tuning. Second pass 2026-03-23.
type: project
---

## Key findings — Second pass (2026-03-23)

### OPTIMIZATIONS VERIFIED CORRECT (first pass)

All four first-pass changes are present and mathematically correct:

1. **`_bc_simp_mul` line 663**: `{ int q = sum / BASE; *pvptr-- = sum - ((q << 3) + (q << 1)); sum = q; }`
   - Correctly computes `sum % 10` as `sum - q*10` via shifts. 1 DIVU eliminated per inner loop iteration.
   - Overflow safe: `sum` max ≈ `9*9*80 + carry` ≈ 6489 — well within int range.

2. **`_one_mult` line 890-891**: `carry = value / BASE; *rptr-- = value - ((carry << 3) + (carry << 1));`
   - Eliminates second DIVU (remainder). First DIVU (quotient) unavoidably remains.
   - Overflow safe: `value` max = `9*9 + 8 = 89`.

3. **`bc_num2long` line 1545**: `val = (val << 3) + (val << 1) + *nptr++;`
   - Loop guard `val <= (LONG_MAX/BASE)` prevents overflow before shift. Correct.

4. **`bc_int2num` lines 1579, 1584**: `{ int q = val / BASE; *bptr++ = val - ((q << 3) + (q << 1)); val = q; }`
   - Correct modulo via subtract. Overflow safe: `val` fits in int throughout.

No correctness regressions introduced by first pass.

### REMAINING HIGH — fpush/fpop malloc (storage.c:256-264, 231-250)

`fpush` calls `bc_malloc(sizeof(fstack_rec))` and `fpop` calls `free()` on every function call and return. AmigaOS `AllocMem`/`FreeMem` (exposed through libnix malloc) is expensive — approximately 200-400 cycles per call on 68000. bc function calls already issue 3x `fpush` (pc_func, pc_addr, i_base) and 3x `fpop` on return.

Fix: static pool array for function stack. bc rarely nests more than 10-20 deep. A `static fstack_rec fn_pool[64]` with a top-of-pool index eliminates all malloc/free on the function stack path. Not recursive-reentrant (bc is single-threaded), so static is safe.

### REMAINING HIGH — push_num/push_copy/pop malloc (storage.c:287-310, 270-281)

Every arithmetic instruction (`+`, `-`, `*`, `/`, `%`, `^`, comparisons) calls `pop()` which does `free()` and `push_copy()`/`push_num()` which call `bc_malloc()`. These are the hottest paths in the execute loop — hit on every computation.

The `_bc_Free_list` already pools `bc_num` structs but NOT `estack_rec` nodes. Adding a similar free-list for `estack_rec` would eliminate most malloc/free churn in the arithmetic path:

```c
static estack_rec *_estack_Free_list = NULL;

/* In pop(): */
temp->s_next = _estack_Free_list;
_estack_Free_list = temp;

/* In push_num()/push_copy(): */
if (_estack_Free_list != NULL) {
    temp = _estack_Free_list;
    _estack_Free_list = temp->s_next;
} else {
    temp = bc_malloc(sizeof(estack_rec));
}
```

Estimated: 50-70% reduction in malloc calls during arithmetic-heavy computation.

### REMAINING MEDIUM — MUL_BASE_DIGITS=80 (number.c:610)

Karatsuba threshold at 80 digits. On 68000 (no cache, 7.14MHz), recursive Karatsuba adds function call overhead + many small mallocs + memory fragmentation. For inputs with total digits < 160, the O(n^2) simple multiply in `_bc_simp_mul` likely wins because it's a tight inner loop with no allocation.

Consider: `#define MUL_BASE_DIGITS 160` for 68000 targets. Would need empirical profiling to confirm — dispatch profiler agent if precision matters.

### REMAINING LOW — check_stack(2) (storage.c:317-333, execute.c:401..532)

`check_stack(2)` is called before every binary arithmetic operation (12 call sites in execute.c). It walks the linked list up to depth 2. In normal bc use, the stack always has 2+ elements at a binary op (the parser guarantees this for well-formed expressions).

The function call overhead (JSR + stack frame) is ~20 cycles on 68000. With 2 pointer dereferences inside, total ~40-50 cycles per binary op just for the safety check.

Fix: inline fast path as a macro before the full check:
```c
#define HAVE_TWO() (ex_stack != NULL && ex_stack->s_next != NULL)
```
Replace `if (check_stack(2))` with `if (HAVE_TWO())` at all 12 arithmetic sites, and keep the full `check_stack()` call only where depth != 2. The rt_error path for underflow is cold — acceptable to keep it with a separate call.

### LOW — % BC_LABEL_GROUP (execute.c:144, load.c:90)

`label_num % BC_LABEL_GROUP` where BC_LABEL_GROUP=64 and `label_num` is `unsigned long`. GCC will optimize this to `AND #63,Dn` (bitwise AND) because 64 is a power of 2 and the type is unsigned. No manual change needed — compiler handles this correctly. Verified at first pass.

### LOW — bc_is_zero scan (number.c:276-295)

Called in almost every arithmetic instruction (branch conditions B/Z, compare ops). The loop `while ((count > 0) && (*nptr++ == 0)) count--;` exits early on first non-zero byte — already has best-case O(1) behavior for non-zero numbers. The `_zero_` pointer fast path handles the common case. No significant gain available here without memchr, and memchr is not significantly faster than the hand-written loop on 68000 (no SIMD, no hardware string ops).

## Summary

- First pass correctly applied all 4 DIVU elimination optimizations. No overflow bugs introduced.
- Two HIGH-value items remain untouched: fpush/fpop static pool and estack_rec free-list.
- MUL_BASE_DIGITS tuning is a judgment call without profiling.
- check_stack(2) inline macro is LOW effort / MEDIUM gain — worth doing.
- % BC_LABEL_GROUP is already optimized by compiler — no action needed.

## Compiler flags
- Currently: `-O2 -m68000`. Good baseline.
- `-fomit-frame-pointer` not set — could save 4 cycles per function call on inner routines.
