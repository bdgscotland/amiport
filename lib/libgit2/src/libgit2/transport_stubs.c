/*
 * transport_stubs.c -- stubs for the transport backends that stay
 * pruned in the AmigaOS build.
 *
 * As of PDR-012 Phase 1 (2026-04-14), the libgit2 port un-prunes
 * clone.c / fetch.c / remote.c / transport.c and the smart transport
 * trio (smart.c, smart_pkt.c, smart_protocol.c). These need
 * `git_smart_subtransport_http` / `_git` / `_ssh` and
 * `git_transport_local` to exist as resolved symbols because upstream
 * `transport.c` references them statically in its dispatch table --
 * even when nothing calls them at runtime.
 *
 * We still keep `transports/http.c`, `transports/git.c`,
 * `transports/ssh_*.c`, `transports/local.c`, and the entire
 * `streams/` subtree pruned. The amigit port registers its own
 * `git_smart_subtransport_definition` for https:// via
 * `git_transport_register` BEFORE any clone/fetch/push is issued, so
 * `transport_find_by_url` returns amigit's backend and the static
 * table is never consulted for the http/https path.
 *
 * Any attempt to use git://, ssh://, ssh+git://, git+ssh://, or
 * file:// falls through to these stubs and returns
 * `GIT_ERROR_NOT_IMPLEMENTED`.
 *
 * Also kept here: `git_http__expect_continue`, a global option
 * referenced by `settings.c`'s `GIT_OPT_HTTP_EXPECT_CONTINUE` handler.
 * Upstream defines it in `transports/http.c` which we keep pruned.
 *
 * Note: `git_smart__ofs_delta_enabled` is NO LONGER defined here --
 * upstream `transports/smart_protocol.c` (un-pruned in PDR-012 Phase
 * 1) owns that definition.
 *
 * amiport: PDR-010 Phase 2 Stage 3 (original stub) + PDR-012 Phase 1
 * (subtransport stubs added). Documented in PATCHES.md.
 */

#include <stddef.h>
#include <stdbool.h>

#include "common.h"
#include "errors.h"
#include "git2/errors.h"
#include "git2/sys/transport.h"

/*
 * git_http__expect_continue -- HTTP Expect: 100-continue flag.
 * Referenced by settings.c's GIT_OPT_HTTP_EXPECT_CONTINUE handler.
 * Upstream declares this in transports/http.h as `bool` so our
 * definition matches.
 */
bool git_http__expect_continue = false;

/*
 * Helper for reporting "not available in amiport build" to callers.
 */
static int amiport_set_not_implemented(const char *name)
{
	git_error_set(GIT_ERROR_NET,
		"%s: not available in amiport build "
		"(only the amigit https:// smart transport is supported)",
		name);
	return GIT_ERROR;
}

/*
 * git_transport_local -- file:// transport. Would normally be
 * implemented by transports/local.c. amigit can't reach a file://
 * remote on AmigaOS anyway, so the stub just errors out.
 */
int git_transport_local(
	git_transport **out,
	git_remote *owner,
	void *payload)
{
	(void)owner;
	(void)payload;
	if (out) *out = NULL;
	return amiport_set_not_implemented("git_transport_local");
}

/*
 * git_smart_subtransport_http -- upstream HTTP subtransport.
 * Stays pruned. amigit registers its own https:// smart subtransport
 * via git_transport_register() before any remote lookup, so
 * transport_find_by_url() returns amigit's backend instead of this
 * one. This stub exists only so the static dispatch table in
 * transport.c resolves at link time.
 */
int git_smart_subtransport_http(
	git_smart_subtransport **out,
	git_transport *owner,
	void *param)
{
	(void)owner;
	(void)param;
	if (out) *out = NULL;
	return amiport_set_not_implemented("git_smart_subtransport_http");
}

/*
 * git_smart_subtransport_git -- git:// subtransport. Stays pruned;
 * GitHub does not speak git:// and we have no plans to ship it.
 */
int git_smart_subtransport_git(
	git_smart_subtransport **out,
	git_transport *owner,
	void *param)
{
	(void)owner;
	(void)param;
	if (out) *out = NULL;
	return amiport_set_not_implemented("git_smart_subtransport_git");
}

/*
 * git_smart_subtransport_ssh -- ssh:// / ssh+git:// / git+ssh://
 * subtransport. Stays pruned. Porting libssh2 is a multi-week project
 * and is explicitly deferred per PDR-012.
 */
int git_smart_subtransport_ssh(
	git_smart_subtransport **out,
	git_transport *owner,
	void *param)
{
	(void)owner;
	(void)param;
	if (out) *out = NULL;
	return amiport_set_not_implemented("git_smart_subtransport_ssh");
}
