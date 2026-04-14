/*
 * amigit_libgit2_stubs.c -- Link-time stubs for PARTIALLY pruned libgit2.a
 *
 * As of PDR-012 Phase 1 (2026-04-14), `lib/libgit2/libgit2-020.a` now
 * ships real upstream `clone.c`, `fetch.c`, `remote.c`, `transport.c`,
 * and the smart transport trio. The `git_remote_*` / `git_clone_*`
 * stubs that lived here during PDR-010 Phase 2 have been REMOVED --
 * those symbols are resolved by the archive. Keeping them here now
 * produces `multiple definition` linker errors.
 *
 * What's still stubbed:
 *
 *   1. Missing libnix symbols (`strnlen`, `difftime`) -- libnix gap,
 *      same as before.
 *
 *   2. `select()` -- posix.c's p_poll() calls select when
 *      `GIT_IO_SELECT` is defined. amigit never clones over the
 *      network in Phase 1 (that's Phase 2+), so the symbol only needs
 *      to resolve at link time. The stub returns -1/ENOSYS.
 *      PDR-012 Phase 7 (AmiSSL integration) will replace this with a
 *      real bsdsocket-backed `select()` when network traffic is live.
 *
 *   3. `git_socket_stream__connect_timeout` / `__timeout` -- globals
 *      referenced by `settings.c` from the still-pruned
 *      `streams/socket.c`. amigit's custom smart-HTTP transport in
 *      PDR-012 Phase 2 will own its own timeout handling (socket-
 *      level via bsdsocket + TLS-level via AmiSSL), so these globals
 *      are dead weight but must resolve.
 *
 *   4. `git_failalloc_*` -- test-only code still excluded from
 *      allocators/.
 *
 *   5. `__divsf3` / `__floatunsisf` -- single-precision soft-float
 *      overrides. Without them, `patch_generate.c`'s progress
 *      fraction computation routes through ROM mathieeesingbas.library
 *      and crashes FS-UAE with Guru 8000000B (known-pitfalls.md
 *      "libgit2 patch_generate Triggers FS-UAE mathieeesingbas Crash").
 *      This is load-bearing -- do not remove.
 *
 * See known-pitfalls.md "libnix missing strnlen and difftime" and
 * "libgit2 khash requires -lm" for related gaps.
 */

#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <sys/select.h>

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
 * git_remote_* / git_clone_* -- REMOVED in PDR-012 Phase 1 (2026-04-14)
 *
 * Previously this file stubbed git_remote_free, git_remote_create,
 * git_remote_lookup, git_remote_fetch, git_remote_list, git_remote_url,
 * git_remote__matching_refspec, git_remote__matching_dst_refspec, and
 * git_clone__submodule.
 *
 * All of those symbols are now provided by the real upstream
 * `remote.c` / `clone.c` inside `libgit2-020.a`. Keeping the stubs
 * here causes multiple-definition linker errors.
 *
 * If amigit ever needs to NOT link the real remote/clone code (for
 * example, a stripped-down variant), re-introduce the stubs here and
 * unlink the upstream files from the library Makefile -- don't just
 * un-comment these.
 * ======================================================================== */

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

/* ========================================================================
 * Single-precision soft-float overrides
 * ========================================================================
 *
 * lib/libgit2/src/libgit2/patch_generate.c line 261 computes a progress
 * fraction using single-precision float:
 *
 *   float progress = patch->diff ?
 *       ((float)patch->delta_index / patch->diff->deltas.length) : 1.0f;
 *
 * This is the ONLY single-precision float division in the entire libgit2
 * build. It references two soft-float support routines:
 *
 *   - __divsf3       (single-precision float division)
 *   - __floatunsisf  (unsigned int -> single-precision float)
 *
 * libnix's versions of these route through ROM mathieeesingbas.library,
 * which is broken on FS-UAE (Guru 8000000B -- same crash pattern as the
 * libSDL2 SDL_CreateRenderer dpi_scale crash documented in crash-patterns
 * #2 and known-pitfalls "FS-UAE ROM mathieeesingbas.library is broken").
 *
 * The progress value is computed BEFORE the (output->file_cb == NULL)
 * check in patch_generated_invoke_file_callback. amigit never sets a
 * file callback (we use git_diff_to_buf which doesn't expose one), so
 * the computed progress value is always discarded. We only need these
 * stubs to not crash -- the return value is irrelevant.
 *
 * Defining both symbols here gives the linker a strong definition that
 * satisfies patch_generate.o's undefined refs BEFORE libnix's archive
 * gets consulted, so the broken ROM path is never reached.
 *
 * The parameters have to match the GCC ABI (float-by-value, returns
 * float-in-d0) but the GCC 68k calling convention passes floats in d0/d1
 * as raw bit patterns, so this is a trivial no-op that returns zero.
 */

float __divsf3(float a, float b);
float __floatunsisf(unsigned int x);

float __divsf3(float a, float b)
{
    (void)a;
    (void)b;
    return 0.0f;
}

float __floatunsisf(unsigned int x)
{
    (void)x;
    return 0.0f;
}
