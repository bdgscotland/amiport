---
name: __progname strong symbol conflict
description: argv_expand.o in libamiport.a now defines __progname as a strong (non-weak) symbol -- redefining it in ported source causes "multiple definition" linker error
type: feedback
---

When `amiport_expand_argv()` is called, `argv_expand.o` provides a strong `__progname` definition. The old workaround of adding `char *__progname = "dirname";` to the ported source now causes a linker error:

```
multiple definition of `__progname'; argv_expand.o:(.data+0x0): first defined here
```

**Fix:** Remove the explicit `__progname` definition from ported source. The `amiport_expand_argv()` call pulls in `argv_expand.o` which provides `__progname` as a strong symbol, and updates it from argv[0] at runtime.

**Why:** The known-pitfalls doc says to define `__progname` directly because the weak symbol got stripped. But `argv_expand.o` was later updated to use a strong symbol -- so the pitfall workaround is now the bug.

**How to apply:** When transforming or building ports that call `amiport_expand_argv()`, do NOT add `char *__progname = "name";` to the source. The argv_expand.o copy handles it.
