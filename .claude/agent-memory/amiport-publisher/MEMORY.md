# amiport-publisher Agent Memory

- [vamos_wrapper_bug.md](vamos_wrapper_bug.md) - Bug fix: vamos wrapper was adding -C argument to args instead of discarding it
- [catalog_two_files.md](catalog_two_files.md) - Two catalog.json files exist (data/ and site/data/) — both must be updated on publish, they can drift
- [site_rsync_delete_wipes_packages.md](site_rsync_delete_wipes_packages.md) - Site rsync --delete wipes server packages/*.lha not in local site/packages/ — add --exclude 'packages/*.lha' to site rsync
