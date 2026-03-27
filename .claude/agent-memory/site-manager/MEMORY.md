# Memory Index

- [bug_jq_version_double_revision.md](bug_jq_version_double_revision.md) - jq.json version field had "-2" suffix embedded AND revision:2, causing JS to render "1.7.1-2-2"
- [pitfall_js_cache_busting.md](pitfall_js_cache_busting.md) - JS files served with max-age=2592000 (30d) but no version query strings — broke browsers after JS updates
