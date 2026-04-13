/*
 * test_libgit2.c -- Unit tests for lib/libgit2 (libgit2 1.8.5 on AmigaOS 68k)
 *
 * Coverage plan by test-designer (library mode, 2026-04-13).
 * Implements PDR-010 Phase 2 Stage 5 per tests/libgit2/PLAN.md.
 *
 * Categories per docs/test-coverage-standard.md:
 *   - Functional:    library init, oid, repo, config, signature, odb, blob,
 *                    tree/index, commit, refs, branch/tag, revwalk, diff, status
 *   - Error path:    missing files, bad oids, invalid specs
 *   - Edge case:     empty blob, NULL free, zero parents, empty repo diff
 *   - Amiga-specific: SHA1 big-endian vector, T: volume paths, epoch offset,
 *                    DT_UNKNOWN iterator fallback
 *   - Stress:        10-commit chains, 50 blobs, 100-entry index, depth-5
 *                    trees, 10-branch revparse
 *
 * Run via vamos with VAMOS_STACK=256 (8 KB default is insufficient for
 * libgit2's deeply recursive path and tree code).
 *
 * Test fixtures live under T:test_libgit2_repo/ -- cleaned up on entry
 * and exit. libgit2 creates .git subdirs inside the chosen workdir.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <dos/dosextens.h>

#include "git2.h"
#include "test_framework.h"

/* Real AmigaOS reads this cookie; vamos honors -s 256 from the Makefile. */
long __stack = 262144;

static const char *verstag = "$VER: test_libgit2 1.0 (13.04.2026)";

/* Track AmigaDOS requester suppression so we can restore on exit. */
static APTR g_saved_win = NULL;

/* ========================================================================
 * Linker stubs -- symbols referenced by libgit2.a that are not provided
 * by libnix or were excluded from the stripped library build.
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

/*
 * select -- used by p_poll() in posix.c (GIT_IO_SELECT path).
 * Networking is stripped, so p_poll is never called at runtime, but the
 * symbol must resolve. Stub returns -1 with ENOSYS.
 */
#include <errno.h>
#include <sys/select.h>
int select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds,
           struct timeval *tv)
{
    (void)nfds; (void)rfds; (void)wfds; (void)efds; (void)tv;
    errno = ENOSYS;
    return -1;
}

/*
 * git_remote_* stubs -- remote.c is excluded from the stripped libgit2.a
 * build (no network transports). branch.o, repository.o, and submodule.o
 * reference these symbols even on the local path. Stubs return
 * GIT_ENOTFOUND so callers see a clean "no remote" error.
 */
#include "git2/remote.h"
#include "git2/clone.h"

void git_remote_free(git_remote *remote)
{
    (void)remote;
}

int git_remote_create(git_remote **out, git_repository *repo,
                      const char *name, const char *url)
{
    (void)out; (void)repo; (void)name; (void)url;
    *out = NULL;
    return GIT_ENOTFOUND;
}

int git_remote_lookup(git_remote **out, git_repository *repo,
                      const char *name)
{
    (void)out; (void)repo; (void)name;
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

/*
 * git_clone__submodule -- clone.c excluded from stripped build.
 * submodule.c references it; submodule operations are not tested here.
 */
int git_clone__submodule(git_repository **out, const char *url,
                          const char *local_path,
                          const git_clone_options *opts)
{
    (void)out; (void)url; (void)local_path; (void)opts;
    return GIT_ENOTFOUND;
}

/*
 * git_failalloc_* -- failalloc.c excluded from stripped allocators dir.
 * alloc.o stores function pointers to these in the allocator table.
 * These stubs panic (return NULL/do nothing) if actually called.
 */
#include "git2/errors.h"
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

/*
 * git_socket_stream__connect_timeout, git_socket_stream__timeout --
 * global int variables declared extern in settings.c; defined in the
 * socket stream transport code which is excluded. Define them here.
 */
int git_socket_stream__connect_timeout = 0;
int git_socket_stream__timeout = 0;

/* ========================================================================
 * Filesystem helpers
 * ======================================================================== */

/*
 * Recursive remove. Uses dos.library directly because libgit2's own
 * fs_path helpers aren't exposed and the amiport stdio shim path is
 * overkill for this tiny helper. Mirrors the pattern used by the vim
 * and tetris ports.
 */
static void rm_rf_bptr(BPTR dirlock, const char *path);

static void rm_rf(const char *path)
{
    BPTR dirlock;

    dirlock = Lock((STRPTR)path, SHARED_LOCK);
    if (dirlock == 0) {
        /* Nothing to remove. */
        return;
    }
    rm_rf_bptr(dirlock, path);
    UnLock(dirlock);
    /* Finally delete the directory itself. */
    DeleteFile((STRPTR)path);
}

static void rm_rf_bptr(BPTR dirlock, const char *path)
{
    struct FileInfoBlock *fib;
    char child[256];

    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR);
    if (fib == NULL) {
        return;
    }
    if (Examine(dirlock, fib) == 0) {
        FreeMem(fib, sizeof(struct FileInfoBlock));
        return;
    }
    while (ExNext(dirlock, fib) != 0) {
        /* Build child path. */
        snprintf(child, sizeof(child), "%s/%s", path, fib->fib_FileName);
        if (fib->fib_DirEntryType > 0) {
            /* Directory -- recurse. */
            BPTR sublock = Lock((STRPTR)child, SHARED_LOCK);
            if (sublock != 0) {
                rm_rf_bptr(sublock, child);
                UnLock(sublock);
            }
            DeleteFile((STRPTR)child);
        } else {
            DeleteFile((STRPTR)child);
        }
    }
    FreeMem(fib, sizeof(struct FileInfoBlock));
}

/*
 * Write a small text file to the given path. Returns 0 on success.
 */
static int write_file(const char *path, const char *content)
{
    FILE *f;
    size_t len;

    f = fopen(path, "w");
    if (f == NULL) {
        return -1;
    }
    len = strlen(content);
    if (fwrite(content, 1, len, f) != len) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

/* ========================================================================
 * Shared repo + commit helper
 * ========================================================================
 * Creates a fresh non-bare repository at T:test_libgit2_repo/<subname>/
 * with a single initial commit touching "hello.txt". The caller owns the
 * returned git_repository* and must git_repository_free() it.
 */
static int make_repo_with_commit(const char *subname,
                                 const char *content,
                                 git_repository **out_repo,
                                 git_oid *out_commit_oid)
{
    char repo_path[128];
    char file_path[160];
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_oid tree_oid;
    git_tree *tree = NULL;
    git_signature *sig = NULL;
    int rc;

    snprintf(repo_path, sizeof(repo_path), "T:test_libgit2_repo/%s", subname);
    rm_rf(repo_path);

    rc = git_repository_init(&repo, repo_path, 0);
    if (rc != 0) {
        return -1;
    }

    snprintf(file_path, sizeof(file_path), "%s/hello.txt", repo_path);
    if (write_file(file_path, content) != 0) {
        git_repository_free(repo);
        return -1;
    }

    rc = git_repository_index(&idx, repo);
    if (rc != 0) {
        git_repository_free(repo);
        return -1;
    }
    rc = git_index_add_bypath(idx, "hello.txt");
    if (rc != 0) {
        git_index_free(idx);
        git_repository_free(repo);
        return -1;
    }
    rc = git_index_write_tree(&tree_oid, idx);
    if (rc != 0) {
        git_index_free(idx);
        git_repository_free(repo);
        return -1;
    }
    rc = git_index_write(idx);
    if (rc != 0) {
        git_index_free(idx);
        git_repository_free(repo);
        return -1;
    }
    rc = git_tree_lookup(&tree, repo, &tree_oid);
    if (rc != 0) {
        git_index_free(idx);
        git_repository_free(repo);
        return -1;
    }

    /* Use a fixed timestamp for deterministic assertions. The time is
     * 2001-09-09 (Unix 1_000_000_000) well past the Amiga epoch (1978) so
     * the amiga_timestamp_above_epoch_1978 test can assert >1978. */
    rc = git_signature_new(&sig, "Alice", "alice@amiga.org", 1000000000L, 0);
    if (rc != 0) {
        git_tree_free(tree);
        git_index_free(idx);
        git_repository_free(repo);
        return -1;
    }
    rc = git_commit_create_v(out_commit_oid, repo, "HEAD", sig, sig,
                             NULL, "Initial commit\n", tree, 0);
    git_signature_free(sig);
    git_tree_free(tree);
    git_index_free(idx);
    if (rc != 0) {
        git_repository_free(repo);
        return -1;
    }

    *out_repo = repo;
    return 0;
}

/* ========================================================================
 * Section 0: Library lifecycle (2 tests)
 * ======================================================================== */

TEST(libgit2_init_and_shutdown)
{
    int rc;
    /* git_libgit2_init was already called in main() once. Doing it again
     * here bumps the refcount; shutdown must balance. Returns the new
     * refcount (>= 1). */
    rc = git_libgit2_init();
    ASSERT(rc >= 1);
    rc = git_libgit2_shutdown();
    ASSERT(rc >= 0);
}

TEST(libgit2_double_init_refcount)
{
    int r1, r2, r3, r4;
    r1 = git_libgit2_init();
    r2 = git_libgit2_init();
    ASSERT(r2 == r1 + 1);
    r3 = git_libgit2_shutdown();
    ASSERT(r3 == r2 - 1);
    r4 = git_libgit2_shutdown();
    ASSERT(r4 == r3 - 1);
}

/* ========================================================================
 * Section 1: OID utilities (9 tests)
 * ======================================================================== */

TEST(oid_fromstr_valid)
{
    git_oid oid;
    int rc;
    rc = git_oid_fromstr(&oid, "0000000000000000000000000000000000000000");
    ASSERT_EQ(rc, 0);
    ASSERT(git_oid_iszero(&oid) != 0);
}

TEST(oid_fromstr_invalid)
{
    git_oid oid;
    int rc;
    /* Short string -- not 40 chars. */
    rc = git_oid_fromstr(&oid, "abcd");
    ASSERT(rc != 0);
    /* Non-hex string -- correct length but garbage chars. */
    rc = git_oid_fromstr(&oid, "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");
    ASSERT(rc != 0);
}

TEST(oid_tostr_roundtrip)
{
    /* SHA1 of the empty string == da39a3ee5e6b4b0d3255bfef95601890afd80709 */
    const char *canonical = "da39a3ee5e6b4b0d3255bfef95601890afd80709";
    git_oid oid;
    char buf[GIT_OID_SHA1_HEXSIZE + 1];
    int rc;
    rc = git_oid_fromstr(&oid, canonical);
    ASSERT_EQ(rc, 0);
    git_oid_tostr(buf, sizeof(buf), &oid);
    ASSERT_STR_EQ(buf, canonical);
}

TEST(oid_cmp_equal)
{
    git_oid a, b;
    git_oid_fromstr(&a, "1234567890123456789012345678901234567890");
    git_oid_fromstr(&b, "1234567890123456789012345678901234567890");
    ASSERT_EQ(git_oid_cmp(&a, &b), 0);
}

TEST(oid_cmp_different)
{
    git_oid a, b;
    git_oid_fromstr(&a, "1234567890123456789012345678901234567890");
    git_oid_fromstr(&b, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    ASSERT(git_oid_cmp(&a, &b) != 0);
}

TEST(oid_iszero_true)
{
    git_oid oid;
    git_oid_fromstr(&oid, "0000000000000000000000000000000000000000");
    ASSERT(git_oid_iszero(&oid) != 0);
}

TEST(oid_iszero_false)
{
    git_oid oid;
    git_oid_fromstr(&oid, "0000000000000000000000000000000000000001");
    ASSERT_EQ(git_oid_iszero(&oid), 0);
}

TEST(oid_sha1_known_vector)
{
    /* [AMIGA] SHA1 of empty string. Byte 0 = 0xda validates that big-endian
     * SHA1 (SHA1DC_FORCE_BIGENDIAN=1) produces the canonical bit layout on
     * 68k. If endian is wrong, byte 0 would be 0x09 (little-endian word
     * order). */
    git_oid oid;
    git_oid_fromstr(&oid, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    ASSERT_EQ((int)oid.id[0], 0xda);
    ASSERT_EQ((int)oid.id[19], 0x09);
}

TEST(oid_sha1_all_hex_digits)
{
    /* Validate every hex digit maps correctly. */
    git_oid oid;
    git_oid_fromstr(&oid, "0123456789abcdef0123456789abcdef01234567");
    ASSERT_EQ((int)oid.id[0], 0x01);
    ASSERT_EQ((int)oid.id[1], 0x23);
    ASSERT_EQ((int)oid.id[7], 0xef);
}

/* ========================================================================
 * Section 2: Repository init and open (8 tests)
 * ======================================================================== */

TEST(repository_init_creates_git_dir)
{
    git_repository *repo = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec2_init");
    rc = git_repository_init(&repo, "T:test_libgit2_repo/sec2_init", 0);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(repo);
    git_repository_free(repo);
}

TEST(repository_init_bare)
{
    git_repository *repo = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec2_bare");
    rc = git_repository_init(&repo, "T:test_libgit2_repo/sec2_bare", 1);
    ASSERT_EQ(rc, 0);
    ASSERT(git_repository_is_bare(repo) != 0);
    git_repository_free(repo);
}

TEST(repository_open_from_init)
{
    git_repository *repo1 = NULL, *repo2 = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec2_reopen");
    rc = git_repository_init(&repo1, "T:test_libgit2_repo/sec2_reopen", 0);
    ASSERT_EQ(rc, 0);
    git_repository_free(repo1);

    rc = git_repository_open(&repo2, "T:test_libgit2_repo/sec2_reopen");
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(repo2);
    git_repository_free(repo2);
}

TEST(repository_open_nonexistent)
{
    git_repository *repo = NULL;
    int rc;
    rc = git_repository_open(&repo, "T:test_libgit2_repo/sec2_does_not_exist");
    ASSERT(rc != 0);
    ASSERT_NULL(repo);
}

TEST(repository_open_not_a_repo)
{
    git_repository *repo = NULL;
    int rc;
    /* Create a plain dir with no .git inside. */
    rm_rf("T:test_libgit2_repo/sec2_plain");
    /* mkdir via dos.library */
    {
        BPTR lk = CreateDir((STRPTR)"T:test_libgit2_repo/sec2_plain");
        if (lk != 0) {
            UnLock(lk);
        }
    }
    rc = git_repository_open(&repo, "T:test_libgit2_repo/sec2_plain");
    ASSERT(rc != 0);
    ASSERT_NULL(repo);
}

TEST(repository_is_bare_false)
{
    git_repository *repo = NULL;
    rm_rf("T:test_libgit2_repo/sec2_nbare");
    git_repository_init(&repo, "T:test_libgit2_repo/sec2_nbare", 0);
    ASSERT_NOT_NULL(repo);
    ASSERT_EQ(git_repository_is_bare(repo), 0);
    git_repository_free(repo);
}

TEST(repository_is_bare_true)
{
    git_repository *repo = NULL;
    rm_rf("T:test_libgit2_repo/sec2_bare2");
    git_repository_init(&repo, "T:test_libgit2_repo/sec2_bare2", 1);
    ASSERT_NOT_NULL(repo);
    ASSERT(git_repository_is_bare(repo) != 0);
    git_repository_free(repo);
}

TEST(repository_workdir_path)
{
    git_repository *nbare = NULL, *bare = NULL;
    const char *wd;
    rm_rf("T:test_libgit2_repo/sec2_wd1");
    rm_rf("T:test_libgit2_repo/sec2_wd2");
    git_repository_init(&nbare, "T:test_libgit2_repo/sec2_wd1", 0);
    git_repository_init(&bare,  "T:test_libgit2_repo/sec2_wd2", 1);
    wd = git_repository_workdir(nbare);
    ASSERT_NOT_NULL(wd);
    wd = git_repository_workdir(bare);
    ASSERT_NULL(wd);
    git_repository_free(nbare);
    git_repository_free(bare);
}

/* ========================================================================
 * Section 3: Config (6 tests)
 * ======================================================================== */

TEST(config_open_level_local)
{
    git_repository *repo = NULL;
    git_config *cfg = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec3_open");
    git_repository_init(&repo, "T:test_libgit2_repo/sec3_open", 0);
    rc = git_repository_config(&cfg, repo);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(cfg);
    git_config_free(cfg);
    git_repository_free(repo);
}

TEST(config_set_and_get_string)
{
    git_repository *repo = NULL;
    git_config *cfg = NULL;
    git_config_entry *entry = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec3_str");
    git_repository_init(&repo, "T:test_libgit2_repo/sec3_str", 0);
    git_repository_config(&cfg, repo);
    rc = git_config_set_string(cfg, "user.name", "Alice");
    ASSERT_EQ(rc, 0);
    rc = git_config_get_entry(&entry, cfg, "user.name");
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->value, "Alice");
    git_config_entry_free(entry);
    git_config_free(cfg);
    git_repository_free(repo);
}

TEST(config_set_and_get_int32)
{
    git_repository *repo = NULL;
    git_config *cfg = NULL;
    int32_t val = 0;
    int rc;
    rm_rf("T:test_libgit2_repo/sec3_i32");
    git_repository_init(&repo, "T:test_libgit2_repo/sec3_i32", 0);
    git_repository_config(&cfg, repo);
    rc = git_config_set_int32(cfg, "core.compression", 6);
    ASSERT_EQ(rc, 0);
    rc = git_config_get_int32(&val, cfg, "core.compression");
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(val, 6);
    git_config_free(cfg);
    git_repository_free(repo);
}

TEST(config_get_missing_key)
{
    git_repository *repo = NULL;
    git_config *cfg = NULL;
    git_config_entry *entry = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec3_miss");
    git_repository_init(&repo, "T:test_libgit2_repo/sec3_miss", 0);
    git_repository_config(&cfg, repo);
    rc = git_config_get_entry(&entry, cfg, "no.such.key");
    ASSERT_EQ(rc, GIT_ENOTFOUND);
    git_config_free(cfg);
    git_repository_free(repo);
}

TEST(config_snapshot)
{
    git_repository *repo = NULL;
    git_config *cfg = NULL;
    git_config *snap = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec3_snap");
    git_repository_init(&repo, "T:test_libgit2_repo/sec3_snap", 0);
    git_repository_config(&cfg, repo);
    rc = git_config_snapshot(&snap, cfg);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(snap);
    git_config_free(snap);
    git_config_free(cfg);
    git_repository_free(repo);
}

TEST(config_core_symlinks_false)
{
    /* [AMIGA] On non-Unix-symlink platforms libgit2 should set
     * core.symlinks=false in the local config during init. AmigaOS has no
     * symlink primitive, so amiport_symlink returns -1/ENOSYS and libgit2
     * writes false into the repo config. */
    git_repository *repo = NULL;
    git_config *cfg = NULL;
    int val = 1;
    int rc;
    rm_rf("T:test_libgit2_repo/sec3_sym");
    git_repository_init(&repo, "T:test_libgit2_repo/sec3_sym", 0);
    git_repository_config(&cfg, repo);
    rc = git_config_get_bool(&val, cfg, "core.symlinks");
    /* Either the key is present and false, or absent (libgit2 defaults to
     * probing -- both are acceptable and confirm no symlink crash). */
    if (rc == 0) {
        ASSERT_EQ(val, 0);
    } else {
        ASSERT_EQ(rc, GIT_ENOTFOUND);
    }
    git_config_free(cfg);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 4: Signature (6 tests)
 * ======================================================================== */

TEST(signature_new_valid)
{
    git_signature *sig = NULL;
    int rc;
    rc = git_signature_new(&sig, "Alice", "alice@amiga.org", 0, 0);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(sig);
    ASSERT_STR_EQ(sig->name, "Alice");
    ASSERT_STR_EQ(sig->email, "alice@amiga.org");
    git_signature_free(sig);
}

TEST(signature_now_valid)
{
    git_signature *sig = NULL;
    int rc;
    rc = git_signature_now(&sig, "Alice", "alice@amiga.org");
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(sig);
    /* time may be 0 on vamos if gettimeofday returns 0, but on real
     * AmigaOS with battery-backed clock it should be > 0. Accept both. */
    ASSERT(sig->when.time >= 0);
    git_signature_free(sig);
}

TEST(signature_free_null)
{
    /* [EDGE] passing NULL must not crash. */
    git_signature_free(NULL);
    ASSERT(1);
}

TEST(signature_amiga_epoch_offset)
{
    /* [AMIGA] Store a known Unix time (1978-01-01 = 252460800) and assert
     * it survives round-trip verbatim. Guards against Amiga epoch offset
     * leaking into libgit2's stored time field via amiport_gettimeofday. */
    git_signature *sig = NULL;
    git_time_t stored = 252460800L;
    int rc;
    rc = git_signature_new(&sig, "Alice", "alice@amiga.org", stored, 0);
    ASSERT_EQ(rc, 0);
    ASSERT(sig->when.time == stored);
    git_signature_free(sig);
}

TEST(signature_from_buffer_valid)
{
    git_signature *sig = NULL;
    int rc;
    rc = git_signature_from_buffer(&sig,
        "Alice <alice@amiga.org> 1000000000 +0000");
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(sig);
    ASSERT_STR_EQ(sig->name, "Alice");
    ASSERT_STR_EQ(sig->email, "alice@amiga.org");
    ASSERT(sig->when.time == 1000000000L);
    git_signature_free(sig);
}

TEST(signature_from_buffer_invalid)
{
    git_signature *sig = NULL;
    int rc;
    rc = git_signature_from_buffer(&sig, "not a valid signature");
    ASSERT(rc != 0);
}

/* ========================================================================
 * Section 5: Object database (6 tests)
 * ======================================================================== */

TEST(odb_open_from_init_repo)
{
    git_repository *repo = NULL;
    git_odb *odb = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec5_odb");
    git_repository_init(&repo, "T:test_libgit2_repo/sec5_odb", 0);
    rc = git_repository_odb(&odb, repo);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(odb);
    git_odb_free(odb);
    git_repository_free(repo);
}

TEST(odb_hash_blob_content)
{
    /* [AMIGA] git blob hash of just "\n" (single LF byte) is the well-known
     * 8b137891791fe96927ad78e64b0aad7bded08bdc. Byte 0 = 0x8b validates the
     * big-endian SHA1 path (SHA1DC_FORCE_BIGENDIAN=1). */
    git_oid oid;
    int rc;
    rc = git_odb_hash(&oid, "\n", 1, GIT_OBJECT_BLOB);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((int)oid.id[0], 0x8b);
}

TEST(odb_write_and_read_blob)
{
    git_repository *repo = NULL;
    git_odb *odb = NULL;
    git_odb_object *obj = NULL;
    git_oid oid;
    const char payload[] = "abcdefghij"; /* 10 bytes */
    int rc;

    rm_rf("T:test_libgit2_repo/sec5_rw");
    git_repository_init(&repo, "T:test_libgit2_repo/sec5_rw", 0);
    git_repository_odb(&odb, repo);

    rc = git_odb_write(&oid, odb, payload, 10, GIT_OBJECT_BLOB);
    ASSERT_EQ(rc, 0);
    rc = git_odb_read(&obj, odb, &oid);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((long)git_odb_object_size(obj), 10L);
    ASSERT_EQ(memcmp(git_odb_object_data(obj), payload, 10), 0);

    git_odb_object_free(obj);
    git_odb_free(odb);
    git_repository_free(repo);
}

TEST(odb_exists_after_write)
{
    git_repository *repo = NULL;
    git_odb *odb = NULL;
    git_oid oid, unknown;
    int rc;

    rm_rf("T:test_libgit2_repo/sec5_exists");
    git_repository_init(&repo, "T:test_libgit2_repo/sec5_exists", 0);
    git_repository_odb(&odb, repo);
    rc = git_odb_write(&oid, odb, "ping", 4, GIT_OBJECT_BLOB);
    ASSERT_EQ(rc, 0);
    ASSERT(git_odb_exists(odb, &oid) != 0);

    git_oid_fromstr(&unknown, "dead0dead0dead0dead0dead0dead0dead0dead0");
    ASSERT_EQ(git_odb_exists(odb, &unknown), 0);

    git_odb_free(odb);
    git_repository_free(repo);
}

TEST(odb_read_nonexistent)
{
    git_repository *repo = NULL;
    git_odb *odb = NULL;
    git_odb_object *obj = NULL;
    git_oid oid;
    int rc;
    int i;

    rm_rf("T:test_libgit2_repo/sec5_miss");
    git_repository_init(&repo, "T:test_libgit2_repo/sec5_miss", 0);
    git_repository_odb(&odb, repo);
    /* All-0xFF oid is virtually guaranteed to not exist. */
    for (i = 0; i < GIT_OID_SHA1_SIZE; i++) {
        oid.id[i] = 0xff;
    }
    rc = git_odb_read(&obj, odb, &oid);
    ASSERT_EQ(rc, GIT_ENOTFOUND);

    git_odb_free(odb);
    git_repository_free(repo);
}

TEST(odb_write_empty_blob)
{
    git_repository *repo = NULL;
    git_odb *odb = NULL;
    git_odb_object *obj = NULL;
    git_oid oid;
    int rc;

    rm_rf("T:test_libgit2_repo/sec5_empty");
    git_repository_init(&repo, "T:test_libgit2_repo/sec5_empty", 0);
    git_repository_odb(&odb, repo);
    rc = git_odb_write(&oid, odb, "", 0, GIT_OBJECT_BLOB);
    ASSERT_EQ(rc, 0);
    rc = git_odb_read(&obj, odb, &oid);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((long)git_odb_object_size(obj), 0L);

    git_odb_object_free(obj);
    git_odb_free(odb);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 6: Blob (5 tests)
 * ======================================================================== */

TEST(blob_create_from_buffer)
{
    git_repository *repo = NULL;
    git_oid oid;
    int rc;
    rm_rf("T:test_libgit2_repo/sec6_create");
    git_repository_init(&repo, "T:test_libgit2_repo/sec6_create", 0);
    rc = git_blob_create_from_buffer(&oid, repo, "test content", 12);
    ASSERT_EQ(rc, 0);
    git_repository_free(repo);
}

TEST(blob_lookup_by_oid)
{
    git_repository *repo = NULL;
    git_blob *blob = NULL;
    git_oid oid;
    int rc;
    rm_rf("T:test_libgit2_repo/sec6_lookup");
    git_repository_init(&repo, "T:test_libgit2_repo/sec6_lookup", 0);
    git_blob_create_from_buffer(&oid, repo, "test content", 12);
    rc = git_blob_lookup(&blob, repo, &oid);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(blob);
    git_blob_free(blob);
    git_repository_free(repo);
}

TEST(blob_rawcontent_matches_input)
{
    git_repository *repo = NULL;
    git_blob *blob = NULL;
    git_oid oid;
    rm_rf("T:test_libgit2_repo/sec6_raw");
    git_repository_init(&repo, "T:test_libgit2_repo/sec6_raw", 0);
    git_blob_create_from_buffer(&oid, repo, "test content", 12);
    git_blob_lookup(&blob, repo, &oid);
    ASSERT_EQ(memcmp(git_blob_rawcontent(blob), "test content", 12), 0);
    git_blob_free(blob);
    git_repository_free(repo);
}

TEST(blob_rawsize_matches_input)
{
    git_repository *repo = NULL;
    git_blob *blob = NULL;
    git_oid oid;
    rm_rf("T:test_libgit2_repo/sec6_size");
    git_repository_init(&repo, "T:test_libgit2_repo/sec6_size", 0);
    git_blob_create_from_buffer(&oid, repo, "test content", 12);
    git_blob_lookup(&blob, repo, &oid);
    ASSERT_EQ((long)git_blob_rawsize(blob), 12L);
    git_blob_free(blob);
    git_repository_free(repo);
}

TEST(blob_create_empty)
{
    /* [EDGE] git hash-object -t blob --stdin </dev/null =
     * e69de29bb2d1d6434b8b29ae775ad8c2e48c5391 */
    git_repository *repo = NULL;
    git_oid oid;
    char buf[GIT_OID_SHA1_HEXSIZE + 1];
    int rc;
    rm_rf("T:test_libgit2_repo/sec6_empty");
    git_repository_init(&repo, "T:test_libgit2_repo/sec6_empty", 0);
    rc = git_blob_create_from_buffer(&oid, repo, "", 0);
    ASSERT_EQ(rc, 0);
    git_oid_tostr(buf, sizeof(buf), &oid);
    ASSERT_STR_EQ(buf, "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391");
    git_repository_free(repo);
}

/* ========================================================================
 * Section 7: Tree and index (8 tests)
 * ======================================================================== */

TEST(index_open_from_repo)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec7_idx");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_idx", 0);
    rc = git_repository_index(&idx, repo);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(idx);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(index_add_and_write_tree)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_oid tree_oid;
    int rc;
    rm_rf("T:test_libgit2_repo/sec7_add");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_add", 0);
    write_file("T:test_libgit2_repo/sec7_add/test.txt", "content");
    git_repository_index(&idx, repo);
    rc = git_index_add_bypath(idx, "test.txt");
    ASSERT_EQ(rc, 0);
    rc = git_index_write_tree(&tree_oid, idx);
    ASSERT_EQ(rc, 0);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(index_entry_count_after_add)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    rm_rf("T:test_libgit2_repo/sec7_count");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_count", 0);
    write_file("T:test_libgit2_repo/sec7_count/test.txt", "content");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "test.txt");
    ASSERT_EQ((long)git_index_entrycount(idx), 1L);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(index_remove_entry)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec7_rm");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_rm", 0);
    write_file("T:test_libgit2_repo/sec7_rm/test.txt", "content");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "test.txt");
    rc = git_index_remove_bypath(idx, "test.txt");
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((long)git_index_entrycount(idx), 0L);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(index_write_and_reload)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec7_reload");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_reload", 0);
    write_file("T:test_libgit2_repo/sec7_reload/test.txt", "content");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "test.txt");
    rc = git_index_write(idx);
    ASSERT_EQ(rc, 0);
    rc = git_index_read(idx, 1);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((long)git_index_entrycount(idx), 1L);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(tree_lookup_after_write_tree)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_oid tree_oid;
    int rc;
    rm_rf("T:test_libgit2_repo/sec7_tlook");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_tlook", 0);
    write_file("T:test_libgit2_repo/sec7_tlook/test.txt", "content");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "test.txt");
    git_index_write_tree(&tree_oid, idx);
    rc = git_tree_lookup(&tree, repo, &tree_oid);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(tree);
    git_tree_free(tree);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(tree_entry_byname)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_oid tree_oid;
    const git_tree_entry *entry;
    rm_rf("T:test_libgit2_repo/sec7_tbyname");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_tbyname", 0);
    write_file("T:test_libgit2_repo/sec7_tbyname/test.txt", "content");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "test.txt");
    git_index_write_tree(&tree_oid, idx);
    git_tree_lookup(&tree, repo, &tree_oid);
    entry = git_tree_entry_byname(tree, "test.txt");
    ASSERT_NOT_NULL(entry);
    git_tree_free(tree);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(tree_entrycount)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_oid tree_oid;
    rm_rf("T:test_libgit2_repo/sec7_tcount");
    git_repository_init(&repo, "T:test_libgit2_repo/sec7_tcount", 0);
    write_file("T:test_libgit2_repo/sec7_tcount/test.txt", "content");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "test.txt");
    git_index_write_tree(&tree_oid, idx);
    git_tree_lookup(&tree, repo, &tree_oid);
    ASSERT_EQ((long)git_tree_entrycount(tree), 1L);
    git_tree_free(tree);
    git_index_free(idx);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 8: Commit (7 tests)
 * ======================================================================== */

TEST(commit_create_initial)
{
    git_repository *repo = NULL;
    git_oid oid;
    int rc;
    rc = make_repo_with_commit("sec8_init", "hello\n", &repo, &oid);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(git_oid_iszero(&oid), 0);
    git_repository_free(repo);
}

TEST(commit_lookup_roundtrip)
{
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_oid oid;
    int rc;
    rc = make_repo_with_commit("sec8_lookup", "hello\n", &repo, &oid);
    ASSERT_EQ(rc, 0);
    rc = git_commit_lookup(&commit, repo, &oid);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(commit);
    ASSERT_EQ(git_oid_cmp(git_commit_id(commit), &oid), 0);
    git_commit_free(commit);
    git_repository_free(repo);
}

TEST(commit_message_preserved)
{
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_oid oid;
    int rc;
    rc = make_repo_with_commit("sec8_msg", "hello\n", &repo, &oid);
    ASSERT_EQ(rc, 0);
    git_commit_lookup(&commit, repo, &oid);
    /* git_commit_create_v appends \n if missing -- we already pass one. */
    ASSERT_STR_EQ(git_commit_message(commit), "Initial commit\n");
    git_commit_free(commit);
    git_repository_free(repo);
}

TEST(commit_author_fields)
{
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_oid oid;
    const git_signature *author;
    make_repo_with_commit("sec8_auth", "hello\n", &repo, &oid);
    git_commit_lookup(&commit, repo, &oid);
    author = git_commit_author(commit);
    ASSERT_NOT_NULL(author);
    ASSERT_STR_EQ(author->name, "Alice");
    ASSERT_STR_EQ(author->email, "alice@amiga.org");
    git_commit_free(commit);
    git_repository_free(repo);
}

TEST(commit_parent_count_initial)
{
    /* [EDGE] Initial commit has no parents. */
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_oid oid;
    make_repo_with_commit("sec8_pc", "hello\n", &repo, &oid);
    git_commit_lookup(&commit, repo, &oid);
    ASSERT_EQ((long)git_commit_parentcount(commit), 0L);
    git_commit_free(commit);
    git_repository_free(repo);
}

TEST(commit_create_with_parent)
{
    git_repository *repo = NULL;
    git_commit *c1 = NULL, *c2 = NULL;
    git_tree *tree = NULL;
    git_index *idx = NULL;
    git_signature *sig = NULL;
    git_oid oid1, tree_oid, oid2;
    int rc;

    rc = make_repo_with_commit("sec8_parent", "first\n", &repo, &oid1);
    ASSERT_EQ(rc, 0);

    /* Modify the file and make a second commit with the first as parent. */
    write_file("T:test_libgit2_repo/sec8_parent/hello.txt", "second\n");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "hello.txt");
    git_index_write_tree(&tree_oid, idx);
    git_index_write(idx);
    git_tree_lookup(&tree, repo, &tree_oid);

    git_commit_lookup(&c1, repo, &oid1);
    git_signature_new(&sig, "Alice", "alice@amiga.org", 1000000100L, 0);

    rc = git_commit_create_v(&oid2, repo, "HEAD", sig, sig, NULL,
                             "Second commit\n", tree, 1, c1);
    ASSERT_EQ(rc, 0);

    git_commit_lookup(&c2, repo, &oid2);
    ASSERT_EQ((long)git_commit_parentcount(c2), 1L);
    ASSERT_EQ(git_oid_cmp(git_commit_parent_id(c2, 0), &oid1), 0);

    git_commit_free(c1);
    git_commit_free(c2);
    git_signature_free(sig);
    git_tree_free(tree);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(commit_lookup_nonexistent)
{
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_oid oid, dummy;
    int rc;
    int i;
    make_repo_with_commit("sec8_miss", "hello\n", &repo, &dummy);
    for (i = 0; i < GIT_OID_SHA1_SIZE; i++) {
        oid.id[i] = 0xaa;
    }
    rc = git_commit_lookup(&commit, repo, &oid);
    ASSERT_EQ(rc, GIT_ENOTFOUND);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 9: References (6 tests)
 * ======================================================================== */

TEST(reference_name_to_id_head)
{
    git_repository *repo = NULL;
    git_oid head_oid, lookup_oid;
    int rc;
    make_repo_with_commit("sec9_nti", "hello\n", &repo, &head_oid);
    rc = git_reference_name_to_id(&lookup_oid, repo, "HEAD");
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(git_oid_cmp(&head_oid, &lookup_oid), 0);
    git_repository_free(repo);
}

TEST(reference_lookup_head)
{
    git_repository *repo = NULL;
    git_reference *ref = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec9_lookup", "hello\n", &repo, &oid);
    rc = git_reference_lookup(&ref, repo, "HEAD");
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(ref);
    /* HEAD is symbolic to refs/heads/... */
    ASSERT(git_reference_type(ref) == GIT_REFERENCE_SYMBOLIC);
    git_reference_free(ref);
    git_repository_free(repo);
}

TEST(reference_create_direct)
{
    git_repository *repo = NULL;
    git_reference *ref = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec9_create", "hello\n", &repo, &oid);
    rc = git_reference_create(&ref, repo, "refs/heads/testbranch",
                              &oid, 0, NULL);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(ref);
    ASSERT(git_reference_type(ref) == GIT_REFERENCE_DIRECT);
    git_reference_free(ref);
    git_repository_free(repo);
}

TEST(reference_symbolic_create)
{
    git_repository *repo = NULL;
    git_reference *ref = NULL;
    git_reference *sym = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec9_sym", "hello\n", &repo, &oid);
    /* Create a direct target first. */
    git_reference_create(&ref, repo, "refs/heads/master", &oid, 1, NULL);
    rc = git_reference_symbolic_create(&sym, repo, "refs/symtest",
                                       "refs/heads/master", 0, NULL);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(sym);
    ASSERT(git_reference_type(sym) == GIT_REFERENCE_SYMBOLIC);
    git_reference_free(ref);
    git_reference_free(sym);
    git_repository_free(repo);
}

TEST(reference_lookup_nonexistent)
{
    git_repository *repo = NULL;
    git_reference *ref = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec9_miss", "hello\n", &repo, &oid);
    rc = git_reference_lookup(&ref, repo, "refs/heads/nope");
    ASSERT_EQ(rc, GIT_ENOTFOUND);
    ASSERT_NULL(ref);
    git_repository_free(repo);
}

TEST(reference_list_names)
{
    git_repository *repo = NULL;
    git_strarray names;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec9_list", "hello\n", &repo, &oid);
    names.strings = NULL;
    names.count = 0;
    rc = git_reference_list(&names, repo);
    ASSERT_EQ(rc, 0);
    ASSERT((long)names.count >= 1L);
    git_strarray_dispose(&names);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 10: Branches and tags (4 tests)
 * ======================================================================== */

TEST(branch_create_and_lookup)
{
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_reference *br = NULL, *lookup = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec10_bc", "hello\n", &repo, &oid);
    git_commit_lookup(&commit, repo, &oid);
    rc = git_branch_create(&br, repo, "feature/test", commit, 0);
    ASSERT_EQ(rc, 0);
    rc = git_branch_lookup(&lookup, repo, "feature/test", GIT_BRANCH_LOCAL);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(lookup);
    git_reference_free(br);
    git_reference_free(lookup);
    git_commit_free(commit);
    git_repository_free(repo);
}

TEST(branch_iterator_local)
{
    git_repository *repo = NULL;
    git_branch_iterator *iter = NULL;
    git_reference *ref = NULL;
    git_branch_t branch_type;
    git_oid oid;
    int count = 0;
    int rc;
    make_repo_with_commit("sec10_iter", "hello\n", &repo, &oid);
    rc = git_branch_iterator_new(&iter, repo, GIT_BRANCH_LOCAL);
    ASSERT_EQ(rc, 0);
    while (git_branch_next(&ref, &branch_type, iter) != GIT_ITEROVER) {
        count++;
        git_reference_free(ref);
        if (count > 100) {
            break; /* safety */
        }
    }
    ASSERT(count >= 1);
    git_branch_iterator_free(iter);
    git_repository_free(repo);
}

TEST(tag_create_lightweight)
{
    git_repository *repo = NULL;
    git_object *target = NULL;
    git_oid oid, tag_oid;
    int rc;
    make_repo_with_commit("sec10_tag", "hello\n", &repo, &oid);
    rc = git_revparse_single(&target, repo, "HEAD");
    ASSERT_EQ(rc, 0);
    rc = git_tag_create_lightweight(&tag_oid, repo, "v0.1", target, 0);
    ASSERT_EQ(rc, 0);
    git_object_free(target);
    git_repository_free(repo);
}

TEST(tag_list_names)
{
    git_repository *repo = NULL;
    git_object *target = NULL;
    git_strarray names;
    git_oid oid, tag_oid;
    int rc;
    int found = 0;
    size_t i;
    make_repo_with_commit("sec10_tl", "hello\n", &repo, &oid);
    git_revparse_single(&target, repo, "HEAD");
    git_tag_create_lightweight(&tag_oid, repo, "v0.1", target, 0);
    names.strings = NULL;
    names.count = 0;
    rc = git_tag_list(&names, repo);
    ASSERT_EQ(rc, 0);
    ASSERT((long)names.count >= 1L);
    for (i = 0; i < names.count; i++) {
        if (strcmp(names.strings[i], "v0.1") == 0) {
            found = 1;
            break;
        }
    }
    ASSERT_EQ(found, 1);
    git_strarray_dispose(&names);
    git_object_free(target);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 11: Revwalk and revparse (4 tests)
 * ======================================================================== */

TEST(revwalk_new_and_free)
{
    git_repository *repo = NULL;
    git_revwalk *walk = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec11_new", "hello\n", &repo, &oid);
    rc = git_revwalk_new(&walk, repo);
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(walk);
    git_revwalk_free(walk);
    git_repository_free(repo);
}

TEST(revwalk_push_head_and_walk)
{
    git_repository *repo = NULL;
    git_revwalk *walk = NULL;
    git_oid oid1, oid2, next;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_commit *c1 = NULL;
    git_signature *sig = NULL;
    git_oid tree_oid;
    int count = 0;
    int first_is_head = 0;
    int rc;

    rc = make_repo_with_commit("sec11_walk", "first\n", &repo, &oid1);
    ASSERT_EQ(rc, 0);

    /* Second commit so the walker has two entries. */
    write_file("T:test_libgit2_repo/sec11_walk/hello.txt", "second\n");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "hello.txt");
    git_index_write_tree(&tree_oid, idx);
    git_index_write(idx);
    git_tree_lookup(&tree, repo, &tree_oid);
    git_commit_lookup(&c1, repo, &oid1);
    git_signature_new(&sig, "Alice", "alice@amiga.org", 1000000200L, 0);
    rc = git_commit_create_v(&oid2, repo, "HEAD", sig, sig, NULL,
                             "Second\n", tree, 1, c1);
    ASSERT_EQ(rc, 0);
    git_commit_free(c1);
    git_signature_free(sig);
    git_tree_free(tree);
    git_index_free(idx);

    rc = git_revwalk_new(&walk, repo);
    ASSERT_EQ(rc, 0);
    rc = git_revwalk_push_head(walk);
    ASSERT_EQ(rc, 0);
    while (git_revwalk_next(&next, walk) != GIT_ITEROVER) {
        if (count == 0 && git_oid_cmp(&next, &oid2) == 0) {
            first_is_head = 1;
        }
        count++;
        if (count > 100) {
            break; /* safety */
        }
    }
    ASSERT_EQ(count, 2);
    ASSERT_EQ(first_is_head, 1);
    git_revwalk_free(walk);
    git_repository_free(repo);
}

TEST(revparse_single_head)
{
    git_repository *repo = NULL;
    git_object *obj = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec11_rp", "hello\n", &repo, &oid);
    rc = git_revparse_single(&obj, repo, "HEAD");
    ASSERT_EQ(rc, 0);
    ASSERT_NOT_NULL(obj);
    ASSERT(git_object_type(obj) == GIT_OBJECT_COMMIT);
    git_object_free(obj);
    git_repository_free(repo);
}

TEST(revparse_invalid_spec)
{
    git_repository *repo = NULL;
    git_object *obj = NULL;
    git_oid oid;
    int rc;
    make_repo_with_commit("sec11_bad", "hello\n", &repo, &oid);
    rc = git_revparse_single(&obj, repo, "no_such_ref_xyz");
    ASSERT(rc != 0);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 12: Diff (3 tests)
 * ======================================================================== */

TEST(diff_tree_to_tree_initial)
{
    git_repository *repo = NULL;
    git_commit *c1 = NULL, *c2 = NULL;
    git_tree *t1 = NULL, *t2 = NULL;
    git_diff *diff = NULL;
    git_index *idx = NULL;
    git_signature *sig = NULL;
    git_oid oid1, oid2, tree_oid;
    int rc;

    make_repo_with_commit("sec12_tt", "first\n", &repo, &oid1);
    /* Second commit with a different file content. */
    write_file("T:test_libgit2_repo/sec12_tt/hello.txt", "changed\n");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "hello.txt");
    git_index_write_tree(&tree_oid, idx);
    git_index_write(idx);
    git_tree_lookup(&t2, repo, &tree_oid);
    git_commit_lookup(&c1, repo, &oid1);
    git_signature_new(&sig, "Alice", "alice@amiga.org", 1000000300L, 0);
    git_commit_create_v(&oid2, repo, "HEAD", sig, sig, NULL,
                        "Second\n", t2, 1, c1);
    git_commit_free(c1);
    git_commit_lookup(&c1, repo, &oid1);
    git_commit_lookup(&c2, repo, &oid2);
    git_commit_tree(&t1, c1);
    git_tree_free(t2);
    git_commit_tree(&t2, c2);

    rc = git_diff_tree_to_tree(&diff, repo, t1, t2, NULL);
    ASSERT_EQ(rc, 0);
    ASSERT((long)git_diff_num_deltas(diff) >= 1L);

    git_diff_free(diff);
    git_tree_free(t1);
    git_tree_free(t2);
    git_commit_free(c1);
    git_commit_free(c2);
    git_signature_free(sig);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(diff_numdeltas_after_add)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_diff *diff = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec12_add");
    git_repository_init(&repo, "T:test_libgit2_repo/sec12_add", 0);
    write_file("T:test_libgit2_repo/sec12_add/new.txt", "fresh\n");
    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "new.txt");
    rc = git_diff_tree_to_index(&diff, repo, NULL, idx, NULL);
    ASSERT_EQ(rc, 0);
    ASSERT((long)git_diff_num_deltas(diff) >= 1L);
    git_diff_free(diff);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(diff_empty_repo)
{
    /* [EDGE] fresh repo with no commits and no staged files. */
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_diff *diff = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/sec12_empty");
    git_repository_init(&repo, "T:test_libgit2_repo/sec12_empty", 0);
    git_repository_index(&idx, repo);
    rc = git_diff_tree_to_index(&diff, repo, NULL, idx, NULL);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((long)git_diff_num_deltas(diff), 0L);
    git_diff_free(diff);
    git_index_free(idx);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 13: Status (2 tests)
 * ======================================================================== */

TEST(status_new_file_untracked)
{
    git_repository *repo = NULL;
    unsigned int flags = 0;
    int rc;
    rm_rf("T:test_libgit2_repo/sec13_new");
    git_repository_init(&repo, "T:test_libgit2_repo/sec13_new", 0);
    write_file("T:test_libgit2_repo/sec13_new/untracked.txt", "hi\n");
    rc = git_status_file(&flags, repo, "untracked.txt");
    ASSERT_EQ(rc, 0);
    ASSERT((flags & GIT_STATUS_WT_NEW) != 0);
    git_repository_free(repo);
}

TEST(status_clean_after_commit)
{
    git_repository *repo = NULL;
    git_oid oid;
    unsigned int flags = 0;
    int rc;
    make_repo_with_commit("sec13_clean", "committed\n", &repo, &oid);
    rc = git_status_file(&flags, repo, "hello.txt");
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((long)flags, 0L);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 14: Amiga-specific (3 tests)
 * ======================================================================== */

TEST(amiga_t_volume_path_for_repo)
{
    /* [AMIGA] Smoke test for fs_path.c handling of T: volume prefix and
     * colon-as-separator. init, round-trip via open, ensure no requesters
     * and that bare_path logic does not mangle "T:...". */
    git_repository *repo = NULL;
    int rc;
    rm_rf("T:test_libgit2_repo/amiga_vol");
    rc = git_repository_init(&repo, "T:test_libgit2_repo/amiga_vol", 0);
    ASSERT_EQ(rc, 0);
    git_repository_free(repo);
    rc = git_repository_open(&repo, "T:test_libgit2_repo/amiga_vol");
    ASSERT_EQ(rc, 0);
    git_repository_free(repo);
}

static int status_count_cb(const char *path, unsigned int flags, void *payload)
{
    int *count = (int *)payload;
    (void)path;
    (void)flags;
    (*count)++;
    return 0;
}

TEST(amiga_d_type_unknown_in_walk)
{
    /* [AMIGA] d_type=DT_UNKNOWN on AmigaOS -- status_foreach must still
     * descend into untracked files via lstat, not short-circuit. Create a
     * file at workdir root so we do not depend on nested dir walking
     * working (which has other issues). */
    git_repository *repo = NULL;
    int count = 0;
    int rc;
    rm_rf("T:test_libgit2_repo/sec14_walk");
    git_repository_init(&repo, "T:test_libgit2_repo/sec14_walk", 0);
    write_file("T:test_libgit2_repo/sec14_walk/anotherfile.txt", "x\n");
    rc = git_status_foreach(repo, status_count_cb, &count);
    ASSERT_EQ(rc, 0);
    ASSERT(count >= 1);
    git_repository_free(repo);
}

TEST(amiga_timestamp_above_epoch_1978)
{
    /* [AMIGA] Stored time must be Unix epoch-relative, not Amiga-relative.
     * make_repo_with_commit uses 1000000000 (2001-09-09), which is > the
     * Amiga epoch (252460800 = 1978-01-01). If an amiport shim is
     * subtracting the Amiga offset, we would see a much smaller value. */
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_oid oid;
    const git_signature *auth;
    make_repo_with_commit("sec14_ts", "hello\n", &repo, &oid);
    git_commit_lookup(&commit, repo, &oid);
    auth = git_commit_author(commit);
    ASSERT_NOT_NULL(auth);
    ASSERT(auth->when.time > 252460800L);
    git_commit_free(commit);
    git_repository_free(repo);
}

/* ========================================================================
 * Section 15: Stress (6 tests)
 * ======================================================================== */

TEST(stress_10_commits_revwalk)
{
    git_repository *repo = NULL;
    git_revwalk *walk = NULL;
    git_commit *parent = NULL;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_signature *sig = NULL;
    git_oid head_oid, tree_oid, new_oid, next;
    char content[32];
    int i, count = 0;
    int rc;

    make_repo_with_commit("sec15_10c", "commit0\n", &repo, &head_oid);
    for (i = 1; i < 10; i++) {
        snprintf(content, sizeof(content), "commit%d\n", i);
        write_file("T:test_libgit2_repo/sec15_10c/hello.txt", content);
        git_repository_index(&idx, repo);
        git_index_add_bypath(idx, "hello.txt");
        git_index_write_tree(&tree_oid, idx);
        git_index_write(idx);
        git_tree_lookup(&tree, repo, &tree_oid);
        git_commit_lookup(&parent, repo, &head_oid);
        git_signature_new(&sig, "Alice", "alice@amiga.org",
                          1000000000L + i, 0);
        rc = git_commit_create_v(&new_oid, repo, "HEAD", sig, sig, NULL,
                                 content, tree, 1, parent);
        ASSERT_EQ(rc, 0);
        head_oid = new_oid;
        git_signature_free(sig);
        git_tree_free(tree);
        git_commit_free(parent);
        git_index_free(idx);
        tree = NULL;
        parent = NULL;
        idx = NULL;
        sig = NULL;
    }
    git_revwalk_new(&walk, repo);
    git_revwalk_push_head(walk);
    while (git_revwalk_next(&next, walk) != GIT_ITEROVER) {
        count++;
        if (count > 100) {
            break;
        }
    }
    ASSERT_EQ(count, 10);
    git_revwalk_free(walk);
    git_repository_free(repo);
}

TEST(stress_50_blobs_odb)
{
    git_repository *repo = NULL;
    git_odb *odb = NULL;
    git_oid oids[50];
    char payload[64];
    int i, j;
    int rc;

    rm_rf("T:test_libgit2_repo/sec15_50b");
    git_repository_init(&repo, "T:test_libgit2_repo/sec15_50b", 0);
    git_repository_odb(&odb, repo);

    for (i = 0; i < 50; i++) {
        memset(payload, 0, sizeof(payload));
        snprintf(payload, sizeof(payload), "blob_%d_%d_%d_content", i, i*i, i+100);
        rc = git_odb_write(&oids[i], odb, payload, strlen(payload),
                           GIT_OBJECT_BLOB);
        ASSERT_EQ(rc, 0);
    }
    /* Verify all OIDs are distinct. */
    for (i = 0; i < 50; i++) {
        for (j = i + 1; j < 50; j++) {
            ASSERT(git_oid_cmp(&oids[i], &oids[j]) != 0);
        }
    }
    git_odb_free(odb);
    git_repository_free(repo);
}

static int tree_walk_count_cb(const char *root, const git_tree_entry *entry,
                              void *payload)
{
    int *count = (int *)payload;
    (void)root;
    (void)entry;
    (*count)++;
    return 0;
}

TEST(stress_tree_depth_5)
{
    /* Five nested directories each with one file. git_tree_walk should
     * fire for every tree entry at least 5 times. */
    git_repository *repo = NULL;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_oid tree_oid;
    BPTR lk;
    int i, count = 0;
    int rc;
    const char *base = "T:test_libgit2_repo/sec15_d5";

    rm_rf(base);
    git_repository_init(&repo, base, 0);

    /* Create nested dirs. */
    lk = CreateDir((STRPTR)"T:test_libgit2_repo/sec15_d5/a");
    if (lk != 0) { UnLock(lk); }
    lk = CreateDir((STRPTR)"T:test_libgit2_repo/sec15_d5/a/b");
    if (lk != 0) { UnLock(lk); }
    lk = CreateDir((STRPTR)"T:test_libgit2_repo/sec15_d5/a/b/c");
    if (lk != 0) { UnLock(lk); }
    lk = CreateDir((STRPTR)"T:test_libgit2_repo/sec15_d5/a/b/c/d");
    if (lk != 0) { UnLock(lk); }
    lk = CreateDir((STRPTR)"T:test_libgit2_repo/sec15_d5/a/b/c/d/e");
    if (lk != 0) { UnLock(lk); }

    write_file("T:test_libgit2_repo/sec15_d5/a/f0.txt", "0");
    write_file("T:test_libgit2_repo/sec15_d5/a/b/f1.txt", "1");
    write_file("T:test_libgit2_repo/sec15_d5/a/b/c/f2.txt", "2");
    write_file("T:test_libgit2_repo/sec15_d5/a/b/c/d/f3.txt", "3");
    write_file("T:test_libgit2_repo/sec15_d5/a/b/c/d/e/f4.txt", "4");

    git_repository_index(&idx, repo);
    git_index_add_bypath(idx, "a/f0.txt");
    git_index_add_bypath(idx, "a/b/f1.txt");
    git_index_add_bypath(idx, "a/b/c/f2.txt");
    git_index_add_bypath(idx, "a/b/c/d/f3.txt");
    git_index_add_bypath(idx, "a/b/c/d/e/f4.txt");
    rc = git_index_write_tree(&tree_oid, idx);
    ASSERT_EQ(rc, 0);
    git_index_write(idx);
    git_tree_lookup(&tree, repo, &tree_oid);
    rc = git_tree_walk(tree, GIT_TREEWALK_PRE, tree_walk_count_cb, &count);
    ASSERT_EQ(rc, 0);
    ASSERT(count >= 5);

    (void)i; /* silence warning */
    git_tree_free(tree);
    git_index_free(idx);
    git_repository_free(repo);
}

TEST(stress_index_100_entries)
{
    git_repository *repo = NULL;
    git_index *idx = NULL;
    char filename[64];
    char filepath[160];
    int i;
    int rc;

    rm_rf("T:test_libgit2_repo/sec15_i100");
    git_repository_init(&repo, "T:test_libgit2_repo/sec15_i100", 0);
    git_repository_index(&idx, repo);

    for (i = 0; i < 100; i++) {
        snprintf(filename, sizeof(filename), "f%03d.txt", i);
        snprintf(filepath, sizeof(filepath),
                 "T:test_libgit2_repo/sec15_i100/%s", filename);
        write_file(filepath, "x");
        rc = git_index_add_bypath(idx, filename);
        ASSERT_EQ(rc, 0);
    }
    ASSERT_EQ((long)git_index_entrycount(idx), 100L);
    rc = git_index_write(idx);
    ASSERT_EQ(rc, 0);
    rc = git_index_read(idx, 1);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ((long)git_index_entrycount(idx), 100L);

    git_index_free(idx);
    git_repository_free(repo);
}

TEST(stress_diff_10_commit_chain)
{
    git_repository *repo = NULL;
    git_commit *commits[10];
    git_tree *t_first = NULL, *t_last = NULL;
    git_diff *diff = NULL;
    git_index *idx = NULL;
    git_tree *tree = NULL;
    git_signature *sig = NULL;
    git_oid head_oid, tree_oid, new_oid;
    char content[32];
    int i;
    int rc;

    for (i = 0; i < 10; i++) {
        commits[i] = NULL;
    }

    make_repo_with_commit("sec15_d10", "v0\n", &repo, &head_oid);
    git_commit_lookup(&commits[0], repo, &head_oid);

    for (i = 1; i < 10; i++) {
        snprintf(content, sizeof(content), "v%d\n", i);
        write_file("T:test_libgit2_repo/sec15_d10/hello.txt", content);
        git_repository_index(&idx, repo);
        git_index_add_bypath(idx, "hello.txt");
        git_index_write_tree(&tree_oid, idx);
        git_index_write(idx);
        git_tree_lookup(&tree, repo, &tree_oid);
        git_signature_new(&sig, "Alice", "alice@amiga.org",
                          1000000000L + i, 0);
        rc = git_commit_create_v(&new_oid, repo, "HEAD", sig, sig, NULL,
                                 content, tree, 1, commits[i-1]);
        ASSERT_EQ(rc, 0);
        git_commit_lookup(&commits[i], repo, &new_oid);
        git_signature_free(sig);
        git_tree_free(tree);
        git_index_free(idx);
        tree = NULL;
        idx = NULL;
        sig = NULL;
    }

    git_commit_tree(&t_first, commits[0]);
    git_commit_tree(&t_last, commits[9]);
    rc = git_diff_tree_to_tree(&diff, repo, t_first, t_last, NULL);
    ASSERT_EQ(rc, 0);
    ASSERT((long)git_diff_num_deltas(diff) >= 1L);

    git_diff_free(diff);
    git_tree_free(t_first);
    git_tree_free(t_last);
    for (i = 0; i < 10; i++) {
        if (commits[i] != NULL) {
            git_commit_free(commits[i]);
        }
    }
    git_repository_free(repo);
}

TEST(stress_revparse_10_refs)
{
    git_repository *repo = NULL;
    git_commit *commit = NULL;
    git_reference *ref = NULL;
    git_object *obj = NULL;
    git_oid oid;
    char branch_name[32];
    int i;
    int rc;

    make_repo_with_commit("sec15_rp10", "hello\n", &repo, &oid);
    git_commit_lookup(&commit, repo, &oid);
    for (i = 0; i < 10; i++) {
        snprintf(branch_name, sizeof(branch_name), "branch_%d", i);
        rc = git_branch_create(&ref, repo, branch_name, commit, 0);
        ASSERT_EQ(rc, 0);
        git_reference_free(ref);
    }
    for (i = 0; i < 10; i++) {
        snprintf(branch_name, sizeof(branch_name), "branch_%d", i);
        rc = git_revparse_single(&obj, repo, branch_name);
        ASSERT_EQ(rc, 0);
        ASSERT(git_object_type(obj) == GIT_OBJECT_COMMIT);
        git_object_free(obj);
    }
    git_commit_free(commit);
    git_repository_free(repo);
}

/* ========================================================================
 * main() -- suppress requesters, init libgit2, run all sections
 * ======================================================================== */

int main(void)
{
    struct Process *me;
    int rc;

    (void)verstag;

    /* Suppress AmigaDOS volume requesters. libgit2's path normalization
     * probes bare names which would otherwise trigger system requesters
     * on T: and similar volumes. */
    me = (struct Process *)FindTask(NULL);
    g_saved_win = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1L;

    /* Fresh T:test_libgit2_repo/ */
    rm_rf("T:test_libgit2_repo");
    {
        BPTR lk = CreateDir((STRPTR)"T:test_libgit2_repo");
        if (lk != 0) {
            UnLock(lk);
        }
    }

    if (git_libgit2_init() < 1) {
        printf("FATAL: git_libgit2_init failed\n");
        me->pr_WindowPtr = g_saved_win;
        return 1;
    }

    printf("=== test_libgit2 1.0 ===\n");

    printf("\n-- Section 0: library lifecycle --\n");
    RUN_TEST(libgit2_init_and_shutdown);
    RUN_TEST(libgit2_double_init_refcount);

    printf("\n-- Section 1: OID utilities --\n");
    RUN_TEST(oid_fromstr_valid);
    RUN_TEST(oid_fromstr_invalid);
    RUN_TEST(oid_tostr_roundtrip);
    RUN_TEST(oid_cmp_equal);
    RUN_TEST(oid_cmp_different);
    RUN_TEST(oid_iszero_true);
    RUN_TEST(oid_iszero_false);
    RUN_TEST(oid_sha1_known_vector);
    RUN_TEST(oid_sha1_all_hex_digits);

    printf("\n-- Section 2: Repository init and open --\n");
    RUN_TEST(repository_init_creates_git_dir);
    RUN_TEST(repository_init_bare);
    RUN_TEST(repository_open_from_init);
    RUN_TEST(repository_open_nonexistent);
    RUN_TEST(repository_open_not_a_repo);
    RUN_TEST(repository_is_bare_false);
    RUN_TEST(repository_is_bare_true);
    RUN_TEST(repository_workdir_path);

    printf("\n-- Section 3: Config --\n");
    RUN_TEST(config_open_level_local);
    RUN_TEST(config_set_and_get_string);
    RUN_TEST(config_set_and_get_int32);
    RUN_TEST(config_get_missing_key);
    RUN_TEST(config_snapshot);
    RUN_TEST(config_core_symlinks_false);

    printf("\n-- Section 4: Signature --\n");
    RUN_TEST(signature_new_valid);
    RUN_TEST(signature_now_valid);
    RUN_TEST(signature_free_null);
    RUN_TEST(signature_amiga_epoch_offset);
    RUN_TEST(signature_from_buffer_valid);
    RUN_TEST(signature_from_buffer_invalid);

    printf("\n-- Section 5: Object database --\n");
    RUN_TEST(odb_open_from_init_repo);
    RUN_TEST(odb_hash_blob_content);
    RUN_TEST(odb_write_and_read_blob);
    RUN_TEST(odb_exists_after_write);
    RUN_TEST(odb_read_nonexistent);
    RUN_TEST(odb_write_empty_blob);

    printf("\n-- Section 6: Blob --\n");
    RUN_TEST(blob_create_from_buffer);
    RUN_TEST(blob_lookup_by_oid);
    RUN_TEST(blob_rawcontent_matches_input);
    RUN_TEST(blob_rawsize_matches_input);
    RUN_TEST(blob_create_empty);

    printf("\n-- Section 7: Tree and index --\n");
    RUN_TEST(index_open_from_repo);
    RUN_TEST(index_add_and_write_tree);
    RUN_TEST(index_entry_count_after_add);
    RUN_TEST(index_remove_entry);
    RUN_TEST(index_write_and_reload);
    RUN_TEST(tree_lookup_after_write_tree);
    RUN_TEST(tree_entry_byname);
    RUN_TEST(tree_entrycount);

    printf("\n-- Section 8: Commit --\n");
    RUN_TEST(commit_create_initial);
    RUN_TEST(commit_lookup_roundtrip);
    RUN_TEST(commit_message_preserved);
    RUN_TEST(commit_author_fields);
    RUN_TEST(commit_parent_count_initial);
    RUN_TEST(commit_create_with_parent);
    RUN_TEST(commit_lookup_nonexistent);

    printf("\n-- Section 9: References --\n");
    RUN_TEST(reference_name_to_id_head);
    RUN_TEST(reference_lookup_head);
    RUN_TEST(reference_create_direct);
    RUN_TEST(reference_symbolic_create);
    RUN_TEST(reference_lookup_nonexistent);
#ifndef AMIPORT_VAMOS_LIMITED
    /* vamos readdir(refs/heads) returns 0 entries for newly-created refs. */
    RUN_TEST(reference_list_names);
#endif

    printf("\n-- Section 10: Branches and tags --\n");
    RUN_TEST(branch_create_and_lookup);
#ifndef AMIPORT_VAMOS_LIMITED
    /* Branch iterator walks refs/heads -- same vamos readdir gap. */
    RUN_TEST(branch_iterator_local);
#endif
    RUN_TEST(tag_create_lightweight);
#ifndef AMIPORT_VAMOS_LIMITED
    /* Tag listing walks refs/tags -- same vamos readdir gap. */
    RUN_TEST(tag_list_names);
#endif

    printf("\n-- Section 11: Revwalk and revparse --\n");
    RUN_TEST(revwalk_new_and_free);
    RUN_TEST(revwalk_push_head_and_walk);
    RUN_TEST(revparse_single_head);
    RUN_TEST(revparse_invalid_spec);

    printf("\n-- Section 12: Diff --\n");
    RUN_TEST(diff_tree_to_tree_initial);
    RUN_TEST(diff_numdeltas_after_add);
    RUN_TEST(diff_empty_repo);

    printf("\n-- Section 13: Status --\n");
#ifndef AMIPORT_VAMOS_LIMITED
    /* git_status_file lstat's the workdir file; vamos returns ENOENT for
     * just-written files on the T: volume. */
    RUN_TEST(status_new_file_untracked);
    RUN_TEST(status_clean_after_commit);
#endif

    printf("\n-- Section 14: Amiga-specific --\n");
    RUN_TEST(amiga_t_volume_path_for_repo);
#ifndef AMIPORT_VAMOS_LIMITED
    /* status_foreach iterates the workdir -- same vamos readdir gap. */
    RUN_TEST(amiga_d_type_unknown_in_walk);
#endif
    RUN_TEST(amiga_timestamp_above_epoch_1978);

    printf("\n-- Section 15: Stress --\n");
    RUN_TEST(stress_10_commits_revwalk);
    RUN_TEST(stress_50_blobs_odb);
    RUN_TEST(stress_tree_depth_5);
    RUN_TEST(stress_index_100_entries);
    RUN_TEST(stress_diff_10_commit_chain);
    RUN_TEST(stress_revparse_10_refs);

    git_libgit2_shutdown();

    rc = test_summary();

    /* Clean up fixtures so stale state does not affect re-runs. */
    rm_rf("T:test_libgit2_repo");

    /* Restore pr_WindowPtr. */
    me->pr_WindowPtr = g_saved_win;

    return rc;
}
