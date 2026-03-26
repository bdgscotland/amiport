---
name: tetris_console_shim_pc_bc_up
description: Classic termcap globals PC/BC/UP were missing from console-shim, causing linker errors in tetris and any other program using the classic termcap API directly
type: project
---

Programs that use the classic BSD termcap API directly (tetris, vi, etc.) declare `extern char PC, *BC, *UP;` and set them from `tgetstr()` results. These globals must be provided by the termcap library.

The console-shim `globals.c` did not define `PC`, `BC`, or `UP`, causing linker errors:
```
undefined reference to 'UP'
undefined reference to 'BC'
undefined reference to 'PC'
```

**Fix applied:** Added `PC`, `BC`, `UP` definitions to `lib/console-shim/src/globals.c` and `extern` declarations to `lib/console-shim/include/amiport-console/term.h`.

**Why:** Traditional termcap implementations define these in their library. Programs like tetris `extern` them and set them after calling `tgetstr()`. Without definitions, the linker fails.

**How to apply:** Any future port that uses the classic termcap API (2-letter `tgetstr`/`tgetnum`/`tgetflag` functions and the global `PC`/`BC`/`UP` pattern) now links correctly against `libamiport-console.a`.
