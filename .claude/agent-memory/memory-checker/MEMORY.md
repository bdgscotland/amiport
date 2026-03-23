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
