Paths: site/packages/**, site/data/packages/*.json, ports/*/Makefile, .claude/agents/amiport-publisher.md, .claude/agents/site-manager.md, scripts/publish-aminet.sh

# Site Mirror Discipline — Stage Before Deploy

## The Rule

**Every LHA advertised in `site/data/packages/<name>.json` MUST exist at the corresponding path in `site/packages/<lha>` BEFORE any rsync deploy.**

The local `site/` directory is the authoritative mirror for the live server. The deploy step rsyncs `site/` to amiport.platesteel.net with `--delete`, which means **anything on the server that is NOT in `site/` locally will be silently removed.**

This applies to:
- Per-revision LHAs: `<name>-<version>.lha` (rev 1) or `<name>-<version>-<revision>.lha` (rev > 1)
- Machine LHAs: `<name>-<version>-machine.lha`
- The corresponding `.readme` files

## Mandatory publishing flow

When publishing or re-publishing any port, the order is:

1. Build the LHA in `ports/<name>/` (build-manager / publish-aminet.sh)
2. Update `site/data/packages/<name>.json` with version/revision/sha/size
3. **Stage the LHA: `cp ports/<name>/<lha> site/packages/<lha>`**
4. **Stage the machine LHA: `cp ports/<name>/<lha-machine> site/packages/<lha-machine>`**
5. Run `make check-port-metadata` (Check 9 enforces site mirror integrity)
6. Commit
7. Dispatch site-manager to deploy

**Do NOT** scp/rsync individual LHAs to the server, ever. That bypasses the local mirror, and the next standard deploy will silently delete the file.

## Enforcement

### check-port-metadata.sh Check 9 (mandatory pre-commit gate — file existence)

For every `site/data/packages/<name>.json` that has a `download` field pointing into `/packages/`, Check 9 verifies the file exists at the corresponding `site/packages/<lha>` path. FAIL on any miss.

If `ports/<name>/<lha>` exists locally but the staged copy at `site/packages/<lha>` does not, the failure message names the exact `cp` command needed to fix it.

### check-port-metadata.sh Check 10 (mandatory pre-commit gate — revision consistency)

For every `site/data/packages/<name>.json` whose `name` matches a `ports/<name>/Makefile`, Check 10 verifies that the catalog JSON's `revision` field equals the Makefile's `REVISION` value (defaulting to 1 if no `REVISION` line). Also verifies the `version` strings match.

This catches **phantom revision drift**, where the catalog metadata invents a revision the port source never shipped, OR where the catalog ships a revision that the Makefile failed to record. Both directions are bugs:

- **Phantom rev** (catalog ahead of Makefile): user clicks the advertised LHA and gets a 404 (or the previous build, which is wrong). 2026-04-14 incident: `lua.json` had `revision: 2` while `ports/lua/Makefile` had no `REVISION` line (= rev 1). No rev-2 work had ever been done. Rolled back catalog to rev 1.
- **Lagging Makefile** (Makefile behind catalog): the artifact really shipped at rev N+1, the catalog correctly advertises it, but the port Makefile didn't get bumped. Means a future `make package` from clean checkout would silently produce a rev-N artifact, and any check-port-metadata-driven revision tracking would think the port is older than it is. 2026-04-14 incident: `sed.json` ships rev 3 (and `site/packages/sed-1.47-3.lha` exists with matching sha) but `ports/sed/Makefile` says `REVISION = 2`. Escalated.

The failure message names BOTH possible fixes and asks the operator to pick whichever reflects the actual work done. There is no auto-fix because the script cannot distinguish between "phantom rev that should be removed from the catalog" and "real work that should be recorded in the Makefile."

### site-manager dry-run gate

Before running any production rsync against `site/packages/`, the site-manager agent MUST run `rsync --dry-run --delete` first and inspect the deletion list. If ANY file under `/packages/` would be deleted, ABORT and report — do not proceed without explicit user approval. Unexpected deletes mean the local mirror is out of sync with the server, which is exactly the failure mode this rule exists to prevent.

### amiport-publisher staging step

The amiport-publisher agent's workflow MUST include an explicit "stage LHA into site/packages/" step before invoking site-manager. This is the publisher's job, not the deploy step's.

## Why this rule exists

**2026-04-14:** A standard rsync `--delete` deploy run by the site-manager agent silently deleted four LHAs from amiport.platesteel.net because they had been built and uploaded to the server by parallel sessions without ever being staged into `site/packages/` locally:

- `jq-1.7.1-3.lha` — recovered from `ports/jq/`, restored
- `wget-1.20.3-2.lha` — recovered from `ports/wget/`, restored
- `amiport-1.0.lha` — local copy at `ports/amiport/` had a different SHA than the JSON advertised; not safely recoverable, escalated to user
- `lua-5.4.7-2.lha` — no rev-2 LHA exists anywhere in the working tree (only `lua-5.4.7.lha` rev 1); the catalog had drifted to rev 2 without an artifact ever being built and committed; escalated to user

Two of four were lost-or-questionable. The rsync incident did not cause the underlying drift — it just exposed it. The drift had been present for as long as the parallel sessions had been deploying directly to the server. Without Check 9 the next deploy would have re-triggered the same loss.

## Related

- `.claude/rules/catalog-sync.md` — the analogous rule for keeping `data/catalog.json` and `site/data/catalog.json` in sync
- `scripts/check-port-metadata.sh` — Check 9 implementation
- `.claude/agents/amiport-publisher.md` — the publisher agent that must stage LHAs
- `.claude/agents/site-manager.md` — the deploy agent that must dry-run-gate
