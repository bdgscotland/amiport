/*
 * amigit_libgit2_stubs.c -- Link-time stubs for pruned libgit2.a
 *
 * lib/libgit2/libgit2.a is built with networking, clone, fetch, remote,
 * submodule transports, and failalloc excluded. However, some objects
 * inside the archive (branch.o, repository.o, submodule.o, settings.o,
 * posix.o, alloc.o) still contain references to the excluded symbols.
 * These stubs resolve those references at final link so amigit and its
 * tests can link without pulling in the excluded code.
 *
 * Every stub returns a "this is not available" code (GIT_ENOTFOUND or
 * equivalent); they are never called at runtime because the call sites
 * are on dead networking paths. The stubs exist solely to satisfy the
 * linker.
 *
 * This file is shared verbatim with tests/libgit2/test_libgit2.c --
 * when the underlying libgit2 build configuration changes (e.g. if
 * networking is added in Phase 4), both this file and the test binary
 * will need updating.
 *
 * See PDR-010a section "Build dependencies" for the rationale.
 * See known-pitfalls.md "libnix missing strnlen and difftime" and
 * "libgit2 khash requires -lm" for related gaps.
 */

#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <sys/select.h>

#include "git2/types.h"
#include "git2/errors.h"
#include "git2/strarray.h"
#include "git2/remote.h"
#include "git2/clone.h"

/* ========================================================================
 * libnix gaps: strnlen and difftime
 * ======================================================================== */

/*
 * strnlen -- POSIX function absent from libnix.
 * Used by libgit2 index.c, alloc.c, and midx.c.
 */
size_t strnlen(const char *s, size_t maxlen)
{
    size_t i;
    for (i = 0; i < maxlen; i++) {
        if (s[i] == '\0') {
            return i;
        }
    }
    return maxlen;
}

/*
 * difftime -- absent from libnix (POSIX/C89 standard, but libnix omits it).
 * Returns the difference between two time_t values as a double.
 */
double difftime(time_t t1, time_t t0)
{
    return (double)(t1 - t0);
}

/* ========================================================================
 * select() -- posix.c p_poll() path (GIT_IO_SELECT)
 * ======================================================================== */

/*
 * select -- used by p_poll() in src/util/posix.c. Networking is stripped,
 * so p_poll is never called at runtime, but the symbol must resolve.
 */
int select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds,
           struct timeval *tv)
{
    (void)nfds; (void)rfds; (void)wfds; (void)efds; (void)tv;
    errno = ENOSYS;
    return -1;
}

/* ========================================================================
 * git_remote_* -- remote.c excluded
 * ======================================================================== */

void git_remote_free(git_remote *remote)
{
    (void)remote;
}

int git_remote_create(git_remote **out, git_repository *repo,
                      const char *name, const char *url)
{
    (void)repo; (void)name; (void)url;
    *out = NULL;
    return GIT_ENOTFOUND;
}

int git_remote_lookup(git_remote **out, git_repository *repo,
                      const char *name)
{
    (void)repo; (void)name;
    *out = NULL;
    return GIT_ENOTFOUND;
}

int git_remote_fetch(git_remote *remote,
                     const git_strarray *refspecs,
                     const git_fetch_options *opts,
                     const char *reflog_message)
{
    (void)remote; (void)refspecs; (void)opts; (void)reflog_message;
    return GIT_ENOTFOUND;
}

int git_remote_list(git_strarray *out, git_repository *repo)
{
    (void)repo;
    out->strings = NULL;
    out->count = 0;
    return 0;
}

const char *git_remote_url(const git_remote *remote)
{
    (void)remote;
    return NULL;
}

const git_refspec *git_remote__matching_refspec(git_remote *remote,
                                                 const char *refname)
{
    (void)remote; (void)refname;
    return NULL;
}

const git_refspec *git_remote__matching_dst_refspec(git_remote *remote,
                                                      const char *refname)
{
    (void)remote; (void)refname;
    return NULL;
}

/* ========================================================================
 * git_clone__submodule -- clone.c excluded
 * ======================================================================== */

int git_clone__submodule(git_repository **out, const char *url,
                          const char *local_path,
                          const git_clone_options *opts)
{
    (void)out; (void)url; (void)local_path; (void)opts;
    return GIT_ENOTFOUND;
}

/* ========================================================================
 * git_failalloc_* -- failalloc.c excluded from allocators/
 * ======================================================================== */

void *git_failalloc_malloc(size_t n, const char *file, int line)
{
    (void)n; (void)file; (void)line;
    return NULL;
}

void *git_failalloc_realloc(void *ptr, size_t n, const char *file, int line)
{
    (void)ptr; (void)n; (void)file; (void)line;
    return NULL;
}

void git_failalloc_free(void *ptr)
{
    (void)ptr;
}

/* ========================================================================
 * git_socket_stream__* -- globals from excluded socket_stream.c
 * ======================================================================== */

int git_socket_stream__connect_timeout = 0;
int git_socket_stream__timeout = 0;
