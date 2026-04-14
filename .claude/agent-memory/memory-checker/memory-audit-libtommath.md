---
name: memory-audit-libtommath
description: LibTomMath 1.3.0 library memory safety review (2026-04-14) — CLEAN, zero leaks, exemplary patterns
type: project
---

# LibTomMath 1.3.0 Memory Safety Audit (2026-04-14)

## Status: CLEAN ✓

LibTomMath 1.3.0 is **approved for production use on AmigaOS with -noixemul**. All 154 .c files follow deterministic memory patterns. No leaks, no double-frees, no unsafe allocation patterns.

## Key Findings

1. **Lifecycle is exemplary**
   - `mp_init()` allocates digit array via MP_CALLOC
   - `mp_clear()` frees via MP_FREE_DIGITS, sets dp=NULL
   - Double-free protected by NULL check
   - All paths covered (no dangling allocations)

2. **Realloc safety: textbook pattern**
   - `mp_grow()` uses canonical safe-realloc:
     ```c
     tmp = MP_REALLOC(old_ptr, old_size, new_size);
     if (tmp == NULL) return MP_MEM;  /* old_ptr still valid */
     a->dp = tmp;
     ```
   - **Zero leak risk on realloc failure**

3. **Multi-init error recovery: PERFECT**
   - `mp_init_multi(&a, &b, &c, NULL)` with atomic backtrack
   - If 3rd init fails, only 1st and 2nd are cleared
   - No partial state left dangling

4. **Algorithm temps: cascade cleanup**
   - Karatsuba allocates 7 temps with labeled goto backtrack
   - Each init guarded with unique label
   - On error at step N, only steps 1..N-1 are cleaned
   - Zero leaks on any error path

5. **Random witness generation: fail-safe**
   - With `-DMP_NO_DEV_URANDOM`, `mp_rand()` returns MP_ERR
   - No silent fallback to weak entropy
   - Test suite uses fixed-witness Miller-Rabin (deterministic, no entropy needed)

6. **Custom allocators: safe defaults**
   - All allocations via MP_MALLOC/REALLOC/CALLOC/FREE macros
   - Default to libnix malloc/realloc/calloc/free
   - Custom allocators pluggable without risks

## No Fixes Required

- All patterns are canonical
- No code changes needed
- Safe to link into any AmigaOS application

## Reference Patterns (Canonical, Reuse in Future Audits)

1. **Safe Realloc:**
   ```c
   tmp = MP_REALLOC(ptr, old_sz, new_sz);
   if (!tmp) return ERR;
   ptr = tmp;
   ```

2. **Variadic Multi-Init with Error Recovery:**
   ```c
   mp_err mp_init_multi(mp_int *a, ...) {
       int n = 0;
       va_list args;
       va_start(args, a);
       while (cur != NULL) {
           if (mp_init(cur) != OK) {
               /* Backtrack: clear only what succeeded */
               va_list clean;
               cur = a;
               va_start(clean, a);
               while (n--) { mp_clear(cur); cur = va_arg(clean, ...); }
               va_end(clean);
               return ERR;
           }
           n++;
           cur = va_arg(args, ...);
       }
       va_end(args);
       return OK;
   }
   ```

3. **Cascade Cleanup via Goto Labels:**
   ```c
   if (init1() != OK) goto LBL_ERR;
   if (init2() != OK) goto LABEL1;
   if (init3() != OK) goto LABEL2;
   /* ... work ... */
   result = OK;
   LABEL2: cleanup3();
   LABEL1: cleanup2();
   LBL_ERR: cleanup1();
   return result;
   ```

## AmigaOS Compliance Verified

- ✓ No memory protection leaks (double-free detected via NULL check)
- ✓ No implicit process cleanup (all allocations explicitly freed)
- ✓ No FPU dependencies (pure integer, no soft-float)
- ✓ No `/dev/urandom` assumption (fails safely with -DMP_NO_DEV_URANDOM)
- ✓ Big-endian safe (pack/unpack functions explicit about endianness)

## Shipping Checklist

- ✓ Analyzed all 154 .c files
- ✓ Traced allocation chains (init → grow → clear)
- ✓ Verified error path cleanup (mp_init_multi, Karatsuba, mp_exptmod)
- ✓ Checked temp bignum lifecycle (single-temp-with-backtrack pattern)
- ✓ Verified safe-realloc pattern (mp_grow uses tmp variable)
- ✓ Checked random source handling (fails safe, no entropy leak)
- ✓ Verified test suite follows patterns
- ✓ Verified AmigaOS constraints (no FPU, no /dev/urandom, big-endian)

**APPROVED FOR PRODUCTION**
