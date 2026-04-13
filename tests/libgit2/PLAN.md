# tests/libgit2/ — Test Plan (Stage 4 output)

**Status:** Design complete, implementation pending.
**Origin:** `test-designer` agent dispatch, 2026-04-13, PDR-010 Phase 2 Stage 4.
**Implements:** `.claude/rules/library-pipeline.md` Stage 4 (test-designer, library mode).
**Consumes:** `lib/libgit2/libgit2.a` (1.37 MB, 162 object files, commit 0068590).
**Next step:** Stage 5 — implement `test_libgit2.c` and `Makefile` per this plan, run via vamos with `test-runner` agent, verify 65/65 pass before proceeding to Stage 6 (memory-checker).

---

## Overview

- **Total test count:** 65
- **Categories:** Functional 28, Error-path 14, Edge 10, Amiga-specific 7, Stress 6
- **Estimated binary size:** 180-220 KB after link (selective pull from 1.37 MB `libgit2.a`)
- **Estimated vamos runtime:** 90-150 seconds
- **Link order:** `-lgit2 -lz -lamiport` (libgit2 depends on libz and libamiport)

---

## Files to create

1. `tests/libgit2/test_libgit2.c` — single file, ~950-1100 lines
2. `tests/libgit2/Makefile` — ~40 lines

No pre-built fixture files. All fixtures built at runtime via libgit2 itself under `T:test_libgit2_repo/`.

---

## Makefile

```makefile
TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc

# -O0 mandatory: matches libgit2.a build flags (crash-patterns #16).
# -std=gnu99: libgit2 headers use C99 (bool, designated initializers).
# -m68000: vamos emulates 68000; 68020 instructions cause ALERT.
# -include amigaos_compat.h: reproduces libgit2.a's force-include so the
#   test TU sees the same macro namespace. Required even though test_libgit2.c
#   does not directly call pread/realpath/etc -- the -include must resolve
#   at link time for the libgit2 symbols to bind correctly.
CFLAGS = -std=gnu99 -O0 -noixemul -m68000 -Wall \
         -I../../lib/libgit2/include \
         -I../../lib/libgit2/src/util \
         -I../../lib/posix-shim/include \
         -include ../../lib/libgit2/src/util/amigaos_compat.h \
         -I../shim

# Link order is critical: libgit2 -> libz -> libamiport -> libnix (implicit).
LDFLAGS = -L../../lib/libgit2 -lgit2 \
          -L../../lib/zlib -lz \
          -L../../lib/posix-shim -lamiport

# vamos default 8 KB stack is insufficient for libgit2's deeply recursive
# tree/pack walk code. 256 KB passed via -s. Real AmigaOS reads __stack
# cookie (declared in test_libgit2.c); vamos uses this flag.
VAMOS_STACK = 256
VAMOS = vamos -q -s $(VAMOS_STACK)

LIBGIT2_DIR = ../../lib/libgit2
ZLIB_DIR    = ../../lib/zlib
SHIM_DIR    = ../../lib/posix-shim

.PHONY: all build run clean

all: build run

build: $(LIBGIT2_DIR)/libgit2.a $(ZLIB_DIR)/libz.a $(SHIM_DIR)/libamiport.a test_libgit2

$(LIBGIT2_DIR)/libgit2.a:
	$(MAKE) -C $(LIBGIT2_DIR)

$(ZLIB_DIR)/libz.a:
	$(MAKE) -C $(ZLIB_DIR)

$(SHIM_DIR)/libamiport.a:
	$(MAKE) -C $(SHIM_DIR)

test_libgit2: test_libgit2.c ../shim/test_framework.h
	$(CC) $(CFLAGS) -o $@ test_libgit2.c $(LDFLAGS)

run: test_libgit2
	$(VAMOS) ./test_libgit2

clean:
	rm -f test_libgit2
```

---

## test_libgit2.c section layout

```
Section 0:  Library lifecycle                (2 tests)
Section 1:  OID utilities                    (9 tests, 1 AMIGA)
Section 2:  Repository init and open         (8 tests, 2 ERROR)
Section 3:  Config                           (6 tests, 1 AMIGA, 1 ERROR)
Section 4:  Signature                        (6 tests, 1 AMIGA, 1 ERROR, 1 EDGE)
Section 5:  Object database (odb loose)      (6 tests, 1 ERROR, 1 EDGE)
Section 6:  Blob                             (5 tests, 1 EDGE)
Section 7:  Tree and index                   (8 tests)
Section 8:  Commit                           (7 tests, 1 ERROR, 1 EDGE)
Section 9:  References                       (6 tests, 1 ERROR)
Section 10: Branches and Tags                (4 tests)
Section 11: Revision walk and revparse       (4 tests, 1 ERROR)
Section 12: Diff                             (3 tests, 1 EDGE)
Section 13: Status                           (2 tests)
Section 14: Amiga-specific                   (3 tests, [AMIGA])
Section 15: Stress                           (6 tests, [STRESS])
```

Total: 65 tests.

---

## Test list with intent

### Section 0: Library lifecycle

- **libgit2_init_and_shutdown** — `git_libgit2_init()` then `shutdown()`. Must be test #0.
- **libgit2_double_init_refcount** — init twice, shutdown twice, verify refcount balance.

### Section 1: OID utilities

- **oid_fromstr_valid** — parse zero oid; verify `git_oid_iszero`.
- **oid_fromstr_invalid** — short string, non-hex string; must return non-zero.
- **oid_tostr_roundtrip** — parse → tostr → `ASSERT_STR_EQ`. Use SHA1 of empty string.
- **oid_cmp_equal** — two oids from same hex; `cmp == 0`.
- **oid_cmp_different** — two oids from different hex; `cmp != 0`.
- **oid_iszero_true** / **oid_iszero_false** — boundary.
- **oid_sha1_known_vector** [AMIGA] — parse `da39a3ee5e6b4b0d3255bfef95601890afd80709`, `ASSERT_EQ((int)oid.id[0], 0xda)`. Catches SHA1 endian confusion on 68k.

### Section 2: Repository init and open

All under `T:test_libgit2_repo/` subpaths. Each test uses a unique subdir.

- **repository_init_creates_git_dir** — non-bare init then open.
- **repository_init_bare** — bare=1, `is_bare() != 0`.
- **repository_open_from_init** — round-trip.
- **repository_open_nonexistent** [ERROR] — `GIT_ENOTFOUND` (-3).
- **repository_open_not_a_repo** [ERROR] — plain dir, not a git repo.
- **repository_is_bare_false** / **repository_is_bare_true** — sanity.
- **repository_workdir_path** — non-bare returns non-NULL, bare returns NULL.

### Section 3: Config

- **config_open_level_local** — `git_repository_config(&cfg, repo)`.
- **config_set_and_get_string** — user.name round-trip via `git_config_get_entry` + `git_config_entry_free`.
- **config_set_and_get_int32** — core.compression = 6, read back.
- **config_get_missing_key** [ERROR] — `GIT_ENOTFOUND`.
- **config_snapshot** — `git_config_snapshot` returns 0.
- **config_core_symlinks_false** [AMIGA] — libgit2 must set this to false on non-Unix-symlink platforms.

### Section 4: Signature

- **signature_new_valid** — explicit time=0, offset=0.
- **signature_now_valid** — `git_signature_now`, `sig->when.time > 0`.
- **signature_free_null** [EDGE] — `git_signature_free(NULL)` no-crash.
- **signature_amiga_epoch_offset** [AMIGA] — `time=252460800` (Unix time for 1978-01-01), assert stored verbatim. Guards against Amiga epoch offset leaking into libgit2's stored time.
- **signature_from_buffer_valid** — parse `"Alice <alice@amiga.org> 1000000000 +0000"`.
- **signature_from_buffer_invalid** [ERROR] — garbage string.

### Section 5: Object database (odb loose)

- **odb_open_from_init_repo** — `git_repository_odb`.
- **odb_hash_blob_content** — SHA1 of `"hello\n"` = `8b137891791fe96927ad78e64b0aad7bded08bdc`. `ASSERT_EQ((int)oid.id[0], 0x8b)`. Validates big-endian SHA1 via `SHA1DC_FORCE_BIGENDIAN`.
- **odb_write_and_read_blob** — 10-byte buffer round-trip via `memcmp`.
- **odb_exists_after_write** — `git_odb_exists` returns non-zero for written, zero for unknown.
- **odb_read_nonexistent** [ERROR] — all-`0xFF` oid → `GIT_ENOTFOUND`.
- **odb_write_empty_blob** [EDGE] — zero-byte content, size 0 on read-back.

### Section 6: Blob

- **blob_create_from_buffer** — `"test content"` (12 bytes).
- **blob_lookup_by_oid** — round-trip.
- **blob_rawcontent_matches_input** — `memcmp`.
- **blob_rawsize_matches_input** — `ASSERT_EQ(12L)`.
- **blob_create_empty** [EDGE] — SHA1 = `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.

### Section 7: Tree and index

- **index_open_from_repo** — `git_repository_index`.
- **index_add_and_write_tree** — create file at `T:test_libgit2_repo/repo_tree/test.txt` via `fopen/fwrite/fclose`, then `git_index_add_bypath` + `git_index_write_tree`.
- **index_entry_count_after_add** — 1 entry.
- **index_remove_entry** — remove, count 0.
- **index_write_and_reload** — `git_index_write` then `git_index_read(idx, 1)`.
- **tree_lookup_after_write_tree** — look up the written tree.
- **tree_entry_byname** — "test.txt" entry exists.
- **tree_entrycount** — 1 entry.

### Section 8: Commit

Shared helper: `static int make_repo_with_commit(const char *subpath, git_repository **out, git_oid *out_oid)`.

- **commit_create_initial** — `git_commit_create_v` with 0 parents.
- **commit_lookup_roundtrip** — `git_commit_id` matches.
- **commit_message_preserved** — `ASSERT_STR_EQ("Initial commit\n", ...)`. Note the trailing `\n` — libgit2 appends if missing.
- **commit_author_fields** — name and email preserved.
- **commit_parent_count_initial** [EDGE] — 0 parents.
- **commit_create_with_parent** — 1 parent, `git_commit_parent_id` matches.
- **commit_lookup_nonexistent** [ERROR] — all-`0xAA` oid → `GIT_ENOTFOUND`.

### Section 9: References

- **reference_name_to_id_head** — after first commit, HEAD resolves to a non-zero oid.
- **reference_lookup_head** — non-NULL, type non-zero.
- **reference_create_direct** — create `refs/heads/testbranch` pointing at HEAD oid.
- **reference_symbolic_create** — `refs/symtest` → `refs/heads/master`, type is `GIT_REFERENCE_SYMBOLIC`.
- **reference_lookup_nonexistent** [ERROR] — `GIT_ENOTFOUND`.
- **reference_list_names** — `git_strarray` with at least 1 entry. Dispose.

### Section 10: Branches and tags

- **branch_create_and_lookup** — `git_branch_create` → `git_branch_lookup`.
- **branch_iterator_local** — `git_branch_iterator_new` + `git_branch_next` until `GIT_ITEROVER`, count >= 1.
- **tag_create_lightweight** — `git_tag_create_lightweight("v0.1", ...)`.
- **tag_list_names** — at least 1, name matches "v0.1".

### Section 11: Revwalk and revparse

- **revwalk_new_and_free** — non-null walker, free without crash.
- **revwalk_push_head_and_walk** — 2 commits, walk to `GIT_ITEROVER`, count = 2, first oid = HEAD.
- **revparse_single_head** — `git_revparse_single(..., "HEAD")` → `GIT_OBJECT_COMMIT`.
- **revparse_invalid_spec** [ERROR] — `"no_such_ref_xyz"` → non-zero rc.

### Section 12: Diff

- **diff_tree_to_tree_initial** — two consecutive commits, `git_diff_num_deltas >= 1`.
- **diff_numdeltas_after_add** — `git_diff_tree_to_index(NULL, idx)` with one staged file.
- **diff_empty_repo** [EDGE] — no commits, no staged files, `num_deltas == 0`.

### Section 13: Status

- **status_new_file_untracked** — `git_status_file(&flags, ...)` with `GIT_STATUS_WT_NEW` bit set.
- **status_clean_after_commit** — `flags == 0` (== `GIT_STATUS_CURRENT`).

### Section 14: Amiga-specific

- **amiga_t_volume_path_for_repo** [AMIGA] — `git_repository_init("T:test_libgit2_repo/amiga_vol/", 0)`. Smoke test for `fs_path.c` handling of `T:` prefix and `:` character.
- **amiga_d_type_unknown_in_walk** [AMIGA] — init repo with subdir, add file inside, `git_status_foreach` fires for the nested file. Validates tree iterator does not short-circuit on `DT_UNKNOWN` (known-pitfalls entry).
- **amiga_timestamp_above_epoch_1978** [AMIGA] — `git_commit_author(c)->when.time > 252460800L`. Guards against the `amiport_gettimeofday` shim returning Amiga-epoch-relative values instead of Unix-relative.

### Section 15: Stress

Each stress test uses `T:test_libgit2_repo/stress_X/` sub-path and cleans up itself at end.

- **stress_10_commits_revwalk** — 10 sequential commits, revwalk counts 10.
- **stress_50_blobs_odb** — 50 distinct 64-byte blobs, all OIDs unique.
- **stress_tree_depth_5** — 5 levels of subdirs, `git_tree_walk` callback fires >= 5 times.
- **stress_index_100_entries** — 100 files staged (names `a0`..`z3`), write/read round-trip.
- **stress_diff_10_commit_chain** — 10 commits modifying one file, `git_diff_tree_to_tree` between #1 and #10 returns 1 delta.
- **stress_revparse_10_refs** — 10 branches, revparse each by name, all resolve to commits.

---

## Tests intentionally omitted

| API | Reason |
|---|---|
| `git_remote_*`, `git_clone_*`, `git_fetch_*`, `git_push_*` | Transports excluded; would link-fail. |
| `git_transport_*` | Stub headers only. |
| `git_submodule_update` | Uses `git_clone__submodule` (stub header, no impl). |
| `git_rebase_*`, `git_stash_*` | High stack depth; defer to Phase 4 real-hardware tests. |
| `git_apply_*`, `git_patch_*` | Complex fixture setup; implicit via diff tests. |
| `git_blame_*` | High memory; defer to Phase 4. |
| `git_worktree_*` | Requires symlinks (disabled on AmigaOS). |
| `git_checkout_*`, `git_merge_*` | Need workdir sync + multi-commit history; Phase 4. |
| `git_mailmap_*`, `git_note_*`, `git_hashsig_*`, `git_filter_*`, `git_describe_*` | Not in amigit Phase 3 CLI scope. |

---

## Ordering

Run tests in the order the sections are listed above — later sections build on earlier ones functionally. If Section 0 or 1 fails, halt the suite (further tests produce no useful diagnostic).

---

## Risks

1. **Stack depth on vamos.** libgit2 tree walk and pack reader have deeply recursive frames. Each `git_tree_walk` level can consume 8-16 KB. With `VAMOS_STACK=256` (256 KB) and 4 KB AmigaOS hidden overhead per dos.library call, stress tests recursing 10+ levels may hit the ceiling. `stress_tree_depth_5` is capped at 5.
2. **`pr_WindowPtr` volume requesters.** libgit2 does path normalization that calls `Access`/`Lock` on bare names, triggering "please insert volume" requesters on AmigaOS. The test `main()` MUST set `pr_WindowPtr = -1` at startup as a guard, and all test paths MUST use the `T:` prefix. See known-pitfalls "AmigaDOS Volume Requester".
3. **`core.symlinks` auto-detection.** libgit2 may probe symlink support via `amiport_symlink`. The shim must fail cleanly so libgit2 sets `core.symlinks=false` correctly. `config_core_symlinks_false` detects regressions.
4. **ODB loose file path length.** Keep test repo subdirs short; AmigaOS FFS has a 107-char path limit.
5. **Index atomic rename.** `git_index_write` uses `.lock` + `rename`. The shim's `amiport_rename` must work; `index_write_and_reload` catches regressions.
6. **Timestamp precision.** FFS has 2-second resolution. Do not assert nanosecond precision.
7. **Link time.** 30-60 s for a clean build through Docker bebbo-gcc.

---

## `main()` requirements

```c
long __stack = 262144;  /* Real AmigaOS reads this; vamos uses -s 256. */

static const char *verstag = "$VER: test_libgit2 1.0 (13.04.2026)";

int main(void) {
    (void)verstag;

    /* Suppress AmigaDOS volume requesters globally for this process.
     * libgit2's path normalization probes bare names, which would
     * otherwise trigger "please insert volume" requesters. */
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR saved_win = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1L;

    /* Fresh T:test_libgit2_repo/ */
    rm_rf("T:test_libgit2_repo");
    amiport_mkdir("T:test_libgit2_repo", 0755);

    /* libgit2 init must happen before any git_* call */
    if (git_libgit2_init() < 1) {
        printf("FATAL: git_libgit2_init failed\n");
        return 1;
    }

    printf("=== test_libgit2 1.0 ===\n");

    /* Run all sections in order */
    printf("\n-- Section 0: library lifecycle --\n");
    RUN_TEST(libgit2_init_and_shutdown);
    RUN_TEST(libgit2_double_init_refcount);
    /* ... section 1 through 15 ... */

    git_libgit2_shutdown();

    /* Restore pr_WindowPtr */
    me->pr_WindowPtr = saved_win;

    /* Clean up T:test_libgit2_repo/ so stale state doesn't affect re-runs */
    rm_rf("T:test_libgit2_repo");

    return test_summary();
}
```

A `static void rm_rf(const char *path)` helper is required; implement via `opendir`/`readdir`/`unlink`/`rmdir` recursively.

---

## Learnings surfaced by test-designer (for /capture-learning routing)

- **[PITFALL]** `git_libgit2_init()` / `git_libgit2_shutdown()` refcount must be strictly balanced even with `GIT_THREADS=0`. Partial init state breaks silently.
- **[PITFALL]** `git_commit_create_v` appends `\n` to commit messages. Any `ASSERT_STR_EQ` on `git_commit_message()` must include the trailing newline.
- **[PROCESS]** Library unit-test Makefiles must **replicate the force-include flag** from the library's own Makefile (`-include amigaos_compat.h` in our case) so the test TU compiles against the same macro namespace the library was built with. This is not obvious from the zlib reference test (zlib has no force-include). Candidate addition to `.claude/rules/library-pipeline.md` Stage 5.
- **[PITFALL]** The `d_type=DT_UNKNOWN` pitfall (known-pitfalls) applies to libgit2's workdir iterator in `src/libgit2/iterator.c`. libgit2 has a `lstat`-based fallback, but only if `amiport_lstat` correctly distinguishes directories from files. If lstat is broken, `git_status_foreach` silently misses entries.
- **[PROCESS]** Stress tests should own their own teardown (cleanup at end of TEST body) rather than rely on a global `main()` cleanup. If a stress test Guru-crashes vamos, stale `T:` state breaks re-runs.

---

## Estimated implementation effort

- `test_libgit2.c`: 950-1100 lines (65 tests + helpers + main)
- `tests/libgit2/Makefile`: ~40 lines
- Debug iterations expected: 3-8 (linker errors, vamos stack issues, missing functions, timestamp assumptions)
- Total Stage 5 budget: comparable to implementing a medium-complexity Category 2 port

Implementation is NOT dispatched to an agent — it is main-session work following this plan. After the C is written and the Makefile compiles, dispatch `test-runner` to run the suite via vamos and report pass/fail per test.
