/*
 * transport_stubs.c -- stub definitions for excluded transport layer
 *
 * The AmigaOS port (PDR-010) excludes network transports entirely.
 * This file provides the symbol definitions that settings.c and
 * other core files reference from the transport layer.
 *
 * amiport: PDR-010 Phase 2, Stage 3 stub -- documented in PATCHES.md.
 */

/* git_smart__ofs_delta_enabled -- pack negotiation flag.
 * Referenced by settings.c GIT_OPT_ENABLE_OFS_DELTA handler.
 * Default value 1 (enabled) matches upstream default. */
int git_smart__ofs_delta_enabled = 1;

/* git_http__expect_continue -- HTTP Expect: 100-continue flag.
 * Referenced by settings.c GIT_OPT_HTTP_EXPECT_CONTINUE handler. */
int git_http__expect_continue = 0;
