# Batch Completion Checklist — MANDATORY BEFORE COMMIT

## The Rule

**After ALL ports in a batch pass FS-UAE testing, you MUST complete EVERY step below IN ORDER before committing. No exceptions. No "I'll do it later." No partial commits.**

This is a BLOCKING checklist. Do not `git add` or `git commit` until every box is checked.

## The Checklist

### Post-FS-UAE (BEFORE any commit)

- [ ] **Memory-checker dispatched AND fixes applied** — Not just "dispatched." The fixes must be in the source code, rebuilt, and re-tested on FS-UAE.
- [ ] **Perf-optimizer dispatched AND HIGH/CRITICAL findings applied** — Not just "reported." The code changes must be in the source, rebuilt, and re-tested on FS-UAE.
- [ ] **REVISION bumped** if source changed after initial build — Update Makefile REVISION, $VER string, .readme Version field. All must match.
- [ ] **Binaries rebuilt** with all fixes applied — The committed binary must be the FINAL version with all memory + perf fixes.
- [ ] **LHA packages rebuilt** — `make package TARGET=ports/<name>` for every port. Stale LHA from initial build is wrong.
- [ ] **PORT.md enriched** — >60 lines, no "To be filled" placeholders, includes portability table, transformation table, test results, memory safety, perf notes, known limitations.
- [ ] **catalog.json updated** — measured_binary_kb matches actual final binary, test counts correct.
- [ ] **site/data/catalog.json synced** — `cp data/catalog.json site/data/catalog.json`
- [ ] **Per-port package JSON updated** — site/data/packages/<name>.json has correct binary size, version, revision.
- [ ] **PORTS.md updated** — Version shows revision if >1 (e.g., "1.22-2").

### Then Commit

- [ ] **Single commit** with all 10 ports fully complete — not 5 partial commits spread across the session.

## Why This Rule Exists

During the 2026-03-26 10-port batch, the pipeline was "completed" 5 separate times:
1. First commit: 45-line stub PORT.md files (user caught it)
2. Second commit: PORT.md enriched but memory fixes not applied (user caught it)
3. Third commit: Memory fixes applied but perf optimizations not applied (user caught it)
4. Fourth commit: Perf optimizations applied but REVISION not bumped, LHA not rebuilt
5. Still going...

Each time Claude declared "done" and the user had to push for the next step. The `/batch-port-parallel` skill documents all these steps but they were executed piecemeal across hours instead of as one atomic sequence.

## How to Apply

After the last FS-UAE test passes, mentally switch to "completion mode." Do not type `git commit` until you have walked through every checkbox above and verified each one. Read this checklist out loud if needed. The cost of doing it right once is 30 minutes. The cost of doing it wrong is the user's trust.
