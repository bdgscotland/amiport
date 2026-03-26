Paths: data/catalog.json, site/data/catalog.json, site/data/packages/*.json

# Catalog Sync — Two Files, Always In Sync

## The Rule

There are TWO catalog files that must stay synchronized:

1. **`data/catalog.json`** — Project-level catalog (source of truth for porting pipeline)
2. **`site/data/catalog.json`** — Site catalog (deployed to Dreamhost, served by catalog.php)

**After ANY change to `data/catalog.json`, immediately copy it to `site/data/catalog.json`.**

```bash
cp data/catalog.json site/data/catalog.json
```

This applies to:
- Moving candidates to ported (batch-port-parallel, port-project)
- Marking candidates as in_progress/failed
- Scoring/re-scoring
- Adding new candidates
- Publishing (amiport-publisher updates status)

## Why

The site catalog is what users see on amiport.platesteel.net. If it's stale:
- The catalog page shows wrong candidate counts
- Recent activity doesn't show new ports
- The "ported" count on stats doesn't increase
- The homepage package count is wrong

This was discovered during a parallel batch port (2026-03-25) where 3 ports were ported and published but the site catalog only had 1 of 3 because each publisher independently modified the catalog and only the last one's copy persisted.

## Enforcement

Three layers:

1. **Pre-commit hook** (`.githooks/pre-commit`): Blocks commits where the two catalog files differ.
2. **Pre-push hook** (`.githooks/pre-push`): Deploys `site/` to Dreamhost on every push via rsync. The live site always matches git.
3. **Skills/agents**: batch-port-parallel, port-project, and amiport-publisher must all sync catalogs as part of their workflow.

## Homepage Package Count

The homepage (`site/index.html`) dynamically loads the package count from the packages API. Do NOT hardcode package counts in HTML.
