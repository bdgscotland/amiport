---
name: rm_analysis
description: OpenBSD rm v1.45 portability analysis: MODERATE verdict, statfs/fstatfs is blocker, strmode in libnix (no shim needed), sys/mount.h must be stubbed, Pflag overwrite path is lowest priority, exit code fixes
type: project
---

# rm v1.45 Portability Analysis

**Verdict: MODERATE**
**Category: 1 (CLI)**

## Key findings

### Blocking issues
- `sys/mount.h` does NOT exist in libnix/newlib sys-include. Provides `statfs` struct and `fstatfs()`. This is used ONLY in `rm_overwrite()` (the `-P` secure overwrite flag). The `statfs`/`fstatfs()` functions are also absent from libc.a.
- `strmode()` declared in libnix string.h (from `__stdargs void strmode(int, char *)`), but the symbol was NOT found in libc.a by nm scan. May be header-only stub or compiled differently. **Risk: linker failure.** Provide local implementation.
- `fsync()` NOT found in libc.a. Used in `rm_overwrite()`.
- `O_NONBLOCK` and `O_NOFOLLOW` ARE defined in sys-include fcntl.h (_FNONBLOCK=0x4000, _FNOFOLLOW=0x100000) -- these compile fine even if the flags are effectively no-ops for AmigaOS file operations.

### Already shimmed (no issues)
- `fts_open/fts_read/fts_close/fts_set` -- full amiport FTS shim in place (`<amiport/fts.h>`)
- `unlink()` -- `amiport_unlink()` via DeleteFile()
- `rmdir()` -- `amiport_rmdir()` via DeleteFile()
- `lstat()` -- `amiport_lstat()` (alias to amiport_stat())
- `stat()` -- `amiport_stat()`
- `access()` -- `amiport_access()`
- `isatty()` -- `amiport_isatty()`
- `getopt()` -- `amiport_getopt()`
- `user_from_uid()` -- `amiport_user_from_uid()` in amiport/pwd.h
- `group_from_gid()` -- `amiport_group_from_gid()` in amiport/grp.h
- `errc()/warnc()` -- in `amiport/err.h`
- `arc4random_buf()` -- in libc.a (confirmed)
- `open()/write()/close()` -- libnix native (for rm_overwrite fd path)
- `fstat()` -- `amiport_fstat()`
- `pledge()` -- stub macro

### Strategy for rm_overwrite() (-P flag)
The secure overwrite feature is entirely contained in `rm_overwrite()` and `pass()`. It requires:
- `sys/mount.h` (struct statfs, fstatfs)
- `fstatfs()`
- `fsync()`

**Recommended approach:** Disable `-P` on AmigaOS with a `#ifdef __AMIGA__` stub that prints a warning and returns 1 (success). The `-P` flag is meaningless on AmigaOS FFS (journaled/logged FS behavior not applicable). This eliminates the blocker without losing core functionality.

### Other issues
- `<pwd.h>` -- NOT in libnix; use `<amiport/pwd.h>` (already provides struct passwd + getpwuid/getpwnam)
- `<grp.h>` -- sys-include has it (NetBSD) but no symbols in libc.a; use `<amiport/grp.h>` 
- `strmode()` -- declared in libnix string.h header but symbol absent from libc.a; provide local 15-char implementation
- `<fts.h>` -- NOT in libnix or sys-include; use `<amiport/fts.h>`
- `<sys/mount.h>` -- NOT in libnix/sys-include; stub out with #ifdef __AMIGA__ around rm_overwrite

### Logic bug patterns
- `sb2.st_dev != sbp->st_dev || sb2.st_ino != sbp->st_ino` in rm_overwrite -- same-file detection, but this is inside the -P stub we're disabling, so not a runtime concern
- Root check: `root.st_ino == sb.st_ino && root.st_dev == sb.st_dev` in checkdot() -- compares stat of "/" (which doesn't exist on AmigaOS) with the target. stat("/") will fail on AmigaOS. Fix: stub checkdot() to use a simpler path check, or guard the stat("/", &root) call.

### STUB VALUE WARNING
- `stat("/", &root)` in checkdot() -- "/" does not exist on AmigaOS (no root filesystem). amiport_stat("/") will likely fail or return garbage. The root-protection check will always fail, meaning `rm /` protection is ineffective on AmigaOS. Low severity (AmigaOS shell already protects system files differently).

### Exit codes
- `exit(1)` in usage() -- fix to exit(10)
- `errc(1, ...)` in rm_tree() -- fix to errc(10, ...)
- `err(1, ...)` in rm_overwrite() and rm_tree() -- fix to err(10, ...)

## Summary
4 blockers all confined to rm_overwrite() (-P flag). Disabling -P with an __AMIGA__ stub resolves all blockers. Core rm functionality (unlink, rmdir, FTS tree walk) is fully shimmed.
