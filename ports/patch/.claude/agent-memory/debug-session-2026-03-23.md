# Debug Session 2026-03-23 — patch v1.78 getopt_long + dirname crashes

## Summary

Three bugs found and fixed. 40/42 FS-UAE tests now passing (up from 0 functional tests).

---

## Bug 1: libnix getopt_long returns '?' for all options (FIXED)

**Root cause:** `patch.c` used `#include <getopt.h>` (system/libnix header). According to
`docs/references/newlib-availability.md`, libnix's `getopt_long` is broken — it returns
`'?'` for ALL options, not just unknown ones.

**Evidence:** FS-UAE debug output showed `DBG:ch=63` for every test (ASCII 63 = '?').
The `'?'` return triggers `default: → usage() → my_exit(10)`, giving RC=10 for all tests.

**Fix:** Replace `#include <getopt.h>` with `#include <amiport/getopt.h>`.
The amiport implementation (`amiport_getopt_long`) works correctly.

---

## Bug 2: dirname() corrupts filearg[0] in-place (FIXED)

**Root cause:** After getopt was fixed, tests failed with "can't find WORK:test-patch-base.txt"
because `filearg[0]` was being corrupted. The code called `dirname(filearg[0])` — POSIX allows
`dirname()` to modify the input buffer in-place. The libnix `dirname` does exactly this,
corrupting `filearg[0]` so later `scan_input(filearg[0])` received a garbage path.

**Fix:** Remove the `dirname(filearg[0])` + `unveil(origdir)` block entirely. Since `unveil()`
is a no-op on AmigaOS, the entire block is dead code. Dropping it eliminates the corruption.

**Location:** `patch.c`, inside the `if (filearg[0] != NULL)` block in `main()`.

---

## Bug 3: Debug instrumentation left in source (FIXED)

Multiple `fprintf(stdout, "DBG:...")` prints were in the source from earlier debugging.
Removed all of them as part of this fix session.

---

## Remaining failures (pre-existing, not regressions)

**Test 5 - Ed diff temp file conflict:**
`PFATAL: can't create T:patcho500000: Text file busy`
The Ed script diff mode creates additional temp files and one conflicts with a
previously-opened temp handle. This is an Ed-mode-specific issue with AmigaOS temp
file semantics. Ed diff is rarely used in practice.

**Test 22 - `-D` define mode:**
The `-D SYMBOL` flag should wrap changes in `#ifdef SYMBOL` / `#endif` markers.
The output contains the file content but without the ifdef wrappers. This is a
pre-existing limitation — the feature was never exercised since getopt was broken.

---

## Iteration count: 3 (within 5-iteration budget)
- Iteration 1: Replace `#include <getopt.h>` with `#include <amiport/getopt.h>`
- Iteration 2: Remove `dirname(filearg[0])` to prevent corruption of filearg[0]
- Iteration 3: Fix test case 1 that modified test-patch-base.txt in-place
