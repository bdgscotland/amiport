# Memory Safety Audit Index

## Completed Audits

- [memory-audit-tail.md](memory-audit-tail.md) - ports/tail memory safety review (2026-03-21)
  - Status: CRITICAL LEAKS FOUND
  - Issues: 2 critical, 1 warning
  - Verdict: Unfixable without code changes
  - tf array never freed (144+ bytes/invocation), obsolete() strings leaked
  - Cannot ship to Aminet without fixes

- [memory-audit-grep.md](memory-audit-grep.md) - ports/grep memory safety review (2026-03-21)
  - Status: CRITICAL LEAKS FOUND
  - Issues: 6 critical, 1 warning
  - Verdict: Unfixable without code changes

- [memory-audit-wc.md](memory-audit-wc.md) - ports/wc memory safety review (2026-03-22)
  - Status: CLEAN
  - Verdict: Approved for shipping — no leaks detected
  - Static buf allocation idempotent, atexit cleanup covers all paths

- [memory-audit-uniq.md](memory-audit-uniq.md) - ports/uniq memory safety review, re-audit after partial fixes (2026-03-22)
  - Status: FAIL — 1 CRITICAL LEAK REMAINING
  - Issues: 1 critical (prevline leak on line 229 error path)
  - Verdict: Partial fixes worked for obsolete() and argv, but missed local variable cleanup
  - Fix required: 1 line (free prevline before err at line 229)
  - Root cause: atexit cleanup cannot handle stack-local variables, must be freed inline

- [memory-audit-patch.md](memory-audit-patch.md) - ports/patch v1.78 (OpenBSD) initial memory safety review (2026-03-23)
  - Status: LEAKS FOUND — 2 CRITICAL (FIXED)
  - Issues: asprintf() temp filename leak on error, unclosed ttyfd file descriptor
  - Verdict: Unfixable without code changes
  - Fix required: Add free() calls in my_cleanup() for TMP* names, close ttyfd in my_cleanup()
  - Leaks: Rare asprintf() failure (~16-64 bytes), permanent ttyfd per invocation (resource exhaustion)

- [memory-audit-patch-recheck.md](memory-audit-patch-recheck.md) - ports/patch v1.78 re-audit after fixes (2026-03-23)
  - Status: CLEAN — All leaks fixed
  - Issues: 2 critical — both RESOLVED
  - Verdict: Approved for shipping — all allocations properly paired
  - TMP* name strings now freed in my_cleanup() (lines 442-446)
  - ttyfd now closed in my_cleanup() (lines 449-452)
  - No new issues introduced by getopt.h swap, tab stop fix, or dump_line optimization

- [memory-audit-jq-recheck.md](memory-audit-jq-recheck.md) - ports/jq 1.7.1 re-audit after partial fixes (2026-03-23)
  - Status: CRITICAL LEAKS REMAINING — 5 unfixed paths
  - Verdict: Cannot ship — fixes incomplete
  - Previous die() fix correct (9 paths now use goto out)
  - jv_mem_realloc() intermediate pointer fix correct
  - But 5 NEW paths found bypassing cleanup: lib_search_paths leak, exit(10) at 699, exit(20) at 720, usage() at 714, unchecked strdup at linker.c:370
  - Root cause: usage() hard exit and strdup OOM checks not converted to goto out
  - All 5 fixes required before shipping

- [memory-audit-bc.md](memory-audit-bc.md) - ports/bc 1.07.1 memory safety review (2026-03-23)
  - Status: CRITICAL LEAKS FOUND
  - Issues: 4 critical (strdup env vars, math lib names, amiport_getenv results, global arrays)
  - Verdict: Cannot ship without fixes
  - Leaks: env_argv[0] never freed, 6× math lib strdups, 3× amiport_getenv() malloc'd strings, storage arrays never freed, global constants never freed
  - Impact: ~1.6 KB permanent memory loss per invocation on AmigaOS
  - Root cause: Ported for Unix where process exit auto-frees; amiport_getenv() ABI mismatch (returns malloc'd strings, not static storage)
  - Fix complexity: Medium — add atexit cleanup pattern, track getenv results, requires main.c and bc_exit() changes

- [memory-audit-bc-recheck.md](memory-audit-bc-recheck.md) - ports/bc 1.07.1 re-audit after partial fixes (2026-03-23)
  - Status: PARTIAL FIXES APPLIED — 3 of 8 issues REMAIN UNFIXED
  - Issues: 4 fixed, 4 remaining critical
  - Verdict: Still cannot ship — 5 fixes still needed
  - Fixes applied: BC_ENV_ARGS tracking (env_allocs[]), strdup("BC_ENV_ARGS") freed, POSIXLY_CORRECT freed, BC_LINE_LENGTH freed ✓
  - Fixes still needed: Math lib strdups leak, f_names/v_names/a_names individual entries leak, function bodies not freed, global constants (_zero_/_one_/_two_) not freed, _bc_Free_list cache not freed
  - Remaining leak: 150+ bytes per invocation, up to 1+ KB for complex programs
