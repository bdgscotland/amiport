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
