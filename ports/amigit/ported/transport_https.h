/*
 * transport_https.h -- amigit custom smart-HTTP(S) transport registration
 *
 * PDR-012 Phase 2: registers a git_smart_subtransport_definition for the
 * "https://" URL scheme so that libgit2 routes remote operations on
 * HTTPS URLs to amigit's backend (rather than the transport_stubs.c
 * dummy that returns GIT_ERROR unconditionally).
 *
 * At Phase 2 the action handler returns "not implemented" for every
 * service verb. Phase 3+ will flesh it out with real HTTP/1.1, AmiSSL,
 * and service discovery.
 *
 * Rationale for Path B (custom subtransport rather than upstream
 * transports/http.c): see docs/pdr/012-amigit-https-networking.md.
 */

#ifndef AMIGIT_TRANSPORT_HTTPS_H
#define AMIGIT_TRANSPORT_HTTPS_H

/*
 * Register the amigit HTTPS subtransport with libgit2.
 *
 * Call exactly once, AFTER git_libgit2_init() and BEFORE any
 * git_remote_lookup / git_remote_create_detached that might be asked
 * to connect to an https:// URL.
 *
 * Returns 0 on success, a libgit2 negative error code on failure.
 */
int amigit_transport_https_register(void);

/*
 * Unregister the amigit HTTPS subtransport. Safe to call from an
 * atexit hook; a second call is a no-op.
 */
void amigit_transport_https_cleanup(void);

/*
 * PDR-012 Phase 6 debug entry point: drive the upload-pack POST
 * path directly (bypassing libgit2's smart transport dispatch) so
 * `amigit _https-probe --pack <url>` can manually exercise the
 * helper that action(GIT_SERVICE_UPLOADPACK) uses for real fetches.
 *
 * `url` must be an https://host[:port]/basepath URL. The helper
 * internally appends the "/git-upload-pack" path suffix and POSTs
 * the supplied body bytes with the standard x-git-upload-pack-
 * request Content-Type. A minimal body of "0000" (a single pkt-line
 * flush frame) is sufficient to exercise the transport -- most
 * servers will reject it at the protocol level with 400/500, which
 * is still a success signal for this probe: it proves the HTTPS +
 * POST flow reached the origin server.
 *
 * Prints progress / status / errbuf to stdout. Returns 0 if the
 * POST completed with a 200 status and the response body drained
 * cleanly; returns -1 on any transport failure OR non-2xx status.
 *
 * This function goes away post-Phase 7 along with the rest of
 * _https-probe.
 */
int amigit_transport_https_debug_post(const char *url,
                                      const char *body,
                                      int         body_len);

#endif /* AMIGIT_TRANSPORT_HTTPS_H */
