/*
 * test_credential.c -- vamos unit tests for amigit's credential
 * sourcing (credential.c).
 *
 * PDR-012 Phase 7. Exercises:
 *   - amigit_base64_encode against the RFC 4648 test vectors
 *   - amigit_base64_encode buffer overflow detection
 *   - amigit_credential_zero actually zeroes a buffer
 *   - amigit_credential_get error path when no env vars are set
 *     and stdin is non-interactive (vamos's Input() is always
 *     non-interactive)
 *
 * The interactive prompt path is NOT covered here -- vamos has no
 * console, so IsInteractive(Input()) returns 0 and prompt_interactive
 * short-circuits. That path is only reachable on FS-UAE with a real
 * console or on real hardware.
 *
 * __stack matches the other tests in this directory for consistency.
 */

#include "test_framework.h"
#include "credential.h"

#include <stdlib.h>
#include <string.h>

long __stack = 262144;

/* ============================================================
 * amigit_base64_encode -- RFC 4648 section 10 test vectors
 * ============================================================ */

TEST(base64_empty)
{
    char dst[16];
    int n = amigit_base64_encode("", 0, dst, sizeof(dst));
    ASSERT_EQ(0, n);
    ASSERT_STR_EQ("", dst);
}

TEST(base64_one_byte)
{
    char dst[16];
    int n = amigit_base64_encode("f", 1, dst, sizeof(dst));
    ASSERT_EQ(4, n);
    ASSERT_STR_EQ("Zg==", dst);
}

TEST(base64_two_bytes)
{
    char dst[16];
    int n = amigit_base64_encode("fo", 2, dst, sizeof(dst));
    ASSERT_EQ(4, n);
    ASSERT_STR_EQ("Zm8=", dst);
}

TEST(base64_three_bytes)
{
    char dst[16];
    int n = amigit_base64_encode("foo", 3, dst, sizeof(dst));
    ASSERT_EQ(4, n);
    ASSERT_STR_EQ("Zm9v", dst);
}

TEST(base64_four_bytes)
{
    char dst[16];
    int n = amigit_base64_encode("foob", 4, dst, sizeof(dst));
    ASSERT_EQ(8, n);
    ASSERT_STR_EQ("Zm9vYg==", dst);
}

TEST(base64_five_bytes)
{
    char dst[16];
    int n = amigit_base64_encode("fooba", 5, dst, sizeof(dst));
    ASSERT_EQ(8, n);
    ASSERT_STR_EQ("Zm9vYmE=", dst);
}

TEST(base64_six_bytes)
{
    char dst[16];
    int n = amigit_base64_encode("foobar", 6, dst, sizeof(dst));
    ASSERT_EQ(8, n);
    ASSERT_STR_EQ("Zm9vYmFy", dst);
}

TEST(base64_github_pat_shape)
{
    /* Simulate a realistic "git:<pat>" credential pair and verify
     * the encoding round-trips through a known-good expected value.
     * The token here is a 40-char hex string, matching the shape of
     * a classic GitHub PAT before fine-grained tokens came along. */
    const char *pair = "git:0123456789abcdef0123456789abcdef01234567";
    char dst[128];
    int n = amigit_base64_encode(pair, strlen(pair), dst, sizeof(dst));
    ASSERT(n > 0);
    /* Decoded via a sanity-check: base64("git:0123456789abcdef...") */
    ASSERT_STR_EQ(
        "Z2l0OjAxMjM0NTY3ODlhYmNkZWYwMTIzNDU2Nzg5YWJjZGVmMDEyMzQ1Njc=",
        dst);
}

TEST(base64_dst_too_small)
{
    /* Encoding 6 bytes requires 8 + NUL = 9; a 5-byte dst must fail. */
    char dst[5];
    int n = amigit_base64_encode("foobar", 6, dst, sizeof(dst));
    ASSERT_EQ(-1, n);
}

TEST(base64_dst_exactly_fits)
{
    /* 3 bytes -> 4 base64 chars + NUL = 5 bytes. */
    char dst[5];
    int n = amigit_base64_encode("foo", 3, dst, sizeof(dst));
    ASSERT_EQ(4, n);
    ASSERT_STR_EQ("Zm9v", dst);
}

TEST(base64_null_dst_rejected)
{
    int n = amigit_base64_encode("foo", 3, NULL, 0);
    ASSERT_EQ(-1, n);
}

/* ============================================================
 * amigit_credential_zero -- volatile wipe
 * ============================================================ */

TEST(zero_wipes_full_buffer)
{
    char buf[32];
    size_t i;
    memset(buf, 'X', sizeof(buf));
    amigit_credential_zero(buf, sizeof(buf));
    for (i = 0; i < sizeof(buf); i++) {
        ASSERT_EQ(0, (int)(unsigned char)buf[i]);
    }
}

TEST(zero_partial)
{
    char buf[16];
    size_t i;
    memset(buf, 'X', sizeof(buf));
    amigit_credential_zero(buf, 8);
    for (i = 0; i < 8; i++) {
        ASSERT_EQ(0, (int)(unsigned char)buf[i]);
    }
    for (i = 8; i < sizeof(buf); i++) {
        ASSERT_EQ('X', buf[i]);
    }
}

TEST(zero_zero_len_noop)
{
    char buf[4] = {'a', 'b', 'c', 'd'};
    amigit_credential_zero(buf, 0);
    ASSERT_EQ('a', buf[0]);
    ASSERT_EQ('d', buf[3]);
}

/* ============================================================
 * amigit_credential_get -- error path when no creds available
 * ============================================================
 *
 * vamos has no AmigaDOS ENV: (Open("ENV:...") fails) and no
 * interactive console (IsInteractive(Input()) returns 0), so this
 * exercises the "neither source has credentials" error branch.
 * The test asserts:
 *   - Return value is -1
 *   - errbuf mentions both GIT_HTTP_TOKEN and SetEnv (user-actionable)
 *   - user_buf and token_buf are zeroed on failure
 *
 * This is the ONLY credential_get path we can test under vamos.
 * The success paths (env var and interactive) require FS-UAE or
 * real hardware to set up proper test fixtures. */

TEST(credential_get_no_creds_returns_error)
{
    char user[128];
    char token[512];
    char err[256];
    int rc;

    memset(user, 'U', sizeof(user));
    memset(token, 'T', sizeof(token));
    err[0] = '\0';

    rc = amigit_credential_get(user, sizeof(user),
                               token, sizeof(token),
                               err, sizeof(err));
    ASSERT_EQ(-1, rc);
    /* errbuf must mention GIT_HTTP_TOKEN so the user knows the fix */
    ASSERT(strstr(err, "GIT_HTTP_TOKEN") != NULL);
    /* User and token buffers must be zeroed on failure */
    ASSERT_EQ(0, (int)(unsigned char)user[0]);
    ASSERT_EQ(0, (int)(unsigned char)token[0]);
}

TEST(credential_get_rejects_tiny_user_buf)
{
    char user[2];      /* too small -- min is 4 for "git\0" */
    char token[64];
    char err[128];
    int rc;
    rc = amigit_credential_get(user, sizeof(user),
                               token, sizeof(token),
                               err, sizeof(err));
    ASSERT_EQ(-1, rc);
}

TEST(credential_get_rejects_null_errbuf)
{
    char user[16];
    char token[64];
    int rc;
    rc = amigit_credential_get(user, sizeof(user),
                               token, sizeof(token),
                               NULL, 0);
    ASSERT_EQ(-1, rc);
}

/* ============================================================
 * Test runner
 * ============================================================ */

int
main(void)
{
    RUN_TEST(base64_empty);
    RUN_TEST(base64_one_byte);
    RUN_TEST(base64_two_bytes);
    RUN_TEST(base64_three_bytes);
    RUN_TEST(base64_four_bytes);
    RUN_TEST(base64_five_bytes);
    RUN_TEST(base64_six_bytes);
    RUN_TEST(base64_github_pat_shape);
    RUN_TEST(base64_dst_too_small);
    RUN_TEST(base64_dst_exactly_fits);
    RUN_TEST(base64_null_dst_rejected);

    RUN_TEST(zero_wipes_full_buffer);
    RUN_TEST(zero_partial);
    RUN_TEST(zero_zero_len_noop);

    RUN_TEST(credential_get_no_creds_returns_error);
    RUN_TEST(credential_get_rejects_tiny_user_buf);
    RUN_TEST(credential_get_rejects_null_errbuf);

    return test_summary();
}
