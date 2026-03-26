---
name: nl_analysis
description: OpenBSD nl v1.8 portability analysis: EASY verdict, Category 1 CLI, NL_TEXTMAX missing from libnix, __dead strip, getline shim, regex Tier 2, mbrlen/MB_CUR_MAX runtime pitfall, errc/strtonum shims, pledge stub, exit code fixes
type: project
---

OpenBSD nl v1.8 portability analysis.

**Why:** nl is a line-numbering utility (Category 1 CLI, simple stdin/stdout). Very small single-file port.

**Verdict:** EASY

Key issues:
- NL_TEXTMAX: not in libnix sys-include. Only in ixemul/include/limits.h. Must define locally: `#define NL_TEXTMAX 255`
- __dead: not defined in any non-ixemul header. Must redefine: `#define __dead __attribute__((noreturn))`
- getline(): not in libnix (only __getline exists in sys-include). Replace with amiport_getline().
- regex: sys-include/regex.h exists with full POSIX API (regcomp/regexec/regfree/regerror) as newlib native. If regcomp works correctly on AmigaOS this is Tier 1 trivial; otherwise falls to Tier 2 amiport_emu_regex(). Given prior analysis classifies as Tier 2, use amiport_emu_regex().
- mbrlen()/MB_CUR_MAX: libnix MB_CUR_MAX expands to __locale_mb_cur_max() at runtime. mbrlen() is present in libnix. The -d option uses mbrlen() to parse delimiter chars -- since AmigaOS has no real multibyte support, mbrlen() will return 1 for ASCII, which is fine for typical use. The MB_CUR_MAX usage as size argument to mbrlen() is a runtime call, not a VLA -- this is safe.
- errc()/strtonum(): available via amiport/err.h shims.
- pledge()/unveil(): stub as macros.
- setlocale(): amiport_setlocale() always returns "C".
- getprogname(): provided via amiport/utsname.h macro.
- exit(EXIT_FAILURE) -> exit(10) for Amiga exit codes.
- wchar.h: included but not used for any function calls (only MB_LEN_MAX/MB_CUR_MAX constants). Header available in libnix.

**How to apply:** During code-transformer stage, apply these fixes to ported/nl.c.
