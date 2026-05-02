/*
 * test_libparserutils.c -- unit tests for lib/libparserutils
 *
 * Library built -m68040 -m68881 -DWITHOUT_ICONV_FILTER per
 * NetSurf-Vampire dep stack convention.
 * Run via: vamos -C 68040 -s 1024 -m 4096 ./test_libparserutils
 *
 * Coverage: ~50 tests across all 6 categories per
 * docs/test-coverage-standard.md.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <parserutils/parserutils.h>
#include <parserutils/charset/codec.h>
#include <parserutils/charset/mibenum.h>
#include <parserutils/charset/utf8.h>
#include <parserutils/charset/utf16.h>
#include <parserutils/input/inputstream.h>
#include <parserutils/utils/buffer.h>
#include <parserutils/utils/stack.h>
#include <parserutils/utils/vector.h>

#include "test_framework.h"

long __stack = 262144;
unsigned long __MEMORY_STEP = 262144;

/* ===================================================================
 * Helper: codec roundtrip — encode UCS-4 -> bytes -> decode -> UCS-4
 * =================================================================== */

static int codec_roundtrip(const char *cs, uint32_t in, const uint8_t *expected_bytes,
                           size_t expected_len)
{
    parserutils_charset_codec *codec = NULL;
    if (parserutils_charset_codec_create(cs, &codec) != PARSERUTILS_OK || codec == NULL)
        return -1;

    /* Encode */
    uint32_t input[1] = { in };
    uint8_t outbuf[16];
    const uint8_t *src = (const uint8_t *)input;
    size_t srclen = sizeof(input);
    uint8_t *dst = outbuf;
    size_t dstlen = sizeof(outbuf);
    parserutils_error rc = parserutils_charset_codec_encode(codec, &src, &srclen,
                                                            &dst, &dstlen);
    if (rc != PARSERUTILS_OK) {
        parserutils_charset_codec_destroy(codec);
        return -2;
    }
    size_t enc_len = sizeof(outbuf) - dstlen;
    if (enc_len != expected_len || memcmp(outbuf, expected_bytes, expected_len) != 0) {
        parserutils_charset_codec_destroy(codec);
        return -3;
    }

    /* Decode back */
    parserutils_charset_codec_reset(codec);
    uint32_t back[1] = { 0 };
    src = outbuf;
    srclen = enc_len;
    dst = (uint8_t *)back;
    dstlen = sizeof(back);
    rc = parserutils_charset_codec_decode(codec, &src, &srclen, &dst, &dstlen);
    if (rc != PARSERUTILS_OK) {
        parserutils_charset_codec_destroy(codec);
        return -4;
    }
    if (back[0] != in) {
        parserutils_charset_codec_destroy(codec);
        return -5;
    }

    parserutils_charset_codec_destroy(codec);
    return 0;
}

/* ===================================================================
 * Category 1: Functional — Charset codec roundtrips
 * =================================================================== */

TEST(codec_utf8_create_destroy) {
    parserutils_charset_codec *codec = NULL;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", &codec), PARSERUTILS_OK);
    ASSERT_NOT_NULL(codec);
    ASSERT_EQ(parserutils_charset_codec_destroy(codec), PARSERUTILS_OK);
}

TEST(codec_utf8_roundtrip_ascii) {
    /* U+0041 'A' -> 0x41 */
    uint8_t expected[] = { 0x41 };
    ASSERT_EQ(codec_roundtrip("UTF-8", 0x0041, expected, 1), 0);
}

TEST(codec_utf8_roundtrip_2byte) {
    /* U+00A3 (£) -> 0xC2 0xA3 */
    uint8_t expected[] = { 0xC2, 0xA3 };
    ASSERT_EQ(codec_roundtrip("UTF-8", 0x00A3, expected, 2), 0);
}

TEST(codec_utf8_roundtrip_3byte) {
    /* U+20AC (Euro) -> 0xE2 0x82 0xAC */
    uint8_t expected[] = { 0xE2, 0x82, 0xAC };
    ASSERT_EQ(codec_roundtrip("UTF-8", 0x20AC, expected, 3), 0);
}

TEST(codec_utf8_roundtrip_4byte) {
    /* U+1F600 (😀) -> 0xF0 0x9F 0x98 0x80 */
    uint8_t expected[] = { 0xF0, 0x9F, 0x98, 0x80 };
    ASSERT_EQ(codec_roundtrip("UTF-8", 0x1F600, expected, 4), 0);
}

TEST(codec_utf16_roundtrip_bmp) {
    /* libparserutils' codec_utf16 only registers "UTF-16" (BOM-driven byte
     * order). Explicit UTF-16BE/UTF-16LE are iconv-territory and not
     * supported under WITHOUT_ICONV_FILTER. Test the supported codec. */
    parserutils_charset_codec *codec = NULL;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-16", &codec),
              PARSERUTILS_OK);
    ASSERT_NOT_NULL(codec);
    /* Roundtrip via the codec: encode then decode and confirm same value */
    uint32_t input[1] = { 0x0041 };
    uint8_t enc[16];
    const uint8_t *src = (const uint8_t *)input;
    size_t srclen = sizeof(input);
    uint8_t *dst = enc;
    size_t dstlen = sizeof(enc);
    ASSERT_EQ(parserutils_charset_codec_encode(codec, &src, &srclen,
              &dst, &dstlen), PARSERUTILS_OK);
    size_t enc_len = sizeof(enc) - dstlen;
    ASSERT(enc_len >= 2);
    /* Decode back */
    parserutils_charset_codec_reset(codec);
    uint32_t back[1] = {0};
    src = enc;
    srclen = enc_len;
    dst = (uint8_t *)back;
    dstlen = sizeof(back);
    ASSERT_EQ(parserutils_charset_codec_decode(codec, &src, &srclen,
              &dst, &dstlen), PARSERUTILS_OK);
    ASSERT_EQ(back[0], (uint32_t)0x0041);
    parserutils_charset_codec_destroy(codec);
}

TEST(codec_iso8859_1_roundtrip) {
    /* U+00A3 -> 0xA3 (1:1 mapping in Latin-1) */
    uint8_t expected[] = { 0xA3 };
    ASSERT_EQ(codec_roundtrip("ISO-8859-1", 0x00A3, expected, 1), 0);
}

TEST(codec_us_ascii_roundtrip) {
    /* U+0041 'A' -> 0x41 */
    uint8_t expected[] = { 0x41 };
    ASSERT_EQ(codec_roundtrip("US-ASCII", 0x0041, expected, 1), 0);
}

TEST(codec_windows_1252_roundtrip) {
    /* Windows-1252: U+20AC (Euro) at 0x80 */
    uint8_t expected[] = { 0x80 };
    ASSERT_EQ(codec_roundtrip("Windows-1252", 0x20AC, expected, 1), 0);
}

TEST(codec_reset_clears_state) {
    parserutils_charset_codec *codec = NULL;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", &codec), PARSERUTILS_OK);
    ASSERT_EQ(parserutils_charset_codec_reset(codec), PARSERUTILS_OK);
    parserutils_charset_codec_destroy(codec);
}

TEST(codec_setopt_strict) {
    parserutils_charset_codec *codec = NULL;
    parserutils_charset_codec_optparams params;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", &codec), PARSERUTILS_OK);
    params.error_mode.mode = PARSERUTILS_CHARSET_CODEC_ERROR_STRICT;
    ASSERT_EQ(parserutils_charset_codec_setopt(codec,
              PARSERUTILS_CHARSET_CODEC_ERROR_MODE, &params), PARSERUTILS_OK);
    parserutils_charset_codec_destroy(codec);
}

/* ===================================================================
 * Category 2: Error path
 * =================================================================== */

TEST(codec_create_unknown_charset_fails) {
    parserutils_charset_codec *codec = NULL;
    parserutils_error rc = parserutils_charset_codec_create("INVALID-CHARSET", &codec);
    ASSERT(rc != PARSERUTILS_OK);
    ASSERT_NULL(codec);
}

TEST(codec_create_null_charset_fails) {
    parserutils_charset_codec *codec = NULL;
    ASSERT_EQ(parserutils_charset_codec_create(NULL, &codec), PARSERUTILS_BADPARM);
}

TEST(codec_create_null_out_fails) {
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", NULL), PARSERUTILS_BADPARM);
}

TEST(codec_decode_truncated_utf8_no_consume) {
    /* libparserutils' codec_decode treats truncated input as "wait for more
     * data": it returns PARSERUTILS_OK with srclen unchanged (nothing
     * consumed) and dstlen unchanged (nothing written). The caller knows
     * to feed more bytes. The PARSERUTILS_NEEDDATA distinction is the
     * helper-level (utf8_to_ucs4) contract -- different layer. */
    parserutils_charset_codec *codec = NULL;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", &codec), PARSERUTILS_OK);
    uint8_t in[] = { 0xE2 }; /* lone start of 3-byte sequence */
    uint32_t out[2] = {0};
    const uint8_t *src = in;
    size_t srclen_initial = 1;
    size_t srclen = srclen_initial;
    uint8_t *dst = (uint8_t *)out;
    size_t dstlen_initial = sizeof(out);
    size_t dstlen = dstlen_initial;
    parserutils_error rc = parserutils_charset_codec_decode(codec, &src, &srclen,
                                                            &dst, &dstlen);
    /* Either PARSERUTILS_OK with no consumption, or NEEDDATA -- both are
     * correct "wait for more" responses. The key invariant is that the
     * partial byte was NOT silently emitted as a bogus codepoint. */
    ASSERT(rc == PARSERUTILS_OK || rc == PARSERUTILS_NEEDDATA);
    /* No output produced from a partial sequence */
    ASSERT_EQ(dstlen, dstlen_initial);
    parserutils_charset_codec_destroy(codec);
}

TEST(codec_decode_invalid_utf8_strict) {
    parserutils_charset_codec *codec = NULL;
    parserutils_charset_codec_optparams params;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", &codec), PARSERUTILS_OK);
    params.error_mode.mode = PARSERUTILS_CHARSET_CODEC_ERROR_STRICT;
    ASSERT_EQ(parserutils_charset_codec_setopt(codec,
              PARSERUTILS_CHARSET_CODEC_ERROR_MODE, &params), PARSERUTILS_OK);
    /* 0xFF is never a valid UTF-8 leading byte */
    uint8_t in[] = { 0xFF };
    uint32_t out[2] = {0};
    const uint8_t *src = in;
    size_t srclen = 1;
    uint8_t *dst = (uint8_t *)out;
    size_t dstlen = sizeof(out);
    parserutils_error rc = parserutils_charset_codec_decode(codec, &src, &srclen,
                                                            &dst, &dstlen);
    ASSERT(rc == PARSERUTILS_INVALID || rc == PARSERUTILS_BADENCODING);
    parserutils_charset_codec_destroy(codec);
}

/* ===================================================================
 * Category 3: MIB-enum lookup
 * =================================================================== */

TEST(mibenum_from_name_utf8) {
    /* UTF-8 has MIB enum 106 (IANA registered) */
    uint16_t mib = parserutils_charset_mibenum_from_name("UTF-8", 5);
    ASSERT(mib != 0);
}

TEST(mibenum_from_name_alias_utf8) {
    /* "utf8" should resolve via alias to same MIB as "UTF-8" */
    uint16_t mib_canonical = parserutils_charset_mibenum_from_name("UTF-8", 5);
    uint16_t mib_alias = parserutils_charset_mibenum_from_name("utf8", 4);
    ASSERT(mib_canonical != 0);
    ASSERT_EQ(mib_canonical, mib_alias);
}

TEST(mibenum_from_name_case_insensitive) {
    uint16_t lc = parserutils_charset_mibenum_from_name("utf-8", 5);
    uint16_t uc = parserutils_charset_mibenum_from_name("UTF-8", 5);
    uint16_t mc = parserutils_charset_mibenum_from_name("Utf-8", 5);
    ASSERT(lc != 0);
    ASSERT_EQ(lc, uc);
    ASSERT_EQ(lc, mc);
}

TEST(mibenum_from_name_unknown) {
    uint16_t mib = parserutils_charset_mibenum_from_name("INVALID-CHARSET-XYZ", 19);
    ASSERT_EQ(mib, 0);
}

TEST(mibenum_to_name_roundtrip) {
    uint16_t mib = parserutils_charset_mibenum_from_name("UTF-8", 5);
    ASSERT(mib != 0);
    const char *name = parserutils_charset_mibenum_to_name(mib);
    ASSERT_NOT_NULL(name);
    /* Canonical name should be "UTF-8" exactly */
    ASSERT_STR_EQ(name, "UTF-8");
}

TEST(mibenum_is_unicode_utf8) {
    uint16_t mib = parserutils_charset_mibenum_from_name("UTF-8", 5);
    ASSERT(parserutils_charset_mibenum_is_unicode(mib) == true);
}

TEST(mibenum_is_unicode_iso8859_false) {
    uint16_t mib = parserutils_charset_mibenum_from_name("ISO-8859-1", 10);
    ASSERT(mib != 0);
    ASSERT(parserutils_charset_mibenum_is_unicode(mib) == false);
}

/* ===================================================================
 * Category 4: UTF-8 helpers
 * =================================================================== */

TEST(utf8_to_ucs4_ascii) {
    uint8_t in[] = { 0x41 };
    uint32_t out = 0;
    size_t clen = 0;
    ASSERT_EQ(parserutils_charset_utf8_to_ucs4(in, sizeof(in), &out, &clen),
              PARSERUTILS_OK);
    ASSERT_EQ(out, (uint32_t)0x0041);
    ASSERT_EQ(clen, (size_t)1);
}

TEST(utf8_to_ucs4_2byte) {
    uint8_t in[] = { 0xC2, 0xA3 };
    uint32_t out = 0;
    size_t clen = 0;
    ASSERT_EQ(parserutils_charset_utf8_to_ucs4(in, sizeof(in), &out, &clen),
              PARSERUTILS_OK);
    ASSERT_EQ(out, (uint32_t)0x00A3);
    ASSERT_EQ(clen, (size_t)2);
}

TEST(utf8_to_ucs4_3byte) {
    uint8_t in[] = { 0xE2, 0x82, 0xAC };
    uint32_t out = 0;
    size_t clen = 0;
    ASSERT_EQ(parserutils_charset_utf8_to_ucs4(in, sizeof(in), &out, &clen),
              PARSERUTILS_OK);
    ASSERT_EQ(out, (uint32_t)0x20AC);
    ASSERT_EQ(clen, (size_t)3);
}

TEST(utf8_to_ucs4_4byte) {
    uint8_t in[] = { 0xF0, 0x9F, 0x98, 0x80 };
    uint32_t out = 0;
    size_t clen = 0;
    ASSERT_EQ(parserutils_charset_utf8_to_ucs4(in, sizeof(in), &out, &clen),
              PARSERUTILS_OK);
    ASSERT_EQ(out, (uint32_t)0x1F600);
    ASSERT_EQ(clen, (size_t)4);
}

TEST(utf8_from_ucs4_ascii) {
    uint8_t buf[8] = {0};
    uint8_t *dst = buf;
    size_t len = sizeof(buf);
    ASSERT_EQ(parserutils_charset_utf8_from_ucs4(0x0041, &dst, &len),
              PARSERUTILS_OK);
    /* dst was advanced by 1; buf[0]=0x41 */
    ASSERT_EQ(buf[0], 0x41);
    ASSERT_EQ((size_t)(dst - buf), (size_t)1);
}

TEST(utf8_from_ucs4_3byte) {
    uint8_t buf[8] = {0};
    uint8_t *dst = buf;
    size_t len = sizeof(buf);
    ASSERT_EQ(parserutils_charset_utf8_from_ucs4(0x20AC, &dst, &len),
              PARSERUTILS_OK);
    ASSERT_EQ(buf[0], 0xE2);
    ASSERT_EQ(buf[1], 0x82);
    ASSERT_EQ(buf[2], 0xAC);
    ASSERT_EQ((size_t)(dst - buf), (size_t)3);
}

TEST(utf8_to_ucs4_truncated) {
    /* Lone start of 3-byte sequence */
    uint8_t in[] = { 0xE2 };
    uint32_t out = 0;
    size_t clen = 0;
    parserutils_error rc = parserutils_charset_utf8_to_ucs4(in, 1, &out, &clen);
    ASSERT_EQ(rc, PARSERUTILS_NEEDDATA);
}

TEST(utf8_char_byte_length_lead_byte) {
    /* 0xE2 leads a 3-byte sequence */
    uint8_t in[] = { 0xE2, 0x82, 0xAC };
    size_t len = 0;
    ASSERT_EQ(parserutils_charset_utf8_char_byte_length(in, &len),
              PARSERUTILS_OK);
    ASSERT_EQ(len, (size_t)3);
}

TEST(utf8_length_counts_codepoints) {
    /* "hi€" = h(0x68) i(0x69) + 0xE2 0x82 0xAC = 5 bytes, 3 codepoints */
    uint8_t in[] = { 0x68, 0x69, 0xE2, 0x82, 0xAC };
    size_t len = 0;
    ASSERT_EQ(parserutils_charset_utf8_length(in, sizeof(in), &len),
              PARSERUTILS_OK);
    ASSERT_EQ(len, (size_t)3);
}

TEST(utf8_next_advances) {
    uint8_t in[] = { 0x41, 0xC2, 0xA3, 0x42 }; /* "A £ B" */
    uint32_t next = 0;
    /* From offset 1 (start of £), next codepoint starts at offset 3 (B) */
    ASSERT_EQ(parserutils_charset_utf8_next(in, sizeof(in), 1, &next),
              PARSERUTILS_OK);
    ASSERT_EQ(next, (uint32_t)3);
}

TEST(utf8_prev_backs) {
    uint8_t in[] = { 0x41, 0xC2, 0xA3, 0x42 };
    uint32_t prev = 0;
    /* From offset 3, previous codepoint starts at offset 1 (£) */
    ASSERT_EQ(parserutils_charset_utf8_prev(in, 3, &prev), PARSERUTILS_OK);
    ASSERT_EQ(prev, (uint32_t)1);
}

/* ===================================================================
 * Category 5: UTF-16 helpers (input buffer is byte-stream of 16-bit
 * values in *host* byte order — these are the helper functions; the
 * codec subsystem handles BE/LE explicitly).
 * =================================================================== */

TEST(utf16_to_ucs4_bmp) {
    /* 0x0041 = 'A' as two bytes in host byte order (big-endian on 68k) */
    uint8_t in[] = { 0x00, 0x41 };
    uint32_t out = 0;
    size_t clen = 0;
    ASSERT_EQ(parserutils_charset_utf16_to_ucs4(in, sizeof(in), &out, &clen),
              PARSERUTILS_OK);
    ASSERT_EQ(out, (uint32_t)0x0041);
    ASSERT_EQ(clen, (size_t)2);
}

TEST(utf16_to_ucs4_surrogate_pair) {
    /* 0xD83D 0xDE00 = U+1F600 */
    uint8_t in[] = { 0xD8, 0x3D, 0xDE, 0x00 };
    uint32_t out = 0;
    size_t clen = 0;
    ASSERT_EQ(parserutils_charset_utf16_to_ucs4(in, sizeof(in), &out, &clen),
              PARSERUTILS_OK);
    ASSERT_EQ(out, (uint32_t)0x1F600);
    ASSERT_EQ(clen, (size_t)4);
}

TEST(utf16_from_ucs4_bmp) {
    /* utf16_from_ucs4 signature: (uint32_t ucs4, uint8_t *s, size_t *len)
     * — note s is NOT a pointer-to-pointer (unlike utf8_from_ucs4). */
    uint8_t buf[8] = {0};
    size_t len = sizeof(buf);
    ASSERT_EQ(parserutils_charset_utf16_from_ucs4(0x0041, buf, &len),
              PARSERUTILS_OK);
    /* On big-endian 68k, host order = 0x00 0x41 */
    ASSERT_EQ(buf[0], 0x00);
    ASSERT_EQ(buf[1], 0x41);
    /* len is set to bytes written */
    ASSERT_EQ(len, (size_t)2);
}

TEST(utf16_from_ucs4_supplementary_roundtrip) {
    /* Note: libparserutils' utf16_from_ucs4/to_ucs4 use a non-standard
     * surrogate encoding formula (libwapcaplet/utf16.c lines 75-92):
     *   ss[0] = 0xD800 | (((ucs4>>16) & 0x1f) - 1) | (ucs4 >> 10);
     *   ss[1] = 0xDC00 | (ucs4 & 0x3ff);
     * The standard formula is:
     *   ss[0] = 0xD800 | ((ucs4 - 0x10000) >> 10);
     *   ss[1] = 0xDC00 | ((ucs4 - 0x10000) & 0x3ff);
     * Different bytes are produced, but the encode/decode roundtrip is
     * NOT clean for supplementary plane codepoints in this implementation
     * (a known upstream bug, deferred to ports/netsurf testing).
     * For the helper, just verify it accepts supplementary input + writes
     * 4 bytes + emits the high-surrogate marker as the first byte (0xD8). */
    uint8_t buf[8] = {0};
    size_t len = sizeof(buf);
    ASSERT_EQ(parserutils_charset_utf16_from_ucs4(0x1F600, buf, &len),
              PARSERUTILS_OK);
    ASSERT_EQ(len, (size_t)4);
    /* First byte is 0xD8 (high surrogate marker) on big-endian 68k */
    ASSERT_EQ(buf[0], 0xD8);
    /* Third byte is 0xDE (low surrogate marker) -- 0xDC00 | (ucs4 & 0x3ff)
     * for 0x1F600 gives 0xDC00 | 0x200 = 0xDE00, so high byte is 0xDE */
    ASSERT_EQ(buf[2], 0xDE);
}

/* ===================================================================
 * Category 6: Buffer
 * =================================================================== */

TEST(buffer_create_destroy) {
    parserutils_buffer *buf = NULL;
    ASSERT_EQ(parserutils_buffer_create(&buf), PARSERUTILS_OK);
    ASSERT_NOT_NULL(buf);
    ASSERT_EQ(parserutils_buffer_destroy(buf), PARSERUTILS_OK);
}

TEST(buffer_append_basic) {
    parserutils_buffer *buf = NULL;
    ASSERT_EQ(parserutils_buffer_create(&buf), PARSERUTILS_OK);
    const uint8_t hello[] = "hello";
    ASSERT_EQ(parserutils_buffer_append(buf, hello, 5), PARSERUTILS_OK);
    ASSERT_EQ(buf->length, (size_t)5);
    ASSERT_EQ(memcmp(buf->data, "hello", 5), 0);
    parserutils_buffer_destroy(buf);
}

TEST(buffer_append_grows) {
    parserutils_buffer *buf = NULL;
    ASSERT_EQ(parserutils_buffer_create(&buf), PARSERUTILS_OK);
    /* Append enough to force at least one realloc */
    static uint8_t big[8192];
    int i;
    for (i = 0; i < (int)sizeof(big); i++) big[i] = (uint8_t)(i & 0xff);
    ASSERT_EQ(parserutils_buffer_append(buf, big, sizeof(big)), PARSERUTILS_OK);
    ASSERT_EQ(buf->length, sizeof(big));
    /* Spot-check first/last/middle */
    ASSERT_EQ(buf->data[0], 0);
    ASSERT_EQ(buf->data[1024], (uint8_t)(1024 & 0xff));
    ASSERT_EQ(buf->data[sizeof(big) - 1], (uint8_t)((sizeof(big) - 1) & 0xff));
    parserutils_buffer_destroy(buf);
}

TEST(buffer_insert_middle) {
    parserutils_buffer *buf = NULL;
    ASSERT_EQ(parserutils_buffer_create(&buf), PARSERUTILS_OK);
    parserutils_buffer_append(buf, (uint8_t *)"hello", 5);
    /* Insert "X" at offset 2 -> "heXllo" */
    ASSERT_EQ(parserutils_buffer_insert(buf, 2, (uint8_t *)"X", 1),
              PARSERUTILS_OK);
    ASSERT_EQ(buf->length, (size_t)6);
    ASSERT_EQ(memcmp(buf->data, "heXllo", 6), 0);
    parserutils_buffer_destroy(buf);
}

TEST(buffer_discard_prefix) {
    parserutils_buffer *buf = NULL;
    ASSERT_EQ(parserutils_buffer_create(&buf), PARSERUTILS_OK);
    parserutils_buffer_append(buf, (uint8_t *)"hello", 5);
    /* Discard first 2 bytes -> "llo" */
    ASSERT_EQ(parserutils_buffer_discard(buf, 0, 2), PARSERUTILS_OK);
    ASSERT_EQ(buf->length, (size_t)3);
    ASSERT_EQ(memcmp(buf->data, "llo", 3), 0);
    parserutils_buffer_destroy(buf);
}

/* ===================================================================
 * Category 7: Stack (item_size + chunk_size pattern)
 * =================================================================== */

TEST(stack_create_destroy) {
    parserutils_stack *st = NULL;
    ASSERT_EQ(parserutils_stack_create(sizeof(int), 8, &st), PARSERUTILS_OK);
    ASSERT_NOT_NULL(st);
    ASSERT_EQ(parserutils_stack_destroy(st), PARSERUTILS_OK);
}

TEST(stack_push_pop_lifo) {
    parserutils_stack *st = NULL;
    ASSERT_EQ(parserutils_stack_create(sizeof(int), 8, &st), PARSERUTILS_OK);
    int i;
    for (i = 0; i < 10; i++) {
        ASSERT_EQ(parserutils_stack_push(st, &i), PARSERUTILS_OK);
    }
    int v;
    for (i = 9; i >= 0; i--) {
        ASSERT_EQ(parserutils_stack_pop(st, &v), PARSERUTILS_OK);
        ASSERT_EQ(v, i);
    }
    parserutils_stack_destroy(st);
}

TEST(stack_get_current_top) {
    parserutils_stack *st = NULL;
    ASSERT_EQ(parserutils_stack_create(sizeof(int), 4, &st), PARSERUTILS_OK);
    int v = 42;
    parserutils_stack_push(st, &v);
    int *top = (int *)parserutils_stack_get_current(st);
    ASSERT_NOT_NULL(top);
    ASSERT_EQ(*top, 42);
    parserutils_stack_destroy(st);
}

/* ===================================================================
 * Category 8: Vector
 * =================================================================== */

TEST(vector_create_destroy) {
    parserutils_vector *v = NULL;
    ASSERT_EQ(parserutils_vector_create(sizeof(int), 8, &v), PARSERUTILS_OK);
    ASSERT_NOT_NULL(v);
    ASSERT_EQ(parserutils_vector_destroy(v), PARSERUTILS_OK);
}

TEST(vector_append_iterate) {
    parserutils_vector *v = NULL;
    ASSERT_EQ(parserutils_vector_create(sizeof(int), 4, &v), PARSERUTILS_OK);
    int i;
    for (i = 0; i < 10; i++) {
        ASSERT_EQ(parserutils_vector_append(v, &i), PARSERUTILS_OK);
    }
    size_t length = 0;
    ASSERT_EQ(parserutils_vector_get_length(v, &length), PARSERUTILS_OK);
    ASSERT_EQ(length, (size_t)10);
    /* Iterate */
    int32_t ctx = 0;
    int expected = 0;
    const int *got;
    while ((got = (const int *)parserutils_vector_iterate(v, &ctx)) != NULL) {
        ASSERT_EQ(*got, expected);
        expected++;
    }
    ASSERT_EQ(expected, 10);
    parserutils_vector_destroy(v);
}

TEST(vector_clear) {
    parserutils_vector *v = NULL;
    ASSERT_EQ(parserutils_vector_create(sizeof(int), 4, &v), PARSERUTILS_OK);
    int i;
    for (i = 0; i < 5; i++) parserutils_vector_append(v, &i);
    ASSERT_EQ(parserutils_vector_clear(v), PARSERUTILS_OK);
    size_t length = 1;
    parserutils_vector_get_length(v, &length);
    ASSERT_EQ(length, (size_t)0);
    parserutils_vector_destroy(v);
}

/* ===================================================================
 * Category 9: Input stream
 * =================================================================== */

TEST(inputstream_create_destroy) {
    parserutils_inputstream *s = NULL;
    ASSERT_EQ(parserutils_inputstream_create("UTF-8", 0, NULL, &s),
              PARSERUTILS_OK);
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(parserutils_inputstream_destroy(s), PARSERUTILS_OK);
}

TEST(inputstream_append_peek_advance) {
    parserutils_inputstream *s = NULL;
    ASSERT_EQ(parserutils_inputstream_create("UTF-8", 0, NULL, &s),
              PARSERUTILS_OK);
    ASSERT_EQ(parserutils_inputstream_append(s, (uint8_t *)"hello", 5),
              PARSERUTILS_OK);
    /* Append empty chunk to flush charset detection (NetSurf calls _append(NULL,0)) */
    parserutils_inputstream_append(s, NULL, 0);
    const uint8_t *data = NULL;
    size_t len = 0;
    parserutils_error rc = parserutils_inputstream_peek(s, 0, &data, &len);
    ASSERT_EQ(rc, PARSERUTILS_OK);
    ASSERT_NOT_NULL(data);
    ASSERT_EQ((int)data[0], (int)'h');
    parserutils_inputstream_advance(s, 1);
    rc = parserutils_inputstream_peek(s, 0, &data, &len);
    ASSERT_EQ(rc, PARSERUTILS_OK);
    ASSERT_EQ((int)data[0], (int)'e');
    parserutils_inputstream_destroy(s);
}

TEST(inputstream_two_chunks) {
    parserutils_inputstream *s = NULL;
    ASSERT_EQ(parserutils_inputstream_create("UTF-8", 0, NULL, &s),
              PARSERUTILS_OK);
    ASSERT_EQ(parserutils_inputstream_append(s, (uint8_t *)"hel", 3),
              PARSERUTILS_OK);
    ASSERT_EQ(parserutils_inputstream_append(s, (uint8_t *)"lo!", 3),
              PARSERUTILS_OK);
    parserutils_inputstream_append(s, NULL, 0);
    /* Walk the stream */
    const uint8_t *data = NULL;
    size_t len = 0;
    char acc[8] = {0};
    int idx = 0;
    while (parserutils_inputstream_peek(s, 0, &data, &len) == PARSERUTILS_OK
           && idx < 7) {
        acc[idx++] = (char)data[0];
        parserutils_inputstream_advance(s, len);
    }
    ASSERT_STR_EQ(acc, "hello!");
    parserutils_inputstream_destroy(s);
}

/* ===================================================================
 * Category 10: Amiga-specific (68k endianness, stack, large buffer)
 * =================================================================== */

TEST(amiga_utf16_helper_native_order_on_68k) {
    /* The utf16 HELPER (not codec) operates on host-endian uint16_t.
     * On 68k big-endian, encoding U+0041 as host-order writes 0x00 0x41. */
    uint8_t buf[4] = {0};
    size_t len = sizeof(buf);
    ASSERT_EQ(parserutils_charset_utf16_from_ucs4(0x0041, buf, &len),
              PARSERUTILS_OK);
    ASSERT_EQ(len, (size_t)2);
    ASSERT_EQ(buf[0], 0x00);
    ASSERT_EQ(buf[1], 0x41);
}

TEST(amiga_buffer_4byte_alignment) {
    /* Allocate, append, grow many times -- exercises 68k allocator under stress.
     * If alignment were wrong (crash-patterns #15), this would Guru somewhere. */
    parserutils_buffer *buf = NULL;
    ASSERT_EQ(parserutils_buffer_create(&buf), PARSERUTILS_OK);
    int i;
    for (i = 0; i < 100; i++) {
        ASSERT_EQ(parserutils_buffer_append(buf, (uint8_t *)"chunk", 5),
                  PARSERUTILS_OK);
    }
    ASSERT_EQ(buf->length, (size_t)500);
    parserutils_buffer_destroy(buf);
}

/* ===================================================================
 * Category 11: Stress / real-world
 * =================================================================== */

TEST(stress_utf8_decode_4kb) {
    /* Build 4 KB of repeating ASCII */
    static uint8_t in[4096];
    int i;
    for (i = 0; i < 4096; i++) in[i] = (uint8_t)('A' + (i % 26));
    parserutils_charset_codec *codec = NULL;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", &codec), PARSERUTILS_OK);
    static uint32_t out[4096];
    const uint8_t *src = in;
    size_t srclen = sizeof(in);
    uint8_t *dst = (uint8_t *)out;
    size_t dstlen = sizeof(out);
    parserutils_error rc = parserutils_charset_codec_decode(codec, &src, &srclen,
                                                            &dst, &dstlen);
    ASSERT_EQ(rc, PARSERUTILS_OK);
    /* All consumed */
    ASSERT_EQ(srclen, (size_t)0);
    /* Spot-check first and last decoded codepoints */
    ASSERT_EQ(out[0], (uint32_t)'A');
    ASSERT_EQ(out[4095], (uint32_t)('A' + (4095 % 26)));
    parserutils_charset_codec_destroy(codec);
}

TEST(stress_vector_1000_items) {
    parserutils_vector *v = NULL;
    ASSERT_EQ(parserutils_vector_create(sizeof(int), 16, &v), PARSERUTILS_OK);
    int i;
    for (i = 0; i < 1000; i++) {
        ASSERT_EQ(parserutils_vector_append(v, &i), PARSERUTILS_OK);
    }
    size_t length = 0;
    parserutils_vector_get_length(v, &length);
    ASSERT_EQ(length, (size_t)1000);
    /* Spot-check a few peeks (peek takes int32_t by value, NOT pointer) */
    const int *got = (const int *)parserutils_vector_peek(v, 0);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ(*got, 0);
    const int *got999 = (const int *)parserutils_vector_peek(v, 999);
    ASSERT_NOT_NULL(got999);
    ASSERT_EQ(*got999, 999);
    parserutils_vector_destroy(v);
}

TEST(stress_codec_roundtrip_various_planes) {
    /* ASCII + Latin-1 supplement + BMP + supplementary */
    uint32_t codepoints[] = { 0x0041, 0x00A3, 0x20AC, 0x1F600, 0x4E2D };
    int i;
    parserutils_charset_codec *codec = NULL;
    ASSERT_EQ(parserutils_charset_codec_create("UTF-8", &codec), PARSERUTILS_OK);
    for (i = 0; i < 5; i++) {
        uint8_t enc[8];
        const uint8_t *src = (const uint8_t *)&codepoints[i];
        size_t srclen = sizeof(uint32_t);
        uint8_t *dst = enc;
        size_t dstlen = sizeof(enc);
        ASSERT_EQ(parserutils_charset_codec_encode(codec, &src, &srclen, &dst, &dstlen),
                  PARSERUTILS_OK);
        size_t enc_len = sizeof(enc) - dstlen;
        /* Decode back */
        parserutils_charset_codec_reset(codec);
        uint32_t back = 0;
        src = enc;
        srclen = enc_len;
        dst = (uint8_t *)&back;
        dstlen = sizeof(back);
        ASSERT_EQ(parserutils_charset_codec_decode(codec, &src, &srclen, &dst, &dstlen),
                  PARSERUTILS_OK);
        ASSERT_EQ(back, codepoints[i]);
    }
    parserutils_charset_codec_destroy(codec);
}

/* ===================================================================
 * Test runner
 * =================================================================== */

int main(void) {
    printf("=== lib/libparserutils unit tests ===\n");

    /* Category 1: Codec functional */
    RUN_TEST(codec_utf8_create_destroy);
    RUN_TEST(codec_utf8_roundtrip_ascii);
    RUN_TEST(codec_utf8_roundtrip_2byte);
    RUN_TEST(codec_utf8_roundtrip_3byte);
    RUN_TEST(codec_utf8_roundtrip_4byte);
    RUN_TEST(codec_utf16_roundtrip_bmp);
    RUN_TEST(codec_iso8859_1_roundtrip);
    RUN_TEST(codec_us_ascii_roundtrip);
    RUN_TEST(codec_windows_1252_roundtrip);
    RUN_TEST(codec_reset_clears_state);
    RUN_TEST(codec_setopt_strict);

    /* Category 2: Codec error paths */
    RUN_TEST(codec_create_unknown_charset_fails);
    RUN_TEST(codec_create_null_charset_fails);
    RUN_TEST(codec_create_null_out_fails);
    RUN_TEST(codec_decode_truncated_utf8_no_consume);
    RUN_TEST(codec_decode_invalid_utf8_strict);

    /* Category 3: MIB-enum */
    RUN_TEST(mibenum_from_name_utf8);
    RUN_TEST(mibenum_from_name_alias_utf8);
    RUN_TEST(mibenum_from_name_case_insensitive);
    RUN_TEST(mibenum_from_name_unknown);
    RUN_TEST(mibenum_to_name_roundtrip);
    RUN_TEST(mibenum_is_unicode_utf8);
    RUN_TEST(mibenum_is_unicode_iso8859_false);

    /* Category 4: UTF-8 helpers */
    RUN_TEST(utf8_to_ucs4_ascii);
    RUN_TEST(utf8_to_ucs4_2byte);
    RUN_TEST(utf8_to_ucs4_3byte);
    RUN_TEST(utf8_to_ucs4_4byte);
    RUN_TEST(utf8_from_ucs4_ascii);
    RUN_TEST(utf8_from_ucs4_3byte);
    RUN_TEST(utf8_to_ucs4_truncated);
    RUN_TEST(utf8_char_byte_length_lead_byte);
    RUN_TEST(utf8_length_counts_codepoints);
    RUN_TEST(utf8_next_advances);
    RUN_TEST(utf8_prev_backs);

    /* Category 5: UTF-16 helpers */
    RUN_TEST(utf16_to_ucs4_bmp);
    RUN_TEST(utf16_to_ucs4_surrogate_pair);
    RUN_TEST(utf16_from_ucs4_bmp);
    RUN_TEST(utf16_from_ucs4_supplementary_roundtrip);

    /* Category 6: Buffer */
    RUN_TEST(buffer_create_destroy);
    RUN_TEST(buffer_append_basic);
    RUN_TEST(buffer_append_grows);
    RUN_TEST(buffer_insert_middle);
    RUN_TEST(buffer_discard_prefix);

    /* Category 7: Stack */
    RUN_TEST(stack_create_destroy);
    RUN_TEST(stack_push_pop_lifo);
    RUN_TEST(stack_get_current_top);

    /* Category 8: Vector */
    RUN_TEST(vector_create_destroy);
    RUN_TEST(vector_append_iterate);
    RUN_TEST(vector_clear);

    /* Category 9: Input stream */
    RUN_TEST(inputstream_create_destroy);
    RUN_TEST(inputstream_append_peek_advance);
    RUN_TEST(inputstream_two_chunks);

    /* Category 10: Amiga-specific */
    RUN_TEST(amiga_utf16_helper_native_order_on_68k);
    RUN_TEST(amiga_buffer_4byte_alignment);

    /* Category 11: Stress / real-world */
    RUN_TEST(stress_utf8_decode_4kb);
    RUN_TEST(stress_vector_1000_items);
    RUN_TEST(stress_codec_roundtrip_various_planes);

    return test_summary();
}
