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

#endif /* AMIGIT_TRANSPORT_HTTPS_H */
