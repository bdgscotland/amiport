# Debug Session 2026-03-27 -- CPython 3.11.12 AN_MemCorrupt Crash

## Iteration count: 6 (iteration 5 = pymalloc disable, iteration 6 = current)

## Current Status
- python3 -V works (RC=0) -- test 1 passes
- python3 -ISEc pass crashes -- AN_MemCorrupt (0x81000005) -- Guru Meditation
- Test hangs at test 2 in FS-UAE debug run

## History
### Iteration 1-3: ACPU_InstErr (#80000004)
- Root cause: _PyCode_Quicken modifying bytecode opcodes incorrectly on big-endian
- Fix: disabled _PyCode_Quicken in specialize.c with ifdef __AMIGA__ guard
- Result: ACPU_InstErr gone, but now AN_MemCorrupt appears

### Iteration 4: Arena alignment fix
- Fixed _PyObject_ArenaMalloc to over-allocate and align to 4KB
- Required for pymalloc to find pool headers via POOL_ADDR(p) = p & ~0xFFF

### Iteration 5: pymalloc disabled
- Disabled WITH_PYMALLOC entirely (commented out in pyconfig.h)
- Result: STILL crashes with AN_MemCorrupt
- Conclusion: pymalloc is NOT the cause of AN_MemCorrupt

## Enforcer Analysis
- 73-74 hits, ALL are "Processor Interrupt Level 3" false positives (ROM timer)
- No Mungwall sentinel hits (0xDEADBEEF, 0xABADCAFE)
- Mungwall IS running but does NOT detect the corruption
- Conclusion: corruption happens in STATIC/BSS memory (not tracked by Mungwall)

## Key Observations
1. pymalloc disabled: STILL crashes -> not a pymalloc bug
2. Mungwall doesn't fire: corruption not in heap-tracked memory
3. AN_MemCorrupt = "Corrupt memory list detected in FreeMem"
4. The SAME python3 eval loop address appears in every interrupt snapshot
5. Hunk 0 frames: 0x7674E (top), 0x76906, 0x769C0, 0x1494BE (deeper)

## Most Likely Cause: SipHash uint64_t unaligned reads on 68k
SipHash24 in pyhash.c uses uint64_t (long long = 8 bytes on 68k).
Reading uint64_t from a 4-byte-aligned char* pointer on big-endian 68k:
- Gets byte-swapped results (wrong hash values)
- May read 8 bytes from a pointer that only points to 4-byte aligned memory
If the hash function incorrectly processes the string data, it could
compute wrong indices into Python's dict/set structures, causing
out-of-bounds writes that corrupt adjacent memory.

## What's in the consistently-crashing code path
The eval loop is processing importlib bootstrap bytecode (frozen module).
This involves dict lookups (LOAD_GLOBAL etc.) which require hashing string names.
If string hashing is producing wrong indices, dict entries could be placed
in wrong buckets, possibly corrupting the dict's hash table in the GC list.

## Next Steps
1. Check pyhash.c SipHash implementation for uint64_t alignment issues on 68k
2. Check if HASH_INF/NaN values are defined correctly for big-endian
3. Try disabling ROTATE macro unaligned reads in SipHash
4. If needed: fall back to FNV hash (simpler, definitely endian-safe)
