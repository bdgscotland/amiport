---
name: port_basename_perf
description: Performance findings for ports/basename 1.14 — trivial Cat 1 CLI, no hot paths, clean bill of health reviewed 2026-03-25
type: project
---

ports/basename 1.14 reviewed 2026-03-25.

**Verdict:** No CRITICAL or HIGH findings. No hot paths exist — run-once, single string operation.

- basename() from libnix is used directly (no shim override). libnix basename() is a correct POSIX implementation. No AmigaOS path translation concern since the program receives a user-supplied string argument, not a real filesystem path.
- dirname() is NOT called (correctly — this is basename, not dirname).
- strlen() called twice on short argv strings (line 111-112) — negligible.
- strcmp() called once on short strings — negligible.
- __stack = 16384: adequate. No local arrays, no recursion, no deep call chain.
- No I/O loops — single puts() call.
- No CHIP RAM pressure — no allocations after argv expand.
- Overall: I/O bound on process startup cost, not CPU.

**Why:** diagnose category 1, trivial, no hotpaths, clean.
