---
name: port_factor_perf
description: Performance findings for ports/factor 1.30 — TABSIZE=256KB stack array in pr_bigfact, 64-bit division in inner loop, reviewed 2026-03-26
type: project
---

# factor 1.30 Performance Notes

**Why:** factor's inner loop in pr_fact() trial-divides by all primes up to 65537. pr_bigfact() runs a full Eratosthenes sieve. Both are CPU-bound.

## Hot Path
- `factor.c:215-240` — pr_fact() inner loop: trial division with `val % (long)*fact` per prime
- `factor.c:269-325` — pr_bigfact() sieve loop over table[TABSIZE]
- `factor.c:257` — `char table[TABSIZE]` where TABSIZE = 256*1024 = 262144 bytes ON THE STACK

## Findings
- **CRITICAL** [factor.c:257] — `char table[TABSIZE]` with TABSIZE=262144 (256 KB) is a 256 KB local array in pr_bigfact(). __stack is only 16384 (16 KB). This will immediately overflow the stack and crash on the first call to pr_bigfact(). The Stack Safety Rule requires __stack >= buffer_size + 8192. This array must be made `static` or heap-allocated. Since pr_bigfact() is not recursive and AmigaOS is single-threaded, `static` is correct and costs nothing.
- **HIGH** [factor.c:218,235] — `val % (long)*fact` is a 64-bit by 32-bit division, emulated in software on 68k. Each iteration of the trial-division loop costs hundreds of cycles. This is inherent to the algorithm and cannot be trivially eliminated, but the cast to `(long)` is correct — the prime table entries fit in 32 bits, so using `(u_int32_t)` rather than `(long)` and matching the divisor type to ubig (u_int32_t) may let the compiler choose a 32-bit DIVU instruction for the intermediate check. Worth trying.
- **MEDIUM** [factor.c:214] — fflush(stdout) called inside pr_fact() before the factor loop (line 214), and again after every set of factors (line 238). For interactive use this is fine; for piped use it adds unnecessary I/O syscall overhead. Not a hot path issue.
- **LOW** [factor.c:274] — `factor = (start%(2*3*5*7*11*13))/2` — the constant 2*3*5*7*11*13=30030 is computed at compile time, so no issue. But the division by 30030 in the modulo is a DIVU each iteration of the while(start<stop) loop. Minor.

## Verdict
The 256 KB stack array is a crash risk that will manifest immediately when factoring large numbers. Fix required before shipping.

**How to apply:** Change `char table[TABSIZE]` to `static char table[TABSIZE]` in pr_bigfact(). Also consider raising __stack or documenting that pr_bigfact() requires static storage.
