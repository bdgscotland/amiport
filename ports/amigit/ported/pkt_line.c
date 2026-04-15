/*
 * pkt_line.c -- git pkt-line framing for amigit smart-HTTP transport.
 *
 * PDR-012 Phase 4. See pkt_line.h for the API contract.
 *
 * Implementation notes:
 *
 * - The 4-byte length prefix is emitted in LOWERCASE hex. libgit2's
 *   upstream smart_pkt.c uses lowercase; a few older git servers emit
 *   uppercase but the spec allows either on decode. We emit lowercase
 *   to match upstream and accept both on decode.
 *
 * - Decode is zero-copy: *out_payload points into the caller's input
 *   buffer. The caller must keep the buffer alive while using the
 *   payload pointer.
 *
 * - No allocation. No I/O. Every error is reported via a negative
 *   return code; no errno / logging.
 */

#include "pkt_line.h"

#include <string.h>

/* Decode one ASCII hex digit. Returns 0..15 on success, -1 on non-hex. */
static int
hex_digit(int c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

/* Write a 4-digit lowercase hex value into buf[0..3]. Caller has
 * already verified 0 <= value <= 0xFFFF and that buf has room. */
static void
write_hex4(char *buf, unsigned int value)
{
    static const char digits[] = "0123456789abcdef";
    buf[0] = digits[(value >> 12) & 0xF];
    buf[1] = digits[(value >>  8) & 0xF];
    buf[2] = digits[(value >>  4) & 0xF];
    buf[3] = digits[ value        & 0xF];
}

int
pkt_line_encode(char *buf, int max, const void *payload, int payload_len)
{
    int total;

    if (buf == NULL) {
        return PKT_ERR_INVAL;
    }
    if (payload_len < 0) {
        return PKT_ERR_INVAL;
    }
    if (payload == NULL && payload_len > 0) {
        return PKT_ERR_INVAL;
    }
    if (payload_len > PKT_LINE_MAX_PAYLOAD) {
        return PKT_ERR_TOO_LONG;
    }

    total = PKT_LINE_HEADER_LEN + payload_len;
    if (total > max) {
        return PKT_ERR_TOO_LONG;
    }

    write_hex4(buf, (unsigned int)total);
    if (payload_len > 0) {
        memcpy(buf + PKT_LINE_HEADER_LEN, payload, (size_t)payload_len);
    }
    return total;
}

int
pkt_line_encode_flush(char *buf, int max)
{
    if (buf == NULL) {
        return PKT_ERR_INVAL;
    }
    if (max < PKT_LINE_HEADER_LEN) {
        return PKT_ERR_TOO_LONG;
    }
    buf[0] = '0';
    buf[1] = '0';
    buf[2] = '0';
    buf[3] = '0';
    return PKT_LINE_HEADER_LEN;
}

int
pkt_line_encode_delim(char *buf, int max)
{
    if (buf == NULL) {
        return PKT_ERR_INVAL;
    }
    if (max < PKT_LINE_HEADER_LEN) {
        return PKT_ERR_TOO_LONG;
    }
    buf[0] = '0';
    buf[1] = '0';
    buf[2] = '0';
    buf[3] = '1';
    return PKT_LINE_HEADER_LEN;
}

int
pkt_line_decode(const char *buf, int len,
                const char **out_payload, int *out_payload_len,
                int *out_kind)
{
    int d0, d1, d2, d3;
    unsigned int declared;
    int payload_len;

    if (buf == NULL || out_payload == NULL ||
        out_payload_len == NULL || out_kind == NULL) {
        return PKT_ERR_INVAL;
    }

    if (len < PKT_LINE_HEADER_LEN) {
        return PKT_ERR_TRUNCATED;
    }

    d0 = hex_digit((unsigned char)buf[0]);
    d1 = hex_digit((unsigned char)buf[1]);
    d2 = hex_digit((unsigned char)buf[2]);
    d3 = hex_digit((unsigned char)buf[3]);
    if (d0 < 0 || d1 < 0 || d2 < 0 || d3 < 0) {
        return PKT_ERR_INVALID;
    }

    declared = ((unsigned int)d0 << 12) |
               ((unsigned int)d1 <<  8) |
               ((unsigned int)d2 <<  4) |
                (unsigned int)d3;

    /* 0000 flush and 0001 delim are zero-payload specials. */
    if (declared == 0) {
        *out_payload = buf + PKT_LINE_HEADER_LEN;
        *out_payload_len = 0;
        *out_kind = PKT_KIND_FLUSH;
        return PKT_LINE_HEADER_LEN;
    }
    if (declared == 1) {
        *out_payload = buf + PKT_LINE_HEADER_LEN;
        *out_payload_len = 0;
        *out_kind = PKT_KIND_DELIM;
        return PKT_LINE_HEADER_LEN;
    }

    /* 0002 and 0003 are reserved / ill-formed data packets -- there
     * is no valid representation of a packet smaller than the 4-byte
     * header itself. */
    if (declared < PKT_LINE_HEADER_LEN) {
        return PKT_ERR_INVALID;
    }

    /* Upper bound per spec. */
    if (declared > (unsigned int)(PKT_LINE_MAX_PAYLOAD + PKT_LINE_HEADER_LEN)) {
        return PKT_ERR_INVALID;
    }

    if ((unsigned int)len < declared) {
        return PKT_ERR_TRUNCATED;
    }

    payload_len = (int)declared - PKT_LINE_HEADER_LEN;
    *out_payload = buf + PKT_LINE_HEADER_LEN;
    *out_payload_len = payload_len;
    *out_kind = PKT_KIND_DATA;
    return (int)declared;
}
