/*
 * transports/http.h -- minimal stub for the AmigaOS amiport build.
 *
 * Upstream's http.h pulls in settings.h + httpclient.h and defines the
 * internal request/reply data structures for transports/http.c. We
 * keep http.c pruned (amigit ships its own smart-HTTP subtransport in
 * ports/amigit/ported/transport_https.c) so most of that header is
 * dead weight. The only symbol from it that reaches live code is
 * `git_http__expect_continue`, referenced by `settings.c` for the
 * `GIT_OPT_HTTP_EXPECT_CONTINUE` option handler.
 *
 * The extern is declared with the same `bool` type upstream uses so
 * that the upstream-sourced settings.c we ship links cleanly against
 * our definition in transport_stubs.c.
 *
 * amiport: PDR-010 Phase 2 (original stub) + PDR-012 Phase 1 (type
 * bumped from int to bool to match upstream).
 */
#ifndef INCLUDE_transports_http_h__
#define INCLUDE_transports_http_h__

#include <stdbool.h>

extern bool git_http__expect_continue;

#endif /* INCLUDE_transports_http_h__ */
