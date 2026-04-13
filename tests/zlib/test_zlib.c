/*
 * test_zlib.c -- Unit tests for lib/zlib (zlib 1.3.1 on AmigaOS 68k)
 *
 * Coverage:
 *   - adler32 known-value check
 *   - crc32 known-value check
 *   - deflate/inflate round-trip on small buffer
 *   - deflate/inflate round-trip on a larger buffer with repeating patterns
 *   - compress2()/uncompress() one-shot helpers
 *   - gzip wrapper format detection (deflateInit2 windowBits = 31)
 *
 * Not tested here (requires file I/O):
 *   - gzopen/gzread/gzwrite -- tested separately if Category-3 issues emerge
 *
 * Build: see tests/zlib/Makefile. Runs on vamos.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zlib.h"
#include "test_framework.h"

/* Real AmigaOS reads this cookie. vamos ignores it and needs -s on the
 * command line (see Makefile VAMOS_STACK). 256 KB is ample -- the streaming
 * test has ~16 KB of locals per frame. */
long __stack = 262144;

/* RFC 1950 canonical adler32: the algorithm is seeded with adler=1, b=0.
 * zlib's API takes whatever seed you pass, splits it into high/low 16, and
 * accumulates from there. Passing 0 as the seed is well-defined but produces
 * different values than the RFC vectors, so we explicitly call
 * adler32(0, Z_NULL, 0) to get the correct initial seed (which returns 1). */
TEST(adler32_known_values)
{
    uLong a;
    uLong seed;

    /* The initial seed is returned when buf=Z_NULL, per zlib.h. */
    seed = adler32(0L, Z_NULL, 0);
    ASSERT_EQ(seed, 1UL);

    /* adler32 of "Wikipedia" = 0x11E60398 (RFC 1950 canonical value). */
    a = adler32(seed, (const Bytef *)"Wikipedia", 9);
    ASSERT_EQ(a, 0x11E60398UL);

    /* adler32 of "hello" = 0x062C0215 (seed=1). */
    a = adler32(seed, (const Bytef *)"hello", 5);
    ASSERT_EQ(a, 0x062C0215UL);

    /* Also verify the zero-seed behaviour: adler32(0, "Wikipedia", 9) should
     * yield 0x11DD0397, matching what we observed in the prior test run.
     * This nails down the contract and prevents regression if zlib's
     * behaviour ever drifts. */
    a = adler32(0L, (const Bytef *)"Wikipedia", 9);
    ASSERT_EQ(a, 0x11DD0397UL);
}

/* crc32 of "123456789" = 0xCBF43926 per the catalogue of CRC parameters
 * (this is the de-facto test vector for CRC-32/ISO-HDLC). */
TEST(crc32_known_values)
{
    uLong c;

    c = crc32(0L, Z_NULL, 0);
    ASSERT_EQ(c, 0UL);

    c = crc32(0L, (const Bytef *)"123456789", 9);
    ASSERT_EQ(c, 0xCBF43926UL);

    c = crc32(0L, (const Bytef *)"", 0);
    ASSERT_EQ(c, 0UL);
}

TEST(compress_uncompress_roundtrip_small)
{
    const char *src = "The quick brown fox jumps over the lazy dog.";
    uLongf src_len = (uLongf)strlen(src);
    Bytef compressed[256];
    uLongf comp_len = sizeof(compressed);
    Bytef decompressed[256];
    uLongf decomp_len = sizeof(decompressed);
    int rc;

    rc = compress(compressed, &comp_len, (const Bytef *)src, src_len);
    ASSERT_EQ(rc, Z_OK);
    ASSERT(comp_len > 0);
    ASSERT(comp_len <= sizeof(compressed));

    rc = uncompress(decompressed, &decomp_len, compressed, comp_len);
    ASSERT_EQ(rc, Z_OK);
    ASSERT_EQ(decomp_len, src_len);
    ASSERT(memcmp(decompressed, src, src_len) == 0);
}

TEST(compress2_uncompress_roundtrip_highly_compressible)
{
    /* Highly compressible repeating pattern -- exercises the LZ77 match path. */
    Bytef src[1024];
    Bytef compressed[2048];
    uLongf comp_len = sizeof(compressed);
    Bytef decompressed[1024];
    uLongf decomp_len = sizeof(decompressed);
    int rc;
    int i;

    for (i = 0; i < (int)sizeof(src); i++) {
        src[i] = (Bytef)('A' + (i % 26));
    }

    rc = compress2(compressed, &comp_len, src, (uLongf)sizeof(src), Z_BEST_COMPRESSION);
    ASSERT_EQ(rc, Z_OK);
    /* Repeating pattern should compress to far less than the original. */
    ASSERT(comp_len < sizeof(src) / 4);

    rc = uncompress(decompressed, &decomp_len, compressed, comp_len);
    ASSERT_EQ(rc, Z_OK);
    ASSERT_EQ(decomp_len, (uLongf)sizeof(src));
    ASSERT(memcmp(decompressed, src, sizeof(src)) == 0);
}

TEST(deflate_inflate_streaming_4kb)
{
    /* Streaming API (not one-shot compress). Closer to how libgit2 will use it. */
    z_stream defs, infs;
    Bytef src[4096];
    Bytef compressed[8192];
    Bytef decompressed[4096];
    int rc;
    int i;

    /* Semi-random deterministic content (linear congruential). */
    for (i = 0; i < (int)sizeof(src); i++) {
        src[i] = (Bytef)((i * 1103515245UL + 12345UL) >> 16);
    }

    memset(&defs, 0, sizeof(defs));
    rc = deflateInit(&defs, Z_DEFAULT_COMPRESSION);
    ASSERT_EQ(rc, Z_OK);

    defs.next_in = src;
    defs.avail_in = (uInt)sizeof(src);
    defs.next_out = compressed;
    defs.avail_out = (uInt)sizeof(compressed);

    rc = deflate(&defs, Z_FINISH);
    ASSERT_EQ(rc, Z_STREAM_END);

    {
        uLong comp_len = defs.total_out;
        deflateEnd(&defs);
        ASSERT(comp_len > 0);
        ASSERT(comp_len <= sizeof(compressed));

        memset(&infs, 0, sizeof(infs));
        rc = inflateInit(&infs);
        ASSERT_EQ(rc, Z_OK);

        infs.next_in = compressed;
        infs.avail_in = (uInt)comp_len;
        infs.next_out = decompressed;
        infs.avail_out = (uInt)sizeof(decompressed);

        rc = inflate(&infs, Z_FINISH);
        ASSERT_EQ(rc, Z_STREAM_END);
        ASSERT_EQ(infs.total_out, (uLong)sizeof(src));
        inflateEnd(&infs);
    }

    ASSERT(memcmp(decompressed, src, sizeof(src)) == 0);
}

TEST(gzip_wrapper_format_roundtrip)
{
    /* windowBits = 15 + 16 selects gzip wrapper. libgit2 does NOT use
     * gzip (it uses raw zlib), but this exercises the path libgit2's pack
     * index v2 uses for the idx footer which includes a gzip-style CRC. */
    z_stream defs, infs;
    const char *src = "gzip-wrapped test payload for amiport zlib port";
    uLong src_len = (uLong)strlen(src);
    Bytef compressed[256];
    Bytef decompressed[256];
    int rc;

    memset(&defs, 0, sizeof(defs));
    rc = deflateInit2(&defs, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                      15 + 16, 8, Z_DEFAULT_STRATEGY);
    ASSERT_EQ(rc, Z_OK);

    defs.next_in = (Bytef *)src;
    defs.avail_in = (uInt)src_len;
    defs.next_out = compressed;
    defs.avail_out = (uInt)sizeof(compressed);

    rc = deflate(&defs, Z_FINISH);
    ASSERT_EQ(rc, Z_STREAM_END);

    /* gzip header starts with 0x1f 0x8b. */
    ASSERT_EQ((int)compressed[0], 0x1f);
    ASSERT_EQ((int)compressed[1], 0x8b);

    {
        uLong comp_len = defs.total_out;
        deflateEnd(&defs);

        memset(&infs, 0, sizeof(infs));
        rc = inflateInit2(&infs, 15 + 16);
        ASSERT_EQ(rc, Z_OK);

        infs.next_in = compressed;
        infs.avail_in = (uInt)comp_len;
        infs.next_out = decompressed;
        infs.avail_out = (uInt)sizeof(decompressed);

        rc = inflate(&infs, Z_FINISH);
        ASSERT_EQ(rc, Z_STREAM_END);
        ASSERT_EQ(infs.total_out, src_len);
        inflateEnd(&infs);
    }

    ASSERT(memcmp(decompressed, src, src_len) == 0);
}

TEST(version_string_present)
{
    const char *v = zlibVersion();
    ASSERT_NOT_NULL(v);
    ASSERT(strlen(v) > 0);
    /* Sanity: must start with "1." for zlib 1.x. */
    ASSERT_EQ(v[0], '1');
    ASSERT_EQ(v[1], '.');
}

int main(void)
{
    printf("=== lib/zlib (zlib 1.3.1) unit tests ===\n");
    RUN_TEST(adler32_known_values);
    RUN_TEST(crc32_known_values);
    RUN_TEST(compress_uncompress_roundtrip_small);
    RUN_TEST(compress2_uncompress_roundtrip_highly_compressible);
    RUN_TEST(deflate_inflate_streaming_4kb);
    RUN_TEST(gzip_wrapper_format_roundtrip);
    RUN_TEST(version_string_present);
    return test_summary();
}
