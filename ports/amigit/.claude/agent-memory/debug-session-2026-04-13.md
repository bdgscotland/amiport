# Debug Session 2026-04-13: Guru 8000 000B in amigit diff --cached

## Status: ESCALATED after 2 fix attempts + 3 FS-UAE runs (iteration limit reached)

## Crash Summary

Guru Meditation `8000000B` = ACPU_LineF (F-line instruction trap on 68000).
Triggered by `amigit diff --cached` on real AmigaOS (FS-UAE).
Works fine on vamos (which uses -s 256 flag, not the __stack cookie).
Isolation suite: 5 tests. Tests 1-4 pass. Test 5 (`diff --cached`) crashes.

## Enforcer Hits

All Enforcer LONG-WRITE hits from the isolation suite are SPURIOUS:
- From "Processor Interrupt Level 3" system interrupt handler
- Writing to 0x00F0FFFC (Ranger RAM area), PC = 0x00F00408
- NOT from amigit code
Amigit crash captured via Alert/Guru mechanism, NOT as an Enforcer hit.

## Original Crash Context (262144 stack, xdiff at -O1)

From enforcer.log Alert context (amigit TCB=0x002B65C0):
- USP at crash: ~0x0033095C
- A4 at crash: 0x00330CE1 (ODD address, within stack area -- corrupted frame)
- Crash "PC": 0x00330CE1 (odd address = not a valid instruction address)

IMPORTANT: The crash PC 0x00330CE1 is a CORRUPTED RETURN ADDRESS loaded from
the stack, NOT a code address. Mapping it via nm to "xdl_recs_cmp" was
INCORRECT -- it's a stack value that happened to fall in the code segment range.

Root mechanism: something writes an odd value to a return address slot on the
stack, causing the 68000 to jump to an odd address when executing RTS.

## Fix Attempt 1 (FAILED -- REGRESSION)

Stack increase: 262144 -> 524288 (amigit.c line 89)
Result: test 2 (amigit add) HUNG instead of test 5 crashing.
Likely cause: AmigaOS could not allocate 512KB contiguous fast RAM after
installing 72 binaries (fragmented), causing a different failure path.
Action: Rolled back stack change.

## Fix Attempt 2 (FAILED -- crash persists in different location)

Rebuilt lib/libgit2 with xdiff files at -O0 instead of -O1.
Changed XDIFF_HOTPATH_CFLAGS = $(XDIFF_CFLAGS) -fno-strict-aliasing in
lib/libgit2/Makefile.
Result: Tests 1-4 now pass. Test 5 still crashes/hangs.
New crash path (based on interrupt-time stack traces of amigit):
- _git_config_parse
- _git_config_backend_from_file
- _git_config_list_* operations
- _git_repository_discover
These are config-loading operations, NOT xdiff.

## Current State

lib/libgit2/Makefile: XDIFF_HOTPATH_CFLAGS has been changed to -O0.
This change is NOT reverted -- it should be committed as a safety measure
regardless of whether it fixes the test 5 crash.

## Real Root Cause (Best Theory)

STACK OVERFLOW during git_diff_print for a real-content diff.

Evidence:
1. Tests 1-4 (init, add, commit, add again) all pass -- these do NOT call git_diff_print.
2. Test 5 (diff --cached) always fails -- this IS the first call to git_diff_print.
3. The crash PC is always an ODD ADDRESS in the STACK AREA (0x0033xxxx vs stack base).
4. Odd-address PC = corrupted return address = stack overflow overwrote return address slot.
5. fix attempt 1 (512KB) caused HANG in test 2 -- AllocMem failed for stack.

The diff --cached call chain is:
  amigit_cmd_diff -> git_diff_index_to_workdir (or tree_to_index)
    -> git_diff_print -> git_patch_generate -> git_xdiff
    -> xdl_diff -> xdl_recs_cmp (recursive Myers O(ND))
    -> plus git_repository_open_ext, git_config loading, etc.

This is the DEEPEST call chain in amigit. With 256KB stack, the combination
of config loading (git_config_parse is large), plus diff patch generation,
plus xdiff recursion exceeds 262144 bytes.

## What To Try Next (for next debug session)

1. Try __stack = 393216 (384KB) -- midpoint between 262KB and 512KB.
   This may avoid the AllocMem fragmentation failure while still covering
   the deep call chain.

2. Run isolated 2-test suite: init + diff --cached with an EMPTY file.
   If empty file works but non-empty crashes, the xdiff recursion is the issue.

3. Check: does the crash happen with diff on an EMPTY file (zero lines)?
   If diff on empty file also crashes, the bug is in the git_diff setup path,
   not in xdiff recursion.

4. If stack increase fixes test 5, ALSO restore the full 81-test suite
   from /tmp/amigit-fullsuite.txt and run it.

5. After fix: update docs/references/crash-patterns.md with the finding.

## Files Modified (current state)

- lib/libgit2/Makefile: XDIFF_HOTPATH_CFLAGS changed from -O1 to -O0
  (should be committed -- prevents future xdiff -O1 issues even if not the
   root cause of this specific crash)
- ports/amigit/ported/amigit.c: UNCHANGED (__stack = 262144 still)
- ports/amigit/amigit: binary rebuilt with xdiff at -O0

## Backup

Full 81-test suite backed up at /tmp/amigit-fullsuite.txt.
Current 5-test isolation suite in ports/amigit/test-fsemu-cases.txt.
