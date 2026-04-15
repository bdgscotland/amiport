/*
 * pkt_line.h -- git pkt-line framing for amigit smart-HTTP transport.
 *
 * PDR-012 Phase 4. Pure C parser; no I/O, no allocation. All
 * functions work on caller-provided buffers so the same code runs
 * on vamos (unit tests) and real AmigaOS (libgit2 transport).
 *
 * Wire format (protocol-common.txt):
 *
 *   <4-hex-length> <payload bytes>
 *
 * The length field counts itself, so a data packet with N payload
 * bytes encodes to a 4+N byte wire value whose hex prefix reads
 * as N+4. Special values:
 *
 *   "0000" -- flush packet, zero payload, end-of-stream marker
 *   "0001" -- delim packet (protocol v2), zero payload
 *   "0002" -- reserved, rejected by this parser
 *   "0003" -- reserved, rejected by this parser
 *   "0004" -- valid DATA packet with zero payload bytes (NOT flush)
 *   "0005".."fff0" -- valid DATA packet with 1..65516 payload bytes
 *   >"fff0" -- rejected (spec maximum)
 *
 * Phase 5 feeds decoded packets into libgit2's smart-HTTP
 * subtransport. We do NOT re-implement pkt_parse_line -- we only
 * need to produce and consume the wire frames; libgit2 handles the
 * ref advertisement and capability parsing on top of that.
 */
#ifndef AMIGIT_PKT_LINE_H
#define AMIGIT_PKT_LINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum payload bytes in a single pkt-line. 0xFFF0 (65520) is the
 * hard wire limit; subtract the 4-byte header to get the payload
 * budget. */
#define PKT_LINE_MAX_PAYLOAD   65516
#define PKT_LINE_HEADER_LEN    4

/* Packet kinds returned by pkt_line_decode(). */
#define PKT_KIND_DATA          0
#define PKT_KIND_FLUSH         1
#define PKT_KIND_DELIM         2

/* Error codes (negative). */
#define PKT_ERR_TRUNCATED     -1    /* input shorter than declared length */
#define PKT_ERR_INVALID       -2    /* bad hex / reserved / overlength */
#define PKT_ERR_TOO_LONG      -3    /* payload or output exceeds budget */
#define PKT_ERR_INVAL         -4    /* NULL arg with non-zero length */

/*
 * pkt_line_encode -- write a data packet.
 *
 * Emits "NNNN" (lowercase 4-hex prefix) followed by payload_len
 * payload bytes into buf. Returns the number of bytes written
 * (4 + payload_len) on success.
 *
 * Errors:
 *   PKT_ERR_TOO_LONG  if 4 + payload_len > max,
 *                     or if payload_len > PKT_LINE_MAX_PAYLOAD
 *   PKT_ERR_INVAL     if buf == NULL,
 *                     or if payload == NULL && payload_len > 0,
 *                     or if payload_len < 0
 */
int pkt_line_encode(char *buf, int max,
                    const void *payload, int payload_len);

/*
 * pkt_line_encode_flush -- write a flush packet "0000".
 * Returns 4 on success, PKT_ERR_TOO_LONG if max < 4.
 */
int pkt_line_encode_flush(char *buf, int max);

/*
 * pkt_line_encode_delim -- write a delim packet "0001".
 * Returns 4 on success, PKT_ERR_TOO_LONG if max < 4.
 */
int pkt_line_encode_delim(char *buf, int max);

/*
 * pkt_line_decode -- parse one pkt-line from buf[0..len-1].
 *
 * On success, returns the total bytes consumed (the full packet
 * length including the 4-byte header). Output arguments:
 *   *out_payload      -> pointer into buf at offset 4 (zero-copy)
 *                        For flush/delim packets this is set to buf+4
 *                        but out_payload_len will be 0 so the caller
 *                        should not dereference it.
 *   *out_payload_len  -> payload bytes, 0 for flush/delim/empty-data
 *   *out_kind         -> PKT_KIND_DATA / PKT_KIND_FLUSH / PKT_KIND_DELIM
 *
 * Errors:
 *   PKT_ERR_TRUNCATED if len < 4 (header incomplete), or if len <
 *                     the declared packet length (body incomplete).
 *   PKT_ERR_INVALID   if any of the 4 header bytes is not hex,
 *                     or if the declared length is 2 or 3 (reserved),
 *                     or if the declared length > 0xFFF0.
 *   PKT_ERR_INVAL     if buf, out_payload, out_payload_len, or
 *                     out_kind is NULL.
 */
int pkt_line_decode(const char *buf, int len,
                    const char **out_payload, int *out_payload_len,
                    int *out_kind);

#ifdef __cplusplus
}
#endif

#endif /* AMIGIT_PKT_LINE_H */
