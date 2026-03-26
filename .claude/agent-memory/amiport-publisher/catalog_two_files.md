---
name: catalog_two_files
description: There are two catalog.json files with different schemas — project-level and site-level — both must be updated on publish
type: project
---

There are two separate catalog.json files, both must be updated when publishing a new port:

1. `data/catalog.json` — project-level catalog. Has a `ported` array with flat entries (id, name, version, description, amiport_category, aminet_category, upstream_source, upstream_license, port_date, aminet_status, measured_binary_kb, measured_stack_kb, test_count, test_pass_rate, status). Some entries here may already exist from the porting pipeline with partial fields.

2. `site/data/catalog.json` — deployed catalog (rsync'd to Dreamhost). Has the same overall structure but the `ported` array may be missing recently published ports. This is the file that actually reaches the live site. New ports must be appended to the `ported` array here.

**Why:** The two files were created independently. The project-level one is updated by the porting pipeline; the site-level one is what gets deployed. They can drift apart.

**How to apply:** On every publish, check both files. Update `data/catalog.json` if the entry exists but is incomplete. Add a new entry to `site/data/catalog.json` ported array if absent. Then redeploy.

Note: the site catalog `ported` entries use fields: id, name, version, description, amiport_category, aminet_category, upstream_source, upstream_license, port_date, aminet_status, aminet_url, measured_binary_kb, measured_stack_kb, test_count, test_pass_rate, hardware_verified[].
