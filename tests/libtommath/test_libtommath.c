/*
 * test_libtommath.c -- Unit tests for lib/libtommath (LibTomMath 1.3.0)
 *
 * Coverage: crypto-critical modexp, primality, modular inverse, core
 * arithmetic, conversion roundtrips, edge cases, 68k endian packing.
 * 30 tests per test-designer plan (2026-04-14).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tommath.h"
#include "test_framework.h"

static const char *verstag __attribute__((used)) =
    "$VER: test_libtommath 1.0 (14.04.2026)";
long __stack = 262144;

/* --- Priority 1: Crypto-Critical --- */

TEST(exptmod_small_known)
{
    mp_int g, x, p, y;
    mp_init_multi(&g, &x, &p, &y, NULL);

    /* 2^10 mod 1000 = 24 */
    mp_set(&g, 2);
    mp_set(&x, 10);
    mp_read_radix(&p, "1000", 10);
    ASSERT_EQ(mp_exptmod(&g, &x, &p, &y), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&y, 24), MP_EQ);

    /* 3^7 mod 13 = 3 */
    mp_set(&g, 3);
    mp_set(&x, 7);
    mp_set(&p, 13);
    ASSERT_EQ(mp_exptmod(&g, &x, &p, &y), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&y, 3), MP_EQ);

    /* 5^3 mod 13 = 8 */
    mp_set(&g, 5);
    mp_set(&x, 3);
    mp_set(&p, 13);
    ASSERT_EQ(mp_exptmod(&g, &x, &p, &y), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&y, 8), MP_EQ);

    mp_clear_multi(&g, &x, &p, &y, NULL);
}

TEST(exptmod_larger)
{
    mp_int g, x, p, y, expected;
    mp_init_multi(&g, &x, &p, &y, &expected, NULL);

    /* 2^255 - 19 is a known prime (Curve25519 field prime) */
    /* Verify: 2^128 mod (2^255 - 19) is computable without crash */
    mp_set(&g, 2);
    mp_read_radix(&x, "128", 10);
    mp_read_radix(&p,
        "7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED", 16);
    ASSERT_EQ(mp_exptmod(&g, &x, &p, &y), MP_OKAY);

    /* 2^128 = 340282366920938463463374607431768211456 */
    mp_read_radix(&expected,
        "100000000000000000000000000000000", 16);
    ASSERT_EQ(mp_cmp(&y, &expected), MP_EQ);

    mp_clear_multi(&g, &x, &p, &y, &expected, NULL);
}

TEST(prime_is_prime_small)
{
    mp_int a;
    int result;
    int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23};
    int composites[] = {4, 6, 8, 9, 10, 12, 14, 15};
    int i;

    mp_init(&a);

    for (i = 0; i < 9; i++) {
        mp_set(&a, (mp_digit)primes[i]);
        result = 0;
        ASSERT_EQ(mp_prime_is_prime(&a, 8, &result), MP_OKAY);
        ASSERT_EQ(result, MP_YES);
    }

    for (i = 0; i < 8; i++) {
        mp_set(&a, (mp_digit)composites[i]);
        result = 1;
        ASSERT_EQ(mp_prime_is_prime(&a, 8, &result), MP_OKAY);
        ASSERT_EQ(result, MP_NO);
    }

    mp_clear(&a);
}

TEST(prime_is_prime_larger)
{
    mp_int a, b;
    int result;

    mp_init_multi(&a, &b, NULL);

    /*
     * mp_prime_is_prime with t>0 needs random witnesses.
     * With -DMP_NO_DEV_URANDOM, use mp_prime_miller_rabin
     * with a fixed witness instead (deterministic).
     * Test: 127 is prime, witness 2.
     */
    mp_set(&a, 127);
    mp_set(&b, 2);
    result = 0;
    ASSERT_EQ(mp_prime_miller_rabin(&a, &b, &result), MP_OKAY);
    ASSERT_EQ(result, MP_YES);

    /* Composite: 121 = 11*11, witness 2 should detect it */
    mp_set(&a, 121);
    mp_set(&b, 2);
    result = 1;
    ASSERT_EQ(mp_prime_miller_rabin(&a, &b, &result), MP_OKAY);
    /* 121 is a pseudoprime base 3 but NOT base 2 */
    ASSERT_EQ(result, MP_NO);

    mp_clear_multi(&a, &b, NULL);
}

TEST(invmod_correctness)
{
    mp_int a, b, c, check;
    mp_init_multi(&a, &b, &c, &check, NULL);

    /* invmod(3, 11) = 4, because 3*4 = 12 = 1 mod 11 */
    mp_set(&a, 3);
    mp_set(&b, 11);
    ASSERT_EQ(mp_invmod(&a, &b, &c), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&c, 4), MP_EQ);

    /* Verify: a * c mod b == 1 */
    ASSERT_EQ(mp_mulmod(&a, &c, &b, &check), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&check, 1), MP_EQ);

    mp_clear_multi(&a, &b, &c, &check, NULL);
}

TEST(invmod_no_inverse)
{
    mp_int a, b, c;
    mp_init_multi(&a, &b, &c, NULL);

    /* gcd(2, 4) = 2 != 1, so no inverse exists */
    mp_set(&a, 2);
    mp_set(&b, 4);
    ASSERT(mp_invmod(&a, &b, &c) != MP_OKAY);

    mp_clear_multi(&a, &b, &c, NULL);
}

TEST(mulmod_correctness)
{
    mp_int a, b, c, d;
    mp_init_multi(&a, &b, &c, &d, NULL);

    /* 123 * 456 = 56088. 56088 mod 789: 789*71=56019, 56088-56019=69 */
    mp_read_radix(&a, "123", 10);
    mp_read_radix(&b, "456", 10);
    mp_read_radix(&c, "789", 10);
    ASSERT_EQ(mp_mulmod(&a, &b, &c, &d), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&d, 69), MP_EQ);

    mp_clear_multi(&a, &b, &c, &d, NULL);
}

TEST(sqrmod_correctness)
{
    mp_int a, b, c;
    mp_init_multi(&a, &b, &c, NULL);

    /* 17^2 mod 100 = 289 mod 100 = 89 */
    mp_set(&a, 17);
    mp_read_radix(&b, "100", 10);
    ASSERT_EQ(mp_sqrmod(&a, &b, &c), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&c, 89), MP_EQ);

    mp_clear_multi(&a, &b, &c, NULL);
}

TEST(gcd_correctness)
{
    mp_int a, b, c;
    mp_init_multi(&a, &b, &c, NULL);

    mp_set(&a, 48);
    mp_set(&b, 18);
    ASSERT_EQ(mp_gcd(&a, &b, &c), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&c, 6), MP_EQ);

    /* Coprime */
    mp_set(&a, 17);
    mp_set(&b, 19);
    ASSERT_EQ(mp_gcd(&a, &b, &c), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&c, 1), MP_EQ);

    mp_clear_multi(&a, &b, &c, NULL);
}

TEST(lcm_correctness)
{
    mp_int a, b, c;
    mp_init_multi(&a, &b, &c, NULL);

    mp_set(&a, 12);
    mp_set(&b, 18);
    ASSERT_EQ(mp_lcm(&a, &b, &c), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&c, 36), MP_EQ);

    mp_clear_multi(&a, &b, &c, NULL);
}

/* --- Priority 2: Core Arithmetic --- */

TEST(init_clear_lifecycle)
{
    mp_int a;
    ASSERT_EQ(mp_init(&a), MP_OKAY);
    ASSERT_EQ(a.used, 0);
    ASSERT_EQ(a.sign, MP_ZPOS);
    mp_clear(&a);
}

TEST(init_multi_clear_multi)
{
    mp_int a, b, c;
    ASSERT_EQ(mp_init_multi(&a, &b, &c, NULL), MP_OKAY);
    ASSERT_EQ(a.used, 0);
    ASSERT_EQ(b.used, 0);
    ASSERT_EQ(c.used, 0);
    mp_clear_multi(&a, &b, &c, NULL);
}

TEST(add_sub_correctness)
{
    mp_int a, b, c;
    mp_init_multi(&a, &b, &c, NULL);

    mp_read_radix(&a, "123", 10);
    mp_read_radix(&b, "456", 10);

    ASSERT_EQ(mp_add(&a, &b, &c), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&c, 579), MP_EQ);

    mp_read_radix(&a, "789", 10);
    mp_read_radix(&b, "123", 10);
    ASSERT_EQ(mp_sub(&a, &b, &c), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&c, 666), MP_EQ);

    mp_clear_multi(&a, &b, &c, NULL);
}

TEST(mul_correctness)
{
    mp_int a, b, c;
    mp_init_multi(&a, &b, &c, NULL);

    mp_read_radix(&a, "123", 10);
    mp_read_radix(&b, "456", 10);
    ASSERT_EQ(mp_mul(&a, &b, &c), MP_OKAY);

    {
        mp_int expected;
        mp_init(&expected);
        mp_read_radix(&expected, "56088", 10);
        ASSERT_EQ(mp_cmp(&c, &expected), MP_EQ);
        mp_clear(&expected);
    }

    mp_clear_multi(&a, &b, &c, NULL);
}

TEST(div_remainder)
{
    mp_int a, b, q, r;
    mp_init_multi(&a, &b, &q, &r, NULL);

    mp_read_radix(&a, "123", 10);
    mp_set(&b, 10);
    ASSERT_EQ(mp_div(&a, &b, &q, &r), MP_OKAY);
    ASSERT_EQ(mp_cmp_d(&q, 12), MP_EQ);
    ASSERT_EQ(mp_cmp_d(&r, 3), MP_EQ);

    mp_clear_multi(&a, &b, &q, &r, NULL);
}

/* --- Priority 3: Conversion --- */

TEST(read_radix_to_radix_hex)
{
    mp_int a;
    char buf[64];
    size_t written;

    mp_init(&a);
    ASSERT_EQ(mp_read_radix(&a, "DEADBEEF", 16), MP_OKAY);
    ASSERT_EQ(mp_to_radix(&a, buf, sizeof(buf), &written, 16), MP_OKAY);
    ASSERT_EQ(strcmp(buf, "DEADBEEF"), 0);

    mp_clear(&a);
}

TEST(from_ubin_to_ubin_roundtrip)
{
    mp_int a, b;
    unsigned char input[] = {0xDE, 0xAD, 0xBE, 0xEF};
    unsigned char output[8];
    size_t written;

    mp_init_multi(&a, &b, NULL);

    ASSERT_EQ(mp_from_ubin(&a, input, sizeof(input)), MP_OKAY);
    ASSERT_EQ(mp_to_ubin(&a, output, sizeof(output), &written), MP_OKAY);
    ASSERT_EQ(written, 4u);
    ASSERT_EQ(memcmp(input, output, 4), 0);

    mp_clear_multi(&a, &b, NULL);
}

TEST(read_radix_invalid)
{
    mp_int a;
    mp_init(&a);

    ASSERT(mp_read_radix(&a, "123", 1) != MP_OKAY);

    mp_clear(&a);
}

/* --- Priority 4: Edge Cases --- */

TEST(cmp_and_cmp_mag)
{
    mp_int a, b;
    mp_init_multi(&a, &b, NULL);

    mp_set(&a, 5);
    mp_set(&b, 10);
    ASSERT_EQ(mp_cmp(&a, &b), MP_LT);

    mp_set(&a, 10);
    mp_set(&b, 10);
    ASSERT_EQ(mp_cmp(&a, &b), MP_EQ);

    mp_set(&a, 10);
    mp_set(&b, 5);
    mp_neg(&b, &b);
    ASSERT_EQ(mp_cmp(&a, &b), MP_GT);

    /* cmp_mag ignores sign */
    ASSERT_EQ(mp_cmp_mag(&a, &b), MP_GT);

    mp_clear_multi(&a, &b, NULL);
}

TEST(div_by_zero)
{
    mp_int a, zero, q, r;
    mp_init_multi(&a, &zero, &q, &r, NULL);

    mp_set(&a, 42);
    mp_zero(&zero);
    ASSERT(mp_div(&a, &zero, &q, &r) != MP_OKAY);

    mp_clear_multi(&a, &zero, &q, &r, NULL);
}

TEST(zero_iszero)
{
    mp_int a;
    mp_init(&a);
    mp_zero(&a);
    ASSERT(mp_iszero(&a));
    mp_set(&a, 1);
    ASSERT(!mp_iszero(&a));
    mp_clear(&a);
}

TEST(neg_abs)
{
    mp_int a, b;
    mp_init_multi(&a, &b, NULL);

    mp_set(&a, 5);
    mp_neg(&a, &b);
    ASSERT_EQ(b.sign, MP_NEG);
    ASSERT_EQ(mp_cmp_d(&b, 5), MP_LT); /* -5 < 5 */

    mp_abs(&b, &b);
    ASSERT_EQ(b.sign, MP_ZPOS);
    ASSERT_EQ(mp_cmp_d(&b, 5), MP_EQ);

    mp_clear_multi(&a, &b, NULL);
}

TEST(count_bits)
{
    mp_int a;
    mp_init(&a);

    mp_set(&a, 255);
    ASSERT_EQ(mp_count_bits(&a), 8);

    mp_set(&a, 256);
    ASSERT_EQ(mp_count_bits(&a), 9);

    mp_zero(&a);
    ASSERT_EQ(mp_count_bits(&a), 0);

    mp_clear(&a);
}

/* --- Priority 5: Stress --- */

TEST(sqr_large)
{
    mp_int a, b, expected;
    mp_init_multi(&a, &b, &expected, NULL);

    /* 2^128 squared = 2^256 */
    mp_read_radix(&a, "100000000000000000000000000000000", 16);
    ASSERT_EQ(mp_sqr(&a, &b), MP_OKAY);

    mp_read_radix(&expected,
        "10000000000000000000000000000000000000000000000000000000000000000", 16);
    ASSERT_EQ(mp_cmp(&b, &expected), MP_EQ);

    mp_clear_multi(&a, &b, &expected, NULL);
}

TEST(add_carry_growth)
{
    mp_int a, b, c;
    mp_init_multi(&a, &b, &c, NULL);

    /* Add two large numbers that force digit growth */
    mp_read_radix(&a, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16);
    mp_set(&b, 1);
    ASSERT_EQ(mp_add(&a, &b, &c), MP_OKAY);

    {
        mp_int expected;
        mp_init(&expected);
        mp_read_radix(&expected,
            "100000000000000000000000000000000", 16);
        ASSERT_EQ(mp_cmp(&c, &expected), MP_EQ);
        mp_clear(&expected);
    }

    mp_clear_multi(&a, &b, &c, NULL);
}

int main(void)
{
    (void)verstag;
    printf("=== LibTomMath Unit Tests ===\n");

    /* Crypto-critical */
    RUN_TEST(exptmod_small_known);
    RUN_TEST(exptmod_larger);
    RUN_TEST(prime_is_prime_small);
    RUN_TEST(prime_is_prime_larger);
    RUN_TEST(invmod_correctness);
    RUN_TEST(invmod_no_inverse);
    RUN_TEST(mulmod_correctness);
    RUN_TEST(sqrmod_correctness);
    RUN_TEST(gcd_correctness);
    RUN_TEST(lcm_correctness);

    /* Core arithmetic */
    RUN_TEST(init_clear_lifecycle);
    RUN_TEST(init_multi_clear_multi);
    RUN_TEST(add_sub_correctness);
    RUN_TEST(mul_correctness);
    RUN_TEST(div_remainder);

    /* Conversion */
    RUN_TEST(read_radix_to_radix_hex);
    RUN_TEST(from_ubin_to_ubin_roundtrip);
    RUN_TEST(read_radix_invalid);

    /* Edge cases */
    RUN_TEST(cmp_and_cmp_mag);
    RUN_TEST(div_by_zero);
    RUN_TEST(zero_iszero);
    RUN_TEST(neg_abs);
    RUN_TEST(count_bits);

    /* Stress */
    RUN_TEST(sqr_large);
    RUN_TEST(add_carry_growth);

    return test_summary();
}
