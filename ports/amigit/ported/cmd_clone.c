/*
 * cmd_clone.c -- `amigit clone <url> [path]`
 *
 * PDR-012 Phase 8: first user-facing networked command. Drives
 * libgit2's git_clone() through our custom HTTPS subtransport
 * (transport_https.c, registered at main() startup), with a progress
 * callback wired into git_remote_callbacks.transfer_progress so the
 * user sees object-count updates during pack download and indexing.
 *
 * Usage:
 *   amigit clone <url>          -- clone into ./<basename>
 *   amigit clone <url> <path>   -- clone into <path>
 *
 * URL handling:
 *   - Supported schemes: https://
 *   - http:// is rejected (matches ls-remote Phase 5 behavior:
 *     our transport registers only "https", and upstream libgit2's
 *     http.c was pruned -- it would otherwise resolve to the stub
 *     from transport_stubs.c)
 *   - Bare host/path like "github.com/foo/bar" is rejected with a
 *     friendly error pointing at the https:// requirement
 *
 * Destination path handling:
 *   - If [path] is omitted, the default is the last segment of the
 *     URL path with any trailing ".git" stripped. For example,
 *     "https://example.com/foo/bar.git" -> "bar".
 *   - AmigaDOS multi-character volume names ("WORK:foo",
 *     "Ram Disk:proj") hit the same libgit2 git_fs_path_root()
 *     limitation that cmd_init does -- libgit2 runs mkdir on
 *     the target path internally, so the friendly error from
 *     cmd_init is replicated here. See the precheck block below.
 *   - If the destination already exists (file or directory),
 *     git_clone() returns GIT_EEXISTS. We surface that as a clean
 *     "destination path already exists" error instead of the raw
 *     libgit2 message.
 *
 * Progress reporting:
 *   - transfer_progress() fires many times per second during pack
 *     reception + delta resolution. Rate-limit to at most once per
 *     128 objects AND always on the final call (when indexed ==
 *     total), so the user sees "Receiving objects: ..." updates
 *     without flooding the console. No \r cursor-return is used --
 *     one line per update, because \r is unreliable across the
 *     FS-UAE console + SCRAPE test harness (crash-patterns entry
 *     on RAW-mode console echo).
 *
 * Authentication:
 *   - Handled entirely inside transport_https.c's Phase 7 401 retry
 *     path (credential.c reads ENV:GIT_HTTP_TOKEN /
 *     ENV:GIT_HTTP_USERNAME, falls back to interactive raw-mode
 *     prompt). cmd_clone passes no credential callback to libgit2,
 *     because libgit2's credential subsystem is pruned from our
 *     build and the 401 retry is amigit-internal. See PDR-012
 *     Phase 7 summary for the architectural decision.
 *
 * Exit codes:
 *   0  on success (repo cloned and checked out)
 *   10 on user error or libgit2 failure
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dos/dos.h>    /* RETURN_OK, RETURN_ERROR */

#include "git2.h"

#include "amigit.h"

/* ========================================================================
 * Usage
 * ======================================================================== */

static int cmd_clone_usage(int rc)
{
    FILE *out = (rc == RETURN_OK) ? stdout : stderr;
    fprintf(out, "usage: amigit clone <url> [path]\n\n");
    fprintf(out,
        "Clone a remote git repository over HTTPS.\n\n");
    fprintf(out, "Arguments:\n");
    fprintf(out,
        "  url     https://host[:port]/path to a remote repository\n");
    fprintf(out,
        "  path    local destination (default: basename of url)\n\n");
    fprintf(out, "Authentication:\n");
    fprintf(out,
        "  Set ENV:GIT_HTTP_TOKEN for a Personal Access Token\n");
    fprintf(out,
        "  and (optionally) ENV:GIT_HTTP_USERNAME. Without an env\n");
    fprintf(out,
        "  var, amigit prompts for a token interactively on 401.\n\n");
    fprintf(out,
        "Note: requires AmiSSL to be installed. Run `amiport install\n");
    fprintf(out,
        "amissl` once on your system, or install the AmiSSL package\n");
    fprintf(out,
        "from amiport.platesteel.net.\n");
    return rc;
}

/* ========================================================================
 * Destination path defaulting
 * ========================================================================
 *
 * Given a URL, pick a sensible default destination directory:
 *   https://example.com/foo/bar.git  -> "bar"
 *   https://example.com/foo/bar/     -> "bar"
 *   https://example.com/bar          -> "bar"
 *
 * Writes the result into `out` (size `outsize`). Returns 0 on success,
 * -1 if the URL has no usable path component or the buffer is too
 * small.
 */
static int default_dest_from_url(const char *url, char *out, size_t outsize)
{
    const char *p;
    const char *last_slash;
    const char *segment;
    size_t seg_len;

    if (url == NULL || out == NULL || outsize < 2) {
        return -1;
    }

    /* Skip the "scheme://" prefix if present. */
    p = strstr(url, "://");
    if (p != NULL) {
        p += 3;
    } else {
        p = url;
    }

    /* Advance past the host[:port] component so we don't use the host
     * name as a default destination. The path begins at the first '/'
     * after the host. */
    while (*p != '\0' && *p != '/') {
        p++;
    }
    if (*p == '\0') {
        return -1;
    }

    /* Find the last non-trailing-slash segment. Walk from the end,
     * stripping trailing '/' first, then looking back for the
     * preceding '/'. */
    {
        const char *end = url + strlen(url);
        while (end > p && end[-1] == '/') {
            end--;
        }
        if (end == p) {
            /* Path was just "/" or a run of slashes. */
            return -1;
        }

        last_slash = end;
        while (last_slash > p && last_slash[-1] != '/') {
            last_slash--;
        }
        segment = last_slash;
        seg_len = (size_t)(end - last_slash);
    }

    if (seg_len == 0) {
        return -1;
    }

    /* Strip a trailing ".git" suffix (common convention). */
    if (seg_len > 4 &&
        segment[seg_len - 4] == '.' &&
        segment[seg_len - 3] == 'g' &&
        segment[seg_len - 2] == 'i' &&
        segment[seg_len - 1] == 't') {
        seg_len -= 4;
    }
    if (seg_len == 0) {
        /* URL path was literally "/.git" or similar. */
        return -1;
    }

    if (seg_len + 1 > outsize) {
        return -1;
    }
    memcpy(out, segment, seg_len);
    out[seg_len] = '\0';
    return 0;
}

/* ========================================================================
 * URL scheme validation
 * ========================================================================
 *
 * Accept only "https://". Reject bare hostnames, http://, ftp://, git://,
 * ssh://, and file:// with a friendly error pointing at the https://
 * requirement. ls-remote (Phase 5) uses a similar scheme check via
 * libgit2's URL dispatcher; we front-run it here so users see a
 * consistent error message across clone and ls-remote.
 */
static int validate_url_scheme(const char *url)
{
    if (url == NULL || url[0] == '\0') {
        printf("clone: missing url argument\n");
        return -1;
    }
    if (strncmp(url, "https://", 8) == 0) {
        /* Verify there's at least one character of host after the
         * scheme. */
        if (url[8] == '\0' || url[8] == '/') {
            printf("clone: url '%s' has no host\n", url);
            return -1;
        }
        return 0;
    }
    if (strncmp(url, "http://", 7) == 0) {
        printf("clone: plain http:// is not supported "
               "(use https://)\n");
        return -1;
    }
    if (strstr(url, "://") != NULL) {
        printf("clone: unsupported URL scheme in '%s' "
               "(only https:// is supported)\n", url);
        return -1;
    }
    printf("clone: '%s' is not a URL "
           "(expected https://host/path)\n", url);
    return -1;
}

/* ========================================================================
 * Multi-char volume precheck
 * ========================================================================
 *
 * libgit2's git_fs_path_root() only recognizes single-letter drive
 * prefixes (see cmd_init.c for the full rationale and the three
 * failed fix attempts). `git_clone` internally calls `git_futils_mkdir`
 * on the destination, so the same limitation applies here -- cloning
 * into a multi-char AmigaOS volume path fails with "failed to make
 * directory './.'". Surface a friendly error that names the Assign
 * workaround, same message as cmd_init.
 */
static int dest_path_is_supported(const char *resolved)
{
    if (resolved == NULL || resolved[0] == '\0') {
        return 1;   /* empty path handled upstream */
    }
    if (resolved[1] == ':' || strchr(resolved, ':') == NULL) {
        /* Single-letter volume (T:, C:, S:) or pure relative/absolute
         * path -- supported. */
        return 1;
    }
    return 0;
}

static void print_volume_error(const char *resolved)
{
    const char *colon = strchr(resolved, ':');
    const char *tail = (colon != NULL) ? (colon + 1) : "";
    fprintf(stderr,
        "amigit: clone: cannot clone into '%s'\n"
        "amigit: clone: libgit2 does not recognize multi-character\n"
        "amigit: clone: AmigaOS volume names ('WORK:', 'Ram Disk:',\n"
        "amigit: clone: 'System 3.1:') as path roots. Only single-\n"
        "amigit: clone: letter assigned volumes (T:, C:, S:) work.\n"
        "amigit: clone:\n"
        "amigit: clone: Workaround -- alias your target volume to a\n"
        "amigit: clone: single letter via AmigaDOS Assign, then clone\n"
        "amigit: clone: against the alias:\n"
        "amigit: clone:     Assign R: WORK:\n"
        "amigit: clone:     amigit clone <url> R:%s\n"
        "amigit: clone:\n"
        "amigit: clone: This is a libgit2 limitation, not an amigit\n"
        "amigit: clone: bug.\n",
        resolved, tail);
}

/* ========================================================================
 * Progress callback -- git_remote_callbacks.transfer_progress
 * ========================================================================
 *
 * Fires from libgit2's pack indexer during git_clone. Rate-limited to
 * at most one line per 128 objects (plus a guaranteed final line when
 * indexed == total) so the FS-UAE console isn't flooded.
 *
 * Return value semantics (libgit2 contract):
 *   0   continue
 *   <0  cancel the download (git_clone returns the same value)
 */
typedef struct clone_progress_state {
    unsigned int last_indexed_printed;
    int          header_printed;
} clone_progress_state;

static int clone_transfer_progress(const git_indexer_progress *stats,
                                   void *payload)
{
    clone_progress_state *state = (clone_progress_state *)payload;
    unsigned int total;
    unsigned int indexed;

    if (stats == NULL || state == NULL) {
        return 0;
    }

    total = stats->total_objects;
    indexed = stats->indexed_objects;

    /* First callback with a non-zero total: print a header once. */
    if (!state->header_printed && total > 0) {
        printf("Receiving %u objects...\n", total);
        state->header_printed = 1;
    }

    /* Rate-limit: print every 128 objects, and always on the final
     * callback (indexed == total, total > 0). */
    if (total > 0 && indexed == total) {
        if (state->last_indexed_printed != indexed) {
            printf("  indexed %u/%u objects\n", indexed, total);
            state->last_indexed_printed = indexed;
        }
    } else if (indexed >= state->last_indexed_printed + 128U) {
        printf("  indexed %u/%u objects\n", indexed, total);
        state->last_indexed_printed = indexed;
    }

    (void)fflush(stdout);
    return 0;
}

/* ========================================================================
 * cmd clone entry point
 * ======================================================================== */

int amigit_cmd_clone(int argc, char **argv)
{
    const char *url = NULL;
    const char *path_arg = NULL;
    char default_dest[256];
    char resolved[256];
    git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
    clone_progress_state progress_state = { 0, 0 };
    git_repository *repo = NULL;
    int i;
    int rc;

    /* Flag parse. Only --help/-h is understood; everything else is
     * positional (first = url, second = path). */
    for (i = 2; i < argc; i++) {
        if (amigit_is_help_flag(argv[i])) {
            return cmd_clone_usage(RETURN_OK);
        }
        if (argv[i][0] == '-') {
            printf("clone: unknown option '%s'\n", argv[i]);
            return cmd_clone_usage(RETURN_ERROR);
        }
        if (url == NULL) {
            url = argv[i];
        } else if (path_arg == NULL) {
            path_arg = argv[i];
        } else {
            printf("clone: too many positional arguments\n");
            return cmd_clone_usage(RETURN_ERROR);
        }
    }

    if (url == NULL) {
        /* Mirror to stdout for the test harness (ls_remote pattern). */
        printf("clone: missing url argument\n");
        return cmd_clone_usage(RETURN_ERROR);
    }

    if (validate_url_scheme(url) != 0) {
        return RETURN_ERROR;
    }

    /* Pick the destination directory. */
    if (path_arg == NULL) {
        if (default_dest_from_url(url, default_dest,
                                  sizeof(default_dest)) != 0) {
            printf("clone: cannot derive default directory from '%s' "
                   "-- pass an explicit path\n", url);
            return RETURN_ERROR;
        }
        path_arg = default_dest;
    }

    /* Resolve the destination path through the shared amigit helper
     * so "." expands to CWD and "T:foo" gets rewritten to "T:/foo"
     * before libgit2 sees it. */
    if (amigit_resolve_repo_path(path_arg, resolved, sizeof(resolved))
            != RETURN_OK) {
        fprintf(stderr,
                "amigit: clone: cannot resolve path '%s'\n", path_arg);
        return RETURN_ERROR;
    }

    /* Multi-char volume precheck -- libgit2's mkdir walk cannot
     * handle these (see cmd_init.c). */
    if (!dest_path_is_supported(resolved)) {
        print_volume_error(resolved);
        return RETURN_ERROR;
    }

    /* Wire up the progress callback. Leave checkout strategy at the
     * GIT_CLONE_OPTIONS_INIT default (GIT_CHECKOUT_SAFE). */
    opts.fetch_opts.callbacks.transfer_progress =
        clone_transfer_progress;
    opts.fetch_opts.callbacks.payload = &progress_state;

    printf("Cloning into '%s'...\n", resolved);
    (void)fflush(stdout);

    rc = git_clone(&repo, url, resolved, &opts);
    if (rc != 0) {
        /* Mirror the libgit2 error to stdout for the test harness --
         * matches the ls-remote pattern (stderr AND stdout). */
        const git_error *e = git_error_last();
        const char *msg = (e != NULL && e->message != NULL)
                          ? e->message : "unknown error";
        if (rc == GIT_EEXISTS) {
            printf("clone: destination path '%s' already exists\n",
                   resolved);
        } else {
            printf("clone: %s\n", msg);
        }
        return amigit_error_exit(rc);
    }

    /* Final progress line in case the callback didn't fire at
     * indexed == total (very small clones can short-circuit). */
    if (progress_state.header_printed &&
        progress_state.last_indexed_printed == 0) {
        printf("  done\n");
    }

    printf("Cloned into '%s'\n", resolved);

    git_repository_free(repo);
    return RETURN_OK;
}
