---
name: define __progname directly
description: User explicitly requires __progname defined directly in source, not as extern, due to weak symbol stripping
type: feedback
---

Always define `__progname` directly in ported source (`char *__progname = "progname";`), never keep the `extern char *__progname;` declaration from the original.

**Why:** bebbo-gcc's linker strips unreferenced weak data symbols from archive members. The weak `__progname` in `argv_expand.o` (libamiport.a) may not survive linking. Any `fprintf(stderr, "%s", __progname)` in `usage()` then crashes with an invalid memory access (garbled output + Guru Meditation on FS-UAE). See known-pitfalls: `__progname Weak Symbol Lost During Linking`.

**How to apply:** When transforming any OpenBSD source that has `extern char *__progname;`, replace it with `char *__progname = "toolname";`. The `amiport_expand_argv()` call updates it from argv[0] at runtime via the extern declaration in glob.h.
