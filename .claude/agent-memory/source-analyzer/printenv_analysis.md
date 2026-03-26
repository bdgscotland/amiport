---
name: printenv_analysis
description: OpenBSD printenv v1.8 portability analysis: EASY verdict, Category 1 CLI, environ stub needed, pledge stub, exit code fix
type: project
---

# printenv 1.8 Portability Analysis

**Verdict:** EASY

**Key facts:**
- 69 lines, single file, pure Category 1 CLI
- No Tier 2/3 issues
- `environ` global not provided by libnix -- must define local empty stub on AmigaOS (same pattern as awk port: `static char *_amiport_empty_environ[] = { NULL }; char **environ = _amiport_empty_environ;`)
- BUT: `environ` with AmigaOS `GetVar()` cannot enumerate all ENV: variables -- the empty stub means `printenv` with no args prints nothing on Amiga; this is a semantic limitation, not a build blocker
- `pledge()` -- stub as `#define pledge(p, e) (0)`
- `<err.h>` -- use `<amiport/err.h>` (already in shim)
- `<unistd.h>` -- use `<amiport/unistd.h>`
- `exit(1)` -- change to `exit(10)` (RETURN_ERROR for Amiga shell scripts)
- `err(1, "pledge")` -- change to `err(10, "pledge")`
- ported/ already exists but is identical to original/ (not yet transformed)
- **Why: Because printenv scans the C runtime `environ[]` array, which AmigaOS does not provide. AmigaDOS variables are iterated with ScanVars() (dos.library), not via a flat string array. Implementing a real environ would require building the array with ScanVars() at startup -- feasible but complex and not yet in the shim.**
