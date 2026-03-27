---
name: port_vim_perf
description: Performance findings for ports/vim 9.1 — safe_Lock redundancy, -O2 struct analysis, binary size reduction, stack cookie discrepancy reviewed 2026-03-26
type: project
---

# vim 9.1 Performance Review Notes (2026-03-26)

## Key findings

### safe_Lock redundancy (LOW impact)
mch_init() sets pr_WindowPtr=-1 globally at startup (before mch_check_win is called).
All subsequent safe_Lock calls save/restore the value they set: save(-1), set(-1), restore(-1).
Pure overhead. The save/restore costs FindTask(NULL) + 2 memory r/w per Lock call.
NOT worth fixing -- it's called on filesystem operations, not in any tight loop.
The global suppression in mch_init was added by amiport; safe_Lock is upstream code.

### -O2 struct-by-value risk (CLEAR)
typval_T is ~16 bytes (vartype_T + char + padding + 8-byte union).
Searched all .pro files: NO functions return typval_T, garray_T, or other large structs by value.
All large struct returns are via pointer. -O2 is safe for this codebase.

### Stack: 256KB is sufficient for FEAT_NORMAL
__stack = 262144 (256KB) for OS3. MAXPATHL=256, IOSIZE=1025, CMDBUFFSIZE=256.
Vim uses heap allocation extensively for buffers. No large local arrays found.
The 256KB is appropriate. 1MB would be excessive for OS3 systems with limited RAM.

### PORT.md inconsistency
PORT.md says "1MB stack" but source has 262144 (256KB) for the OS3 code path.
The 1MB comment came from the OS4/MorphOS path. PORT.md needs correction.
Makefile VAMOS_STACK=1024 comment says "must match 1MB" but is actually just generous
for testing (vim can't run under vamos anyway -- 68020 instructions).

### Binary size: 2.7MB -- no low-hanging fruit
regexp.o is 202KB (includes regexp_bt.c + regexp_nfa.c = 16K lines compiled as one unit).
option.o is 115KB (optiondefs.h is 3022 lines of option tables).
evalfunc.o is 114KB. These are all code/data that's actually used.
Potential savings:
- -DDISABLE_SPELL would save ~108KB of objects (spell.o + spellfile.o + spellsuggest.o)
  but spell is a useful feature for an advanced editor
- -Os instead of -O2 could reduce binary by 10-20% (est 250-550KB) with minimal speed impact
  since 68EC020 has only 256B instruction cache
- -ffunction-sections -fdata-sections + --gc-sections: potential dead code removal
  but bebbo-gcc's gc-sections support for hunk format is uncertain

### mch_char_avail: 100ms WaitForChar -- NOT a busy-wait
WaitForChar internally calls Wait() which yields the task. This is correct OS3 usage.
Not a performance concern.
