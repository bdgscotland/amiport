---
name: port_bc_perf
description: Performance findings for ports/bc 1.07.1 — divide/modulo in hot loops, malloc pressure in execute loop, fpush/fpop per-call malloc, MUL_BASE_DIGITS tuning
type: project
---

## Key findings (2026-03-23)

### CRITICAL
- `_bc_simp_mul` inner loop (number.c:661-663): `sum % BASE` and `sum / BASE` compile to DIVU on 68000 — 76-140 cycles each, two per multiply step. Replace with subtraction loop (sum -= BASE * count). Estimated 3-5x speedup for typical-size multiplies.
- `_one_mult` (number.c:889-890): same pattern — `value % BASE` and `value / BASE` on every digit. Same fix applies.
- `bc_int2num` (number.c:1577-1584): `val % BASE` and `val / BASE` in a digit-extraction loop. Same fix.
- `bc_num2long` (number.c:1544): `val*BASE` — MULS on 68000 (38-70 cycles). Called on every array index access. Replace with `val = (val << 3) + (val << 1)` (shifts + add for *10).

### HIGH
- `fpush`/`fpop` (storage.c:256-264, 231-250): bc_malloc/free per function call push/pop. Use a static pool array for the function stack — bc doesn't need deep function stacks (bc scripts rarely nest more than 10 deep).
- `push_num`/`push_copy`/`pop` (storage.c:287-310, 270-281): malloc/free per execution stack op. These are called in every arithmetic instruction in the execute loop. A pool of pre-allocated estack_rec nodes would eliminate malloc overhead in the hot path.
- MUL_BASE_DIGITS=80: Karatsuba recursive multiply engages at 80 digits. On 68000 with no cache, the recursive overhead (many small mallocs, function calls) may outweigh the algorithmic win for inputs < 200 digits. Consider tuning to 160 for 68000 target.

### MEDIUM
- `bc_is_zero` (number.c:276-295): called in almost every operation. The `num == _zero_` fast path is good. The loop scans all bytes — could exit earlier on non-zero with a memchr() call for zero bytes.
- `fflush(stdout)` after every 'W', 'P', 'O', 'w' instruction in execute.c: correct but expensive if many print ops. No easy fix — bc's interactive nature requires it.
- execute.c loop condition checks both `f_code_size` and `runtime_error` and `had_sigint` every iteration — minor overhead but unavoidable for correctness.

### LOW
- `check_stack(2)` called before every binary operation in execute.c — walks list to depth 2. In normal use stack always has 2 items. Add a fast `ex_stack != NULL && ex_stack->s_next != NULL` inline check.
- `label_num % BC_LABEL_GROUP` (execute.c:139) — BC_LABEL_GROUP=64 is a power of 2, so `% 64` should compile to `& 63`. Verify compiler generates AND not DIVU here.

## Compiler flags
- Currently: `-O2 -m68000`. Good baseline.
- No `-fomit-frame-pointer` — could save 4 cycles per function call on inner routines. Safe here since no signal handlers that walk frames.

## Bottom line
bc is primarily CPU-bound on arithmetic. The % and / by BASE=10 in tight loops is the dominant cost — these generate DIVU instructions (76-140 cycles on 68000 vs ~10 cycles for shifts). Eliminating division by 10 in `_bc_simp_mul`, `_one_mult`, and `bc_int2num` is the highest-value change.
