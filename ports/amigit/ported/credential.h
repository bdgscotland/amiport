/*
 * credential.h -- HTTP Basic auth credential sourcing for amigit.
 *
 * Phase 7 wires GitHub personal access tokens (or equivalent on
 * other origin servers) into the transport_https.c 401 retry
 * path. The credential subsystem in libgit2 is pruned in the
 * amiport build (see lib/libgit2/src/libgit2/transport_stubs.c);
 * instead of plumbing through git_credential_userpass_plaintext_new
 * and the smart.c credential callback dispatch, amigit owns the
 * 401 retry entirely inside open_request_with_redirects and calls
 * amigit_credential_get directly.
 *
 * This decouples the auth path from libgit2 internals and keeps
 * the binary small. Phase 8 cmd_clone may later add a second
 * pathway that honors libgit2's git_remote_callbacks.credentials
 * if the un-prune cost is worth it; until then, this header is
 * the sole source of auth credentials.
 *
 * amiport: PDR-012 Phase 7 (2026-04-15).
 */

#ifndef AMIGIT_CREDENTIAL_H
#define AMIGIT_CREDENTIAL_H

#include <stddef.h>

/*
 * Fetch HTTP Basic auth credentials used by the 401 retry path.
 *
 * Sources, tried in order:
 *   1. ENV:GIT_HTTP_USERNAME + ENV:GIT_HTTP_TOKEN (AmigaDOS env
 *      variables, read via direct Open/Read rather than GetVar
 *      because libnix getenv and GetVar don't reliably read ENV:
 *      in all process contexts -- notably Execute scripts). The
 *      username defaults to "git" if only the token is set, to
 *      match GitHub's PAT convention.
 *   2. Interactive prompt on stdin/stdout -- only if
 *      IsInteractive(Input()) returns true. Username is read in
 *      cooked mode with echo; token is read in raw mode (no
 *      echo). The ScreenRead newline ends either prompt.
 *
 * On success, writes NUL-terminated strings to user_buf and
 * token_buf and returns 0. On failure (no env vars, non-interactive
 * input, or any read error) returns -1 and populates errbuf with
 * a human-readable message naming both env vars so the user can
 * fix the problem.
 *
 * The caller MUST zero user_buf and token_buf via
 * amigit_credential_zero after use -- the token is a PAT and must
 * not persist on the stack. This function itself zeros any internal
 * scratch buffers on every exit path.
 */
int amigit_credential_get(char *user_buf, size_t user_buf_sz,
                          char *token_buf, size_t token_buf_sz,
                          char *errbuf, size_t errbuf_sz);

/*
 * Base64-encode src_len bytes of src into dst, using the RFC 4648
 * standard alphabet with '=' padding. dst must have room for
 * ((src_len + 2) / 3) * 4 bytes + 1 NUL terminator. Returns the
 * number of chars written (not counting the NUL) on success, or
 * -1 if dst_sz is too small.
 *
 * Used by transport_https.c to build "Basic base64(user:token)"
 * Authorization headers. Pulling in libtomcrypt for this would add
 * a library dependency to amigit purely for ~30 lines of RFC 4648;
 * the inline encoder is cheaper and the binary stays small.
 */
int amigit_base64_encode(const void *src, size_t src_len,
                         char *dst, size_t dst_sz);

/*
 * Zero `len` bytes starting at `buf`. Uses a volatile pointer to
 * prevent the compiler from eliminating the write as dead store.
 * Intended for credential buffers after use -- the stack copy of a
 * PAT should be scrubbed before the function returns, not left
 * visible via a subsequent debugger snapshot or stack scan.
 */
void amigit_credential_zero(void *buf, size_t len);

#endif /* AMIGIT_CREDENTIAL_H */
