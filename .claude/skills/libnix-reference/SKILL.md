---
name: libnix-reference
description: Load the libnix function availability reference. Lists all 700+ functions available in libnix (the -noixemul C runtime), including which are present and which are missing. Use when checking whether a C standard library function is available on AmigaOS, or when a linker reports undefined symbols.
---

# libnix Function Reference

Load `docs/references/libnix-reference.md` now. It contains:

- Complete function list extracted from libnix libc.a (700+ functions)
- Runtime behavior documentation from libnix.texi
- Which C99/POSIX functions ARE available (many more than expected)
- Which functions are NOT available (and what to use instead)
- Memory allocation behavior (`-noixemul` cleanup semantics)
- Printf/scanf format support details

**Use this reference to answer:**
- "Does libnix have `strptime()`?" → Yes
- "Does libnix have `strtoll()`?" → Yes (surprisingly)
- "Does libnix have `getline()`?" → Check the reference
- "Why does the linker say undefined `realpath`?" → Check if it's in the list
