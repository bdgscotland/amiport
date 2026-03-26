---
name: PROGDIR path separator
description: When building file paths with PROGDIR: or any AmigaOS volume prefix, check trailing character before adding a slash separator
type: feedback
---

When HOME falls back to "PROGDIR:" on AmigaOS, a naive "%s/%s" gives "PROGDIR:/file"
which is wrong. "PROGDIR:" already ends with ':' and needs no separator.
The fix: check the last character of home -- if it is ':' or '/', use "%s%s", otherwise
use "%s/%s". This applies to any code that builds paths from an env var that might be
a volume prefix (PROGDIR:, SYS:, WORK:) rather than a directory path.

**Why:** AmigaDOS volume references end with ':'. A '/' after ':' creates an invalid path
on OFS/FFS filesystems.

**How to apply:** Whenever substituting HOME with a fallback that could be a volume
prefix, always check the trailing character and choose the separator accordingly.
