# PDR-011: Per-revision change notes for ports

## Status

Proposed (Draft, not yet accepted)

## Date

2026-04-14

## Problem

Ports have two version components: upstream `VERSION` and port `REVISION`. When a port is re-published with the same upstream version but a bumped REVISION (e.g., `jq 1.7.1-3` after `jq 1.7.1-2`), there is currently no way for a user to know what changed. Specifically:

- 13 packages on the live site already have `revision > 1`, including `jq` at rev 3 and `sed` at rev 3
- The only narrative field per package is `porting_notes`, which is a one-time porting overview, not a per-revision change log
- Per CLAUDE.md, REVISION bumps when "ported/, Makefile, shim deps, or tests change but upstream version stays the same" — so something *did* change every time, but the change is invisible to anyone reading the site
- The Aminet `Replaces:` mechanism communicates "this replaces an older upload" but does not carry change notes
- Subscribers to the RSS feed see the same package title twice (once at rev 2, once at rev 3) with no indication of why
- A user trying to decide whether to upgrade has no information to base the decision on

This is a trust gap. The whole point of bumping a revision instead of silently re-publishing is to communicate that *something is different*. Right now we communicate "different" without ever saying "what."

## Target Users

- **Amiga end users** deciding whether to download an upgrade. "Should I bother re-installing jq? What changed?" Today: no answer.
- **Bug reporters** who file an issue against `jq 1.7.1-2` and want to know if it was fixed in `1.7.1-3`. Today: they have to read git history or test the binary.
- **Curious watchers** following the porting effort via RSS who want to see real progress, not opaque version bumps.

## Decision

Add a `revisions` array to the per-package JSON schema. Capture per-revision summaries in `PORT.md` at the moment a revision is bumped. Surface them on the package detail view, the changelog page, and the RSS feed.

### Schema addition

`site/data/packages/<name>.json`:

```json
{
  "name": "jq",
  "version": "1.7.1",
  "revision": 3,
  "revisions": [
    {
      "revision": 3,
      "date": "2026-04-12",
      "summary": "Re-link against oniguruma 6.9.9 to fix a regex test that crashed on FS-UAE.",
      "kind": ["dep", "test"]
    },
    {
      "revision": 2,
      "date": "2026-04-05",
      "summary": "Drop -O1 hot-path optimization on patch_generate.c after soft-float crash on real hardware.",
      "kind": ["build"]
    },
    {
      "revision": 1,
      "date": "2026-03-23",
      "summary": "Initial port.",
      "kind": ["initial"]
    }
  ]
}
```

Field semantics:

- `revisions[]` is **ordered newest-first** (matching how a user reads "what's new")
- `revision` (int) — must match the rev number in the entry
- `date` (ISO 8601 date) — when the revision was published, not when work started
- `summary` (string, ASCII, 200 chars max) — one or two sentences, user-facing language. Not a commit message. Not jargon. "Fixed a crash on real hardware" beats "Adjust __divsf3 stub linkage."
- `kind` (string array, optional) — categorical tags from a small fixed vocabulary: `initial`, `bugfix`, `dep` (dependency change), `test` (test coverage change), `perf`, `build` (toolchain/optimization), `compat` (Amiga-specific compatibility fix), `doc`. Used for filtering and badging.

The schema is **additive**. Packages without a `revisions` array continue to work — the UI shows the existing single version and nothing extra. No breaking change for existing consumers.

### Source of truth

The source of truth lives in **`PORT.md`** under a new mandatory `## Revision History` section:

```markdown
## Revision History

### 1.7.1-3 (2026-04-12) [dep, test]

Re-link against oniguruma 6.9.9 to fix a regex test that crashed on FS-UAE.

### 1.7.1-2 (2026-04-05) [build]

Drop -O1 hot-path optimization on patch_generate.c after soft-float crash on real hardware.

### 1.7.1 (2026-03-23) [initial]

Initial port.
```

PORT.md already exists per port and already gets enriched at port time. The publisher agent reads PORT.md and writes the JSON. The Markdown form is what humans edit; the JSON form is what the site consumes. Single source of truth, no double-bookkeeping.

### Validation

`scripts/check-port-metadata.sh` gains a Check 3c:

- For each port, count the rows in `PORT.md` `## Revision History` (after the heading, before the next `## ` heading)
- The latest row's revision number must equal the Makefile `REVISION`
- The latest row's `VERSION-REVISION` string must match `display_version`
- A port at REVISION > 1 with no Revision History section is **FAIL**
- A port at REVISION = 1 with no Revision History section is **WARN** (encourage but not block)

This means: you cannot bump REVISION in a Makefile and commit without also adding a row to PORT.md. The pre-commit hook enforces it. No way to silently rev a port.

### Capture point

The `amiport-publisher` agent gates publishing. Its existing prerequisite chain becomes:

1. Tests pass (existing)
2. `make check-port-metadata` passes (existing — now also validates revision history)
3. `PORT.md` Revision History latest row matches Makefile REVISION (new — enforced by check-port-metadata)
4. **NEW gate:** publisher agent parses `PORT.md` Revision History, builds the `revisions` array, writes it into `site/data/packages/<name>.json`
5. Deploy (existing)

For initial ports (REVISION = 1), the publisher writes a single `{revision: 1, date: published_at, summary: "Initial port.", kind: ["initial"]}` entry automatically — the human only needs to write entries for actual rev bumps.

### Surfaces

- **`site/packages.html` detail view** — a new "Revision History" group-frame below "Porting Notes." Each entry is a row with date, version-revision, kind badges, and summary. Most-recent row visually emphasized.
- **`site/changelog.html`** — currently one row per port. Becomes one row per (port, revision). Date column uses each revision's date, not just the latest publish. Old static rows continue to work via fallback to the single `revision` field.
- **`site/feed.php`** (RSS) — emit one item per revision instead of one per package. Title: `jq 1.7.1-3`. Description: the revision summary plus kind tags. Existing single-version packages emit one item as before.
- **`site/api/v1/activity.php`** — same change as feed.php. Activity feed shows distinct rows per revision, with the summary as subtext.
- **Home page Popular Ports widget** — already updated in Part A to show the version with revision suffix. No further change needed; the widget stays intentionally minimal.
- **News page** — no automatic surface. Operator may opt to write a `/post-news` entry for a notable revision (e.g., a security fix), but most revisions are too small to warrant news.

### Retrofit

Two paths for the 13 existing packages with `revision > 1`:

**Option R1 — Backfill from git log (recommended).**
- Write a one-shot script `scripts/backfill-revision-history.sh` that, for each port at REVISION > 1, walks `git log --oneline ports/<name>/` and identifies commits that bumped REVISION
- Use the commit subject as a seed summary, prefixed with `(reconstructed)`
- Operator reviews each one and rewrites in user-facing language before committing
- Estimated effort: ~30 minutes for 13 ports

**Option R2 — Leave history blank, capture going forward.**
- Existing rev > 1 packages get a single retroactive entry: `{revision: <current>, date: <published_at>, summary: "(History before this revision was not captured.)", kind: []}`
- Cleaner, but leaves users with a blank history for the most popular ports (jq, sed)

R1 is preferred because the data exists in git and the cost is small. Worth doing right.

## Rationale

**Why PORT.md as the source of truth?** PORT.md is the existing per-port narrative document. Every port has one. It gets enriched at port time and at revision time. Adding a structured section to a document people already write is lower-friction than a new file. The publisher agent already reads PORT.md.

**Why a `kind` taxonomy?** Three reasons. (1) Lets the UI badge entries — a `bugfix` looks different from a `perf` tweak. (2) Lets users filter the changelog ("show me only security fixes"). (3) Forces the human to think about the *category* of change, which catches "what's actually different" better than free-text alone.

**Why an array on the package JSON instead of a separate revisions endpoint?** The packages JSON is already the per-package proxy file. Co-locating revision history avoids a second fetch on the detail view. The array is small (most ports will have 1-3 entries; the longest current case is jq at rev 3). Total payload bloat for the full packages.php response with all 13 revisioned packages backfilled: estimated under 8KB. Cheap.

**Why enforce in pre-commit instead of just publisher?** Pre-commit runs locally and catches the gap immediately. Publisher runs late in the chain. Catching it early means the human doesn't bump REVISION, fix six other things, then get told to also write notes — they get told the moment they touch the Makefile. Faster feedback loop, fewer mistakes.

**Why ASCII-only and 200-char cap?** ASCII per existing repo rule (avoids bebbo-gcc preprocessor issues that bit other ports, plus it'll render correctly in the Aminet readme footprint). 200 chars forces clarity — a revision summary that needs more than 200 chars is hiding a complexity that should be split into multiple revisions.

**Why not also propagate to the Aminet `.readme`?** The Aminet readme is uploaded once per LHA. We could add a "Recent changes:" section to the readme template populated from the latest revision entry. **Yes, and proposed as Part 2** — see "Phasing" below. Not in the v1 scope to keep the first cut small.

## Success criteria

1. `make check-port-metadata` blocks a commit that bumps REVISION without adding a PORT.md history row.
2. Visiting any package detail page on the site shows the full revision history with dates and summaries.
3. The RSS feed emits distinct items per revision; subscribing to the feed makes revision activity visible without needing to visit the site.
4. The 13 existing rev > 1 packages all have backfilled history within 1 week of acceptance.
5. The next REVISION bump on any port (whoever does it) writes a useful summary without being prompted, because the workflow makes it the natural path.
6. A user filing a bug report can identify which revision they have and which revision contains a fix, without reading source code.

## Alternatives Considered

**Alt 1: Use git log directly, no separate field.**
- Render `git log --oneline ports/<name>/` on the package detail page server-side
- Pro: zero new schema, zero capture workflow
- Con: commit messages are written for engineers, not users — they're full of internals like "fix __divsf3 stub" which means nothing to a user trying to decide whether to upgrade. The whole point is to translate engineering changes into user-facing language at capture time, and git log skips that translation.
- Rejected.

**Alt 2: Free-text changelog field, no per-revision structure.**
- One big `changelog` string per package, edited cumulatively
- Pro: simpler schema, no array
- Con: no way to distinguish "current revision changes" from "all historical changes." Editing becomes append-only and gets messy. No per-revision dating, no programmatic filtering, no per-revision RSS. Loses the structured benefits.
- Rejected.

**Alt 3: Defer entirely until there's a complaint.**
- Ship nothing. Wait for a user to ask "what changed in jq rev 3?"
- Pro: zero work
- Con: by the time a user asks, we've shipped 5 more revisions on other ports and the backfill cost has compounded. Users who would have asked have already left. Trust gaps don't surface as bug reports — they surface as silent disengagement.
- Rejected.

**Alt 4: Use GitHub releases as the source.**
- Tag each port revision in git, write release notes on GitHub
- Pro: leverages existing GitHub UI, no schema change
- Con: amiport ships ~10 ports per batch — that's a lot of GitHub releases, and the GitHub UI is not where Amiga users go. The site is the user-facing surface, not GitHub. Plus this couples the porting workflow to an external service the toolchain otherwise doesn't depend on.
- Rejected.

## Phasing

**Phase 1 (this PDR, ~half day):**
- Schema addition to packages.json
- PORT.md Revision History format documented in `ports/templates/PORT.md.template`
- check-port-metadata.sh Check 3c enforcement
- amiport-publisher agent updated to parse PORT.md and write `revisions[]`
- packages.html detail view renders revision history
- changelog.html and feed.php emit per-revision entries
- Backfill script for 13 existing rev > 1 packages

**Phase 2 (separate PDR if/when desired):**
- Aminet `.readme` template gains a "Recent changes" section sourced from the latest revision entry
- The `aminet-publisher` agent enforces this is non-empty for revision > 1 publishes
- Provides Aminet users (who don't visit the website) with the same change visibility

**Phase 3 (separate PDR if/when desired):**
- News page integration — operator can opt to promote a revision to a full news entry via the existing `/post-news` skill, with the revision summary pre-filled
- Ties revision history to project narrative without forcing every revision to become news

## Open questions

1. Does the publisher agent need a "review the auto-generated entry" prompt for initial ports, or is `"Initial port."` always good enough? (Lean: always good enough — initial ports already have rich `porting_notes`.)
2. Should the JSON schema validate the `kind` enum, or accept arbitrary strings? (Lean: validate, with an explicit `other` escape hatch.)
3. Should `summary` allow inline links (Markdown `[text](url)`)? (Lean: no, keep it plain text. Links lead to mission creep — "the readme is the changelog now.")
4. Retrofit policy for ports that get re-revved later — do we keep the `(reconstructed)` marker on old entries forever, or strip it once a real human has reviewed? (Lean: strip on review, with a `verified: true` field if we want to track which entries are human-curated.)

## Files this PDR would touch (for the implementer)

- **Schema/data:** `site/data/packages/<all>.json` (additive field)
- **Capture:**
  - `ports/templates/PORT.md.template` (new section)
  - `scripts/check-port-metadata.sh` (new Check 3c)
  - `.claude/agents/amiport-publisher.md` (parse + write step)
  - `scripts/backfill-revision-history.sh` (new, one-shot)
- **Render:**
  - `site/js/packages.js` (detail view)
  - `site/js/changelog.js` (per-revision rows)
  - `site/feed.php` (per-revision RSS items)
  - `site/api/v1/activity.php` (per-revision activity)
- **Docs:**
  - `CLAUDE.md` (mention the new PORT.md Revision History requirement under Versioning)
  - `docs/pdr/README.md` (add PDR-011 to the index when accepted)
  - `docs/architecture.md` (mention the data flow if relevant)
- **NOT touched:** the `lib/`, `ports/<name>/ported/`, `tests/`, or any C source. This is a metadata/UX change, not a code change to the ports themselves.
