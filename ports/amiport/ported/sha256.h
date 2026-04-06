/*
 * sha256.h -- Minimal SHA-256 for download integrity verification
 *
 * Based on Brad Conte's public domain crypto-algorithms.
 * All static state, no malloc.
 *
 * amiport: original code for amiport
 */

#ifndef AMIPORT_SHA256_H
#define AMIPORT_SHA256_H

/* Hash a file. Writes 64-char lowercase hex string to out.
 * out must be >= 65 bytes. Returns 0 on success, -1 on error. */
int sha256_file(const char *path, char *out);

/* Hash a memory buffer. Writes 64-char lowercase hex string to out.
 * out must be >= 65 bytes. */
void sha256_hash(const unsigned char *data, unsigned long len, char *out);

#endif /* AMIPORT_SHA256_H */
