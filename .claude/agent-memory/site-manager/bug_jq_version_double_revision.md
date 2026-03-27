---
name: jq version double revision bug
description: jq.json version field contained the revision suffix already, so JS rendered it twice
type: project
---

The jq package JSON had `version: "1.7.1-2"` combined with `revision: 2`. The JS in packages.js builds the display version as `version + "-" + revision` when revision > 1, resulting in `1.7.1-2-2`.

**Why:** The amiport-publisher agent or whoever created jq.json embedded the revision into the version string directly, then also set the revision field correctly.

**How to apply:** When auditing package JSON files, check that `version` contains only the upstream version (no `-N` suffix) and `revision` is the port revision number. The JS handles the display formatting.

Fixed 2026-03-27: set `version: "1.7.1"`, kept `revision: 2`.
