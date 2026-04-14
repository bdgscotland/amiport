---
name: libtommath_analysis
description: LibTomMath 1.3.0 portability analysis for AmigaOS 3.x — CLEAN verdict, pure integer math with zero POSIX dependencies
type: portability-analysis
---

# Portability Analysis: LibTomMath 1.3.0

## Summary

- **Port category**: Library (pure math, no I/O dependencies with -DMP_NO_FILE)
- **Total source files**: 154 .c files
- **Lines of code**: ~8,963
- **Tier 1 (shim) issues**: 1 (entropy source for random number generation)
- **Tier 2 (emulation) issues**: 0
- **Tier 3 (redesign) issues**: 0
- **Architecture issues**: 2 (uint64_t usage, big-endian compatibility)
- **Required libraries**: None (standalone, pure C)
- **Build flags required**: `-DMP_NO_FILE -DMP_LOW_MEM -DMP_FIXED_CUTOFFS -DMP_NO_DEV_URANDOM`
- **Test strategy**: vamos (pure computation, no filesystem/OS dependencies)
- **Portability verdict**: **CLEAN**

## Executive Summary

LibTomMath is an **ideal AmigaOS library port**. It is a pure integer math library with zero OS dependencies beyond malloc/realloc/free. All source files include only `"tommath_private.h"`, which then includes the standard C headers (`stdint.h`, `stddef.h`, `limits.h`, `stdlib.h`, `string.h`, `stdarg.h`). No POSIX headers anywhere.

The library is designed for embedded systems and includes configuration defines to disable all features that would require OS support:

- `-DMP_NO_FILE` — Disables `mp_fread()` / `mp_fwrite()` (FILE* I/O functions)
- `-DMP_LOW_MEM` — Reduces default precision from 32 to 8 digits (memory-constrained mode)
- `-DMP_FIXED_CUTOFFS` — Uses compile-time algorithm cutoffs instead of runtime tuning
- `-DMP_NO_DEV_URANDOM` — Disables `/dev/urandom` entropy source

With these defines, the library compiles to pure integer arithmetic with no external dependencies.

## POSIX Headers Found

**None.** All source files include only:

1. `"tommath_private.h"` (library-internal header)
2. Standard C89/C99 headers via tommath_private.h:
   - `<stdint.h>` — uint32_t, uint64_t, int32_t, int64_t types
   - `<stddef.h>` — size_t, NULL
   - `<limits.h>` — CHAR_BIT
   - `<stdlib.h>` — malloc, realloc, free, calloc
   - `<string.h>` — memset (optional via MP_USE_MEMSET)
   - `<stdarg.h>` — variadic macros (mp_init_multi, mp_clear_multi)

The conditional includes in `bn_s_mp_rand_platform.c` (lines 31-86) are all gated by platform defines that won't trigger on AmigaOS with `-DMP_NO_DEV_URANDOM`.

## Header Compatibility Notes

### stdint.h (C99)

LibTomMath requires `<stdint.h>` for fixed-width integer types. **bebbo-gcc provides this header** even under `-ansi` (C89 mode). The types used are:

- `uint8_t` — MP_8BIT mode (deprecated, not recommended)
- `uint16_t` — MP_16BIT mode (not used)
- `uint32_t` — Default mp_digit type (28-bit or 31-bit precision)
- `uint64_t` — Used for mp_word (double-width digit for intermediate calculations)
- `int32_t`, `int64_t` — API getters/setters (mp_get_i32, mp_set_i64, etc.)

The default build uses `uint32_t` for `mp_digit` and `uint64_t` for `mp_word`. On 68020+, 64-bit operations are emulated via software — slow but functional.

**Recommendation**: Build with default settings (MP_28BIT mode: 28-bit digits in 32-bit uint32_t). The 68020 has native 32x32→64 MULS.L/MULU.L instructions which will accelerate the intermediate 64-bit products in mp_word calculations.

## Tier 1 — Shim (Minimal Work)

| Function | File:Line | Severity | Shim Wrapper | Notes |
|----------|-----------|----------|--------------|-------|
| Random entropy | bn_s_mp_rand_platform.c:88-113 | Low | Stub or LCG | Only used by mp_rand() / mp_prime_rand() for prime generation. Can provide trivial entropy via clock()/time() seed. |

### Random Number Generation Detail

`bn_s_mp_rand_platform.c` attempts to read entropy from:

1. `arc4random()` — *BSD systems (lines 10-16)
2. `CryptGenRandom()` — Windows (lines 19-48)
3. `getrandom()` — Linux glibc 2.25+ (lines 51-73)
4. `/dev/urandom` — Unix fallback (lines 79-113)
5. `ltm_rng()` callback — User-provided (lines 116-128, deprecated)

**AmigaOS solution**: Build with `-DMP_NO_DEV_URANDOM` to disable the `/dev/urandom` path. Provide a custom `ltm_rng` callback or use the fallback Jenkins PRNG (`bn_s_mp_rand_jenkins.c`) which seeds from a uint64_t value. For Dropbear SSH, seed from `clock()` + `time()` + stack address (same pattern as CPython's `pyurandom()` fallback).

The Jenkins PRNG is already in the library and provides cryptographically adequate randomness for key generation when seeded properly. No external entropy source needed at build time — just a proper seed at runtime.

## Tier 2 — Emulation

**None.** The library has no approximate behaviors.

## Tier 3 — Redesign

**None.** The library has no structural dependencies on Unix process models.

## Architecture Issues

| Issue | File:Line | Severity | Notes |
|-------|-----------|----------|-------|
| **uint64_t usage** | All files via mp_word | Low | 68k has no native 64-bit registers. bebbo-gcc emulates via software (slow). Acceptable for crypto — correctness > speed. |
| **Big-endian compatibility** | bn_mp_pack.c, bn_mp_unpack.c | None | Library is endian-agnostic. `mp_pack()` / `mp_unpack()` take an `mp_endian` parameter (MP_LITTLE_ENDIAN / MP_BIG_ENDIAN / MP_NATIVE_ENDIAN). Caller specifies byte order explicitly. No hardcoded assumptions. |

### 64-bit Integer Performance

On 68000/68020 without FPU, all uint64_t operations route through libm software emulation:

- Addition/subtraction: ~10-20 cycles (acceptable)
- Multiplication: ~100-200 cycles (acceptable for infrequent ops)
- Division: ~500-1000 cycles (acceptable for infrequent ops)

The 68020 has `MULS.L` / `MULU.L` (32x32→64 multiply) which bebbo-gcc uses for the low half of 64-bit products. This accelerates the mp_word intermediate calculations significantly vs pure 68000.

LibTomMath's algorithms are designed to minimize 64-bit ops — most work is done on 28-bit mp_digit values, with mp_word used only for intermediate carry propagation in multiplication/squaring. The 64-bit overhead is tolerable.

### No Struct-by-Value Returns

The only struct defined is `mp_int` (4 fields: int used, int alloc, mp_sign sign, mp_digit *dp). **No functions return mp_int by value** — all functions take `mp_int *` pointers. No crash-patterns #16 risk.

### No Large Local Buffers

Grepped for `char.*\[` declarations — **zero hits** for non-static arrays. All digit arrays are heap-allocated via `MP_MALLOC` / `MP_REALLOC`. Stack usage is minimal (local int/mp_digit variables only). No crash-patterns #10 risk.

### No offsetof() Alignment Calculations

Grepped for `offsetof` / `__packed__` — **zero hits**. No custom allocator alignment code. The library uses standard malloc/realloc, which on libnix returns 16-byte-aligned blocks (adequate for mp_digit = uint32_t). No crash-patterns #15 risk.

## C Language Compatibility

### C89 vs C99

The library **requires C99** for:

1. `<stdint.h>` — fixed-width integer types
2. `//` comments — present in 6 files (bn_mp_unpack.c, bn_mp_pack.c, bn_s_mp_rand_jenkins.c, bn_mp_prime_strong_lucas_selfridge.c, bn_mp_sqrtmod_prime.c, bn_s_mp_log.c)
3. Mixed declarations — some loop variables declared mid-block

**Recommendation**: Build with `-std=gnu99` (ADR-022). bebbo-gcc 6.5.0b supports gnu99 and provides `<stdint.h>`. The C99 language features are minimal and non-invasive.

**Verified**: Zero `for (int i = 0; ...)` patterns (C99 loop-init declarations). All loop counters are pre-declared. Only 8 `//` comment lines across 6 files (trivial to mechanically convert if needed, but gnu99 accepts them).

## Required Build Configuration

```make
# Makefile for libtommath on AmigaOS
CC = m68k-amigaos-gcc
CFLAGS = -O0 -noixemul -m68020 -std=gnu99 \
         -DMP_NO_FILE \
         -DMP_LOW_MEM \
         -DMP_FIXED_CUTOFFS \
         -DMP_NO_DEV_URANDOM \
         -Wall -Wextra

SRC = $(wildcard src/bn_*.c)
OBJ = $(SRC:.c=.o)

libtommath.a: $(OBJ)
	m68k-amigaos-ar rcs $@ $^

clean:
	rm -f $(OBJ) libtommath.a
```

### Build Flag Rationale

- `-O0` — Default until proven safe at -O1/-O2 (known-pitfalls: libraries default to -O0)
- `-m68020` — Target 68020+ for native 32x32→64 multiply (MULS.L instruction)
- `-std=gnu99` — Required for `<stdint.h>` and `//` comments
- `-DMP_NO_FILE` — Removes `mp_fread()` / `mp_fwrite()` (FILE* dependencies)
- `-DMP_LOW_MEM` — Reduces default precision (8 digits instead of 32, ~256 bytes per mp_int)
- `-DMP_FIXED_CUTOFFS` — Compile-time algorithm thresholds (no runtime tuning globals)
- `-DMP_NO_DEV_URANDOM` — Disables `/dev/urandom` open/read in rand platform code

## Test Coverage Recommendations

From the test-coverage-standard perspective, the library test suite should cover:

### Functional (API surface)

All 140+ public functions in `tommath.h`. Prioritize:

- `mp_init` / `mp_clear` — memory lifecycle
- `mp_add` / `mp_sub` / `mp_mul` / `mp_div` / `mp_mod` — basic arithmetic
- `mp_exptmod` — modular exponentiation (Dropbear's core operation)
- `mp_prime_is_prime` / `mp_prime_rand` — prime testing/generation
- `mp_invmod` — modular inverse (RSA key ops)
- `mp_pack` / `mp_unpack` — binary serialization (big-endian for network protocols)

### Error Paths

- `MP_MEM` — malloc failure during `mp_grow()`, `mp_init()`, etc.
- `MP_VAL` — invalid input (negative size, bad radix, etc.)
- `MP_BUF` — buffer overflow in `mp_pack()` / `mp_to_radix()`

### Edge Cases

- Zero values (`mp_zero`)
- Single-digit values
- Maximum-precision values (MP_PREC digits)
- Negative numbers (sign handling)

### Amiga-Specific

- **Endianness**: `mp_pack(MP_BIG_ENDIAN)` vs `mp_pack(MP_LITTLE_ENDIAN)` produces correct byte order
- **64-bit emulation**: Large multiplications (>32 bits) produce correct results on 68k
- **Memory exhaustion**: Graceful MP_MEM return when libnix pool exhausted (not crash)

### Stress

- 1024-bit RSA key generation (prime testing loop, ~10-30 seconds on 68020)
- 2048-bit modular exponentiation (typical Dropbear operation)
- Repeated alloc/free cycles (no leaks, no fragmentation crashes)

## Recommended Approach

This is an **EASY** port. The library is designed for portability and has zero OS dependencies with the recommended build configuration.

### Stage 1: Initial Build (Estimated: 30 minutes)

1. Copy source to `lib/libtommath/src/`, headers to `lib/libtommath/include/`
2. Write Makefile with flags above
3. Build with `make -C lib/libtommath`
4. **Expected result**: Clean compile, 154 object files → `libtommath.a`

### Stage 2: Test Suite (Estimated: 2 hours)

1. Port upstream `demo/test.c` to AmigaOS (remove FILE* I/O if present)
2. Write custom test harness using `tests/shim/test_framework.h` pattern
3. Test via vamos (pure computation, no FS-UAE needed)
4. Verify all API categories: init/clear, arithmetic, modular ops, primes, serialization

### Stage 3: Integration (Estimated: 1 hour)

1. Update `lib/libtomcrypt/Makefile` to link `-ltommath`
2. Update Dropbear build to include `-L../../lib/libtommath -ltommath`
3. Test Dropbear key generation / crypto operations

**Total estimated effort**: 4-5 hours for a complete, tested library build.

## Known Limitations

1. **No FILE* I/O**: `mp_fread()` / `mp_fwrite()` disabled via `-DMP_NO_FILE`. Dropbear doesn't use these (it serializes via `mp_pack()` / `mp_unpack()` to memory buffers).

2. **Reduced precision**: `-DMP_LOW_MEM` sets default precision to 8 digits (~224 bits). Adequate for 2048-bit RSA (needs ~73 digits, allocated dynamically via `mp_grow()`). The LOW_MEM flag only affects the **initial** allocation, not the maximum — mp_int can grow to any size.

3. **Slow 64-bit ops**: All uint64_t operations emulated in software on 68k. Acceptable for crypto (correctness-critical, not speed-critical). A 2048-bit RSA signature on 68020@14MHz will take ~5-10 seconds (vs ~0.1s on modern x86). This is the nature of software bignum math on 32-bit hardware.

4. **Entropy source**: `-DMP_NO_DEV_URANDOM` disables automatic entropy. **Action required**: Dropbear must call `mp_rand_source()` at startup to register a custom entropy callback. Use the pattern from CPython's `pyurandom()` (clock + time + stack address LCG). Or rely on Jenkins PRNG with a properly seeded `s_mp_rand_jenkins_init()`.

## Comparison to Other Crypto Libraries

LibTomMath is one of three major bignum libraries used in embedded SSH:

| Library | Size | Speed | Portability | Dropbear Default |
|---------|------|-------|-------------|------------------|
| LibTomMath | ~300KB | Medium | Excellent | Yes (default) |
| GMP | ~1MB | Fast | Poor (ASM) | Optional |
| mbedTLS bignum | ~200KB | Slow | Excellent | No |

**Verdict**: LibTomMath is the correct choice for AmigaOS. GMP requires platform-specific assembly (no 68k support). mbedTLS is smaller but slower and not Dropbear's default.

## Post-Port Integration Notes

After `lib/libtommath/` builds successfully:

1. **Update `lib/libtomcrypt/Makefile`**: Add `-I../../lib/libtommath/include` to CFLAGS, `-L../../lib/libtommath -ltommath` to LDFLAGS. LibTomCrypt uses LibTomMath as its math backend.

2. **Update `ports/dropbear/Makefile`**: Same — add include path and link flags.

3. **Seed the PRNG**: In Dropbear's `random_init()` (or equivalent), call:
   ```c
   #ifdef __AMIGA__
   mp_rand_source(amiga_entropy_source);
   #endif
   ```
   Where `amiga_entropy_source()` is a callback using the CPython pyurandom pattern.

4. **Test key generation**: Run `dropbearkey -t rsa -s 2048 -f test.key` on vamos. Should complete in ~10-30 seconds on emulated 68020.

5. **Memory audit**: Dispatch `memory-checker` agent after successful Dropbear build. Verify no leaks in key-gen / crypto paths.

## Learnings

None — this analysis proceeded without surprises. The library's design (pure math, configurable OS dependencies, embedded-system focus) made it a textbook CLEAN port.
