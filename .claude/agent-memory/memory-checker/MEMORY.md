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
