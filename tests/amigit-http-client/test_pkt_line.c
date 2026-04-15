/*
 * test_pkt_line.c -- vamos unit tests for amigit's pkt-line framing.
 *
 * PDR-012 Phase 4. Exercises pkt_line_encode / pkt_line_decode /
 * encode_flush / encode_delim over in-memory buffers. No I/O, no
 * allocation.
 *
 * Large stress buffers (65516 bytes) are declared static so they
 * don't blow the AmigaOS stack per known-pitfalls "Large Local
 * Buffers Cause Guru".
 */

#include "test_framework.h"
#include "pkt_line.h"

#include <string.h>
#include <stdlib.h>

long __stack = 262144;

/* ============================================================
 * Encode -- happy path
 * ============================================================ */

TEST(encode_data_small)
{
    char buf[16];
    int n;

    n = pkt_line_encode(buf, (int)sizeof(buf), "abcd", 4);
    ASSERT_EQ(n, 8);
    ASSERT(memcmp(buf, "0008abcd", 8) == 0);
}

TEST(encode_data_zero_payload)
{
    char buf[8];
    int n;

    n = pkt_line_encode(buf, (int)sizeof(buf), "", 0);
    ASSERT_EQ(n, 4);
    ASSERT(memcmp(buf, "0004", 4) == 0);
}

TEST(encode_data_one_byte)
{
    char buf[8];
    int n;

    n = pkt_line_encode(buf, (int)sizeof(buf), "X", 1);
    ASSERT_EQ(n, 5);
    ASSERT(memcmp(buf, "0005X", 5) == 0);
}

TEST(encode_flush)
{
    char buf[8];
    int n;

    memset(buf, 0xAA, sizeof(buf));
    n = pkt_line_encode_flush(buf, (int)sizeof(buf));
    ASSERT_EQ(n, 4);
    ASSERT(memcmp(buf, "0000", 4) == 0);
}

TEST(encode_delim)
{
    char buf[8];
    int n;

    memset(buf, 0xAA, sizeof(buf));
    n = pkt_line_encode_delim(buf, (int)sizeof(buf));
    ASSERT_EQ(n, 4);
    ASSERT(memcmp(buf, "0001", 4) == 0);
}

TEST(encode_payload_with_embedded_nul)
{
    const char payload[6] = { 'a', 'b', '\0', 'c', 'd', '\0' };
    char buf[16];
    int n;

    n = pkt_line_encode(buf, (int)sizeof(buf), payload, 6);
    ASSERT_EQ(n, 10);
    ASSERT(memcmp(buf, "000a", 4) == 0);
    ASSERT(memcmp(buf + 4, payload, 6) == 0);
}

/* Large static buffers used by max-payload and roundtrip tests. */
static char big_payload[PKT_LINE_MAX_PAYLOAD];
static char big_wire[PKT_LINE_MAX_PAYLOAD + PKT_LINE_HEADER_LEN];
static char big_back[PKT_LINE_MAX_PAYLOAD];

TEST(encode_max_payload)
{
    int n;

    memset(big_payload, 'A', sizeof(big_payload));
    n = pkt_line_encode(big_wire, (int)sizeof(big_wire),
                        big_payload, PKT_LINE_MAX_PAYLOAD);
    ASSERT_EQ(n, PKT_LINE_MAX_PAYLOAD + PKT_LINE_HEADER_LEN);
    /* Lowercase hex per libgit2 convention. */
    ASSERT(memcmp(big_wire, "fff0", 4) == 0);
    ASSERT(big_wire[4] == 'A');
    ASSERT(big_wire[PKT_LINE_HEADER_LEN + PKT_LINE_MAX_PAYLOAD - 1] == 'A');
}

/* ============================================================
 * Encode -- error path
 * ============================================================ */

TEST(encode_err_too_long_payload)
{
    static char over[PKT_LINE_MAX_PAYLOAD + 1];
    static char out[PKT_LINE_MAX_PAYLOAD + 1 + PKT_LINE_HEADER_LEN];

    memset(over, 'B', sizeof(over));
    ASSERT_EQ(pkt_line_encode(out, (int)sizeof(out),
                              over, PKT_LINE_MAX_PAYLOAD + 1),
              PKT_ERR_TOO_LONG);
}

TEST(encode_err_buf_too_small)
{
    char buf6[6];
    char buf8[8];

    /* payload 4 needs 8, buf6 is too small. */
    ASSERT_EQ(pkt_line_encode(buf6, 6, "abcd", 4), PKT_ERR_TOO_LONG);
    /* boundary: buf exactly 8 succeeds. */
    ASSERT_EQ(pkt_line_encode(buf8, 8, "abcd", 4), 8);
    ASSERT(memcmp(buf8, "0008abcd", 8) == 0);
}

TEST(encode_err_inval_null_payload)
{
    char buf[16];

    /* NULL payload with positive length rejected. */
    ASSERT_EQ(pkt_line_encode(buf, (int)sizeof(buf), NULL, 4),
              PKT_ERR_INVAL);
    /* NULL payload with zero length is legal (empty data packet). */
    ASSERT_EQ(pkt_line_encode(buf, (int)sizeof(buf), NULL, 0), 4);
    ASSERT(memcmp(buf, "0004", 4) == 0);
}

TEST(encode_err_negative_payload_len)
{
    char buf[16];

    ASSERT_EQ(pkt_line_encode(buf, (int)sizeof(buf), "abcd", -1),
              PKT_ERR_INVAL);
}

/* ============================================================
 * Decode -- happy path
 * ============================================================ */

TEST(decode_data_packet)
{
    static const char wire[] = "0008abcd";
    const char *payload;
    int payload_len, kind;
    int n;

    n = pkt_line_decode(wire, 8, &payload, &payload_len, &kind);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(kind, PKT_KIND_DATA);
    ASSERT_EQ(payload_len, 4);
    ASSERT(memcmp(payload, "abcd", 4) == 0);
    /* Zero-copy: pointer lands 4 bytes into the wire. */
    ASSERT(payload == wire + 4);
}

TEST(decode_flush_packet)
{
    const char *payload;
    int payload_len = -1;
    int kind = -1;
    int n;

    n = pkt_line_decode("0000", 4, &payload, &payload_len, &kind);
    ASSERT_EQ(n, 4);
    ASSERT_EQ(kind, PKT_KIND_FLUSH);
    ASSERT_EQ(payload_len, 0);
}

TEST(decode_delim_packet)
{
    const char *payload;
    int payload_len = -1;
    int kind = -1;
    int n;

    n = pkt_line_decode("0001", 4, &payload, &payload_len, &kind);
    ASSERT_EQ(n, 4);
    ASSERT_EQ(kind, PKT_KIND_DELIM);
    ASSERT_EQ(payload_len, 0);
}

TEST(decode_zero_byte_data_packet)
{
    const char *payload;
    int payload_len = -1;
    int kind = -1;
    int n;

    /* "0004" is a DATA packet with zero payload bytes -- NOT a flush. */
    n = pkt_line_decode("0004", 4, &payload, &payload_len, &kind);
    ASSERT_EQ(n, 4);
    ASSERT_EQ(kind, PKT_KIND_DATA);
    ASSERT_EQ(payload_len, 0);
}

TEST(decode_packet_with_nul_in_payload)
{
    static const char wire[10] = {
        '0', '0', '0', 'a',
        'a', 'b', '\0', 'c', 'd', '\0'
    };
    const char *payload;
    int payload_len = -1;
    int kind = -1;
    int n;

    n = pkt_line_decode(wire, 10, &payload, &payload_len, &kind);
    ASSERT_EQ(n, 10);
    ASSERT_EQ(kind, PKT_KIND_DATA);
    ASSERT_EQ(payload_len, 6);
    ASSERT(memcmp(payload, wire + 4, 6) == 0);
}

TEST(decode_libgit2_ref_advertisement_fragment)
{
    /* "0032want " (9) + 40-char sha1 hex (40) + "\n" (1) = 50 bytes.
     * Header field 0x32 = 50 decimal. Matches libgit2 smart_pkt.c. */
    static const char wire[] =
        "0032want 1111222233334444555566667777888899990000\n";
    const char *payload;
    int payload_len = -1;
    int kind = -1;
    int n;

    ASSERT_EQ((int)(sizeof(wire) - 1), 50);
    n = pkt_line_decode(wire, 50, &payload, &payload_len, &kind);
    ASSERT_EQ(n, 50);
    ASSERT_EQ(kind, PKT_KIND_DATA);
    ASSERT_EQ(payload_len, 46);
    ASSERT(memcmp(payload,
        "want 1111222233334444555566667777888899990000\n", 46) == 0);
}

TEST(decode_accepts_uppercase_hex)
{
    /* Spec allows either case on decode. Use "00FF" (255 == 0xff). */
    static const char wire[255] = {
        '0', '0', 'F', 'F',
        /* 251 padding bytes */ 0
    };
    const char *payload;
    int payload_len = -1;
    int kind = -1;
    int n;

    n = pkt_line_decode(wire, 255, &payload, &payload_len, &kind);
    ASSERT_EQ(n, 255);
    ASSERT_EQ(kind, PKT_KIND_DATA);
    ASSERT_EQ(payload_len, 251);
}

/* ============================================================
 * Decode -- error path
 * ============================================================ */

TEST(decode_err_truncated_header)
{
    const char *payload;
    int payload_len, kind;

    ASSERT_EQ(pkt_line_decode("003", 3, &payload, &payload_len, &kind),
              PKT_ERR_TRUNCATED);
    ASSERT_EQ(pkt_line_decode("", 0, &payload, &payload_len, &kind),
              PKT_ERR_TRUNCATED);
}

TEST(decode_err_truncated_payload)
{
    const char *payload;
    int payload_len, kind;

    /* Declared 8 bytes, only 6 available. */
    ASSERT_EQ(pkt_line_decode("0008ab", 6, &payload, &payload_len, &kind),
              PKT_ERR_TRUNCATED);
}

TEST(decode_err_invalid_hex)
{
    const char *payload;
    int payload_len, kind;

    ASSERT_EQ(pkt_line_decode("00G8abcd", 8, &payload, &payload_len, &kind),
              PKT_ERR_INVALID);
    ASSERT_EQ(pkt_line_decode("ZZZZ", 4, &payload, &payload_len, &kind),
              PKT_ERR_INVALID);
}

TEST(decode_err_reserved_lengths)
{
    const char *payload;
    int payload_len, kind;

    /* 0002 and 0003 are ill-formed; 0000 and 0001 are legal specials. */
    ASSERT_EQ(pkt_line_decode("0002", 4, &payload, &payload_len, &kind),
              PKT_ERR_INVALID);
    ASSERT_EQ(pkt_line_decode("0003", 4, &payload, &payload_len, &kind),
              PKT_ERR_INVALID);
}

TEST(decode_err_overlength_prefix)
{
    const char *payload;
    int payload_len, kind;
    /* fff1 = 65521, one over PKT_LINE_MAX_PAYLOAD + header. */
    static const char wire[] = "fff1....";

    ASSERT_EQ(pkt_line_decode(wire, 8, &payload, &payload_len, &kind),
              PKT_ERR_INVALID);
}

TEST(decode_err_null_out_args)
{
    const char *payload;
    int payload_len, kind;

    ASSERT_EQ(pkt_line_decode(NULL, 4, &payload, &payload_len, &kind),
              PKT_ERR_INVAL);
    ASSERT_EQ(pkt_line_decode("0000", 4, NULL, &payload_len, &kind),
              PKT_ERR_INVAL);
    ASSERT_EQ(pkt_line_decode("0000", 4, &payload, NULL, &kind),
              PKT_ERR_INVAL);
    ASSERT_EQ(pkt_line_decode("0000", 4, &payload, &payload_len, NULL),
              PKT_ERR_INVAL);
}

/* ============================================================
 * Stress
 * ============================================================ */

TEST(encode_decode_roundtrip_max)
{
    const char *payload;
    int payload_len = -1;
    int kind = -1;
    int i;
    int n;

    for (i = 0; i < PKT_LINE_MAX_PAYLOAD; i++) {
        big_payload[i] = (char)(i & 0x7F);
    }

    n = pkt_line_encode(big_wire, (int)sizeof(big_wire),
                        big_payload, PKT_LINE_MAX_PAYLOAD);
    ASSERT_EQ(n, PKT_LINE_MAX_PAYLOAD + PKT_LINE_HEADER_LEN);

    n = pkt_line_decode(big_wire, PKT_LINE_MAX_PAYLOAD + PKT_LINE_HEADER_LEN,
                        &payload, &payload_len, &kind);
    ASSERT_EQ(n, PKT_LINE_MAX_PAYLOAD + PKT_LINE_HEADER_LEN);
    ASSERT_EQ(kind, PKT_KIND_DATA);
    ASSERT_EQ(payload_len, PKT_LINE_MAX_PAYLOAD);

    memcpy(big_back, payload, (size_t)payload_len);
    ASSERT(memcmp(big_back, big_payload, PKT_LINE_MAX_PAYLOAD) == 0);
}

/* Build a small ref-advertisement-like stream and decode it
 * packet-by-packet. Verifies the caller-advance contract
 * (consumer adds the returned length to its own cursor). */
TEST(decode_ref_ad_stream)
{
    /* 3 "0032want <sha40>\n" refs (50 bytes each) + "0000" flush. */
    static const char stream[] =
        "0032want 1111222233334444555566667777888899990000\n"
        "0032want 2222333344445555666677778888999900001111\n"
        "0032want 3333444455556666777788889999000011112222\n"
        "0000";
    int pos = 0;
    int total = (int)(sizeof(stream) - 1);
    int pkt_count = 0;
    int saw_flush = 0;

    while (pos < total) {
        const char *payload;
        int payload_len, kind;
        int n;

        n = pkt_line_decode(stream + pos, total - pos,
                            &payload, &payload_len, &kind);
        ASSERT(n > 0);

        if (kind == PKT_KIND_FLUSH) {
            saw_flush = 1;
            pos += n;
            break;
        }
        ASSERT_EQ(kind, PKT_KIND_DATA);
        ASSERT_EQ(payload_len, 46);
        ASSERT(memcmp(payload, "want ", 5) == 0);

        pkt_count++;
        pos += n;
    }
    ASSERT_EQ(pkt_count, 3);
    ASSERT_EQ(saw_flush, 1);
    ASSERT_EQ(pos, total);
}

int
main(void)
{
    /* Encode -- happy path */
    RUN_TEST(encode_data_small);
    RUN_TEST(encode_data_zero_payload);
    RUN_TEST(encode_data_one_byte);
    RUN_TEST(encode_flush);
    RUN_TEST(encode_delim);
    RUN_TEST(encode_payload_with_embedded_nul);
    RUN_TEST(encode_max_payload);

    /* Encode -- error path */
    RUN_TEST(encode_err_too_long_payload);
    RUN_TEST(encode_err_buf_too_small);
    RUN_TEST(encode_err_inval_null_payload);
    RUN_TEST(encode_err_negative_payload_len);

    /* Decode -- happy path */
    RUN_TEST(decode_data_packet);
    RUN_TEST(decode_flush_packet);
    RUN_TEST(decode_delim_packet);
    RUN_TEST(decode_zero_byte_data_packet);
    RUN_TEST(decode_packet_with_nul_in_payload);
    RUN_TEST(decode_libgit2_ref_advertisement_fragment);
    RUN_TEST(decode_accepts_uppercase_hex);

    /* Decode -- error path */
    RUN_TEST(decode_err_truncated_header);
    RUN_TEST(decode_err_truncated_payload);
    RUN_TEST(decode_err_invalid_hex);
    RUN_TEST(decode_err_reserved_lengths);
    RUN_TEST(decode_err_overlength_prefix);
    RUN_TEST(decode_err_null_out_args);

    /* Stress */
    RUN_TEST(encode_decode_roundtrip_max);
    RUN_TEST(decode_ref_ad_stream);

    return test_summary();
}
