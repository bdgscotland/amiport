---
name: site_rsync_delete_wipes_packages
description: Site rsync with --delete wipes server packages/ LHAs that aren't in site/packages/ locally
type: feedback
---

The site rsync command uses `--delete`, which removes server files not present in the local `site/` tree. The `site/packages/` directory has a `.gitignore` that blocks `packages/*.lha`, so most LHA files are not tracked or present locally. When the rsync runs, it deletes all LHAs from the server's `packages/` directory that aren't in the local `site/packages/`.

**Why:** LHAs deployed via the direct `rsync packages/` step exist on the server but not in git. The `--delete` flag treats them as stray files.

**Incident:** During the logname/touch/rm/pr/sponge publish (2026-04-11), the site rsync deleted which, strings, seq, mv, cmp LHAs from the server (the previous batch). Had to re-deploy 10 extra LHAs to restore them.

**How to apply:**
- Always add `--exclude 'packages/*.lha'` to the site rsync command to protect LHAs.
- Or use `--filter 'protect packages/*.lha'` for stricter protection.
- Corrected site rsync command:
  ```bash
  rsync -avz --delete --exclude '.env' --exclude 'data/counters/*.txt' --exclude 'packages/*.lha' \
    -e ssh site/ amiport-deploy:amiport.platesteel.net/
  ```
- Always deploy LHAs AFTER the site rsync (not before), so a re-deploy is easy if something goes wrong.
