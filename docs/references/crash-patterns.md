# Crash Pattern Knowledge Base

This file stores structured crash signature to fix pattern entries used by the `debug-agent` to accelerate crash diagnosis. When the debug agent fixes a crash, it appends a new entry here. Over time this becomes a lookup table — the agent checks this KB before reasoning from scratch.

See ADR-016 for the autonomous debug agent architecture.

---

## 1. NULL Struct Member Access

**Enforcer Signature:** Address < `0x1000`. Typically a small offset from NULL (e.g., `0x00000014` for the 5th long field of a struct accessed via a NULL pointer).

**Hit Type:** `LONG-READ`, `WORD-READ`, or `BYTE-READ` from a low address.

**Root Cause:** A function returns NULL (e.g., failed allocation, failed lookup) and the caller dereferences the result without checking. The faulting address is the struct field offset from the NULL base pointer.

**Fix Template:**
```c
/* Before (crashes if find_node returns NULL): */
struct node *n = find_node(key);
printf("%s\n", n->name);

/* After: */
struct node *n = find_node(key);
if (n == NULL) {
    fprintf(stderr, "error: node not found for key '%s'\n", key);
    exit(10);  /* amiport: RETURN_ERROR */
}
printf("%s\n", n->name);
```

**Diagnostic Clue:** The offset in the Enforcer address reveals which struct field was accessed. Cross-reference with the struct definition to identify the field (e.g., offset `0x14` = 20 bytes = 5th `LONG` field or a field after shorter members).

**Example Port:** TBD

---

## 2. Use-After-Free (Mungwall Sentinel)

**Enforcer Signature:** Address is a Mungwall sentinel value:
- `0xDEADBEEF` — freed memory (Mungwall fills freed blocks with this)
- `0xABADCAFE` — pre-fill pattern (Mungwall fills allocated blocks before first use)
- `0xCCCCCCCC` — uninitialized memory (some allocators use this)

**Hit Type:** Any read or write to a sentinel address.

**Root Cause:** A pointer to a freed memory block is used after `free()` was called, or a pointer to freshly allocated memory is used before initialization. Mungwall fills freed/allocated memory with sentinel values, turning silent corruption into a detectable Enforcer hit.

**Fix Template (use-after-free):**
```c
/* Before (use-after-free): */
free(node);
/* ... other code ... */
printf("%s\n", node->name);  /* CRASH: node memory is 0xDEADBEEF */

/* After: */
free(node);
node = NULL;  /* amiport: NULL after free to catch reuse */
/* ... other code ... */
if (node != NULL) {
    printf("%s\n", node->name);
}
```

**Fix Template (uninitialized):**
```c
/* Before (uninitialized pointer in struct): */
struct context ctx;
/* forgot to set ctx.buffer */
memcpy(ctx.buffer, src, len);  /* CRASH: ctx.buffer is 0xABADCAFE */

/* After: */
struct context ctx;
memset(&ctx, 0, sizeof(ctx));  /* amiport: zero-init struct */
ctx.buffer = malloc(len);
if (ctx.buffer == NULL) {
    fprintf(stderr, "error: out of memory\n");
    exit(20);  /* amiport: RETURN_FAIL */
}
memcpy(ctx.buffer, src, len);
```

**Diagnostic Clue:** The specific sentinel value identifies the category. `0xDEADBEEF` means the memory was freed — search for `free()` calls on that pointer. `0xABADCAFE` means the memory was allocated but never written — search for missing initialization.

**Example Port:** TBD

---

## 3. Stack Overflow

**Enforcer Signature:** Crash occurs during deep recursion or in a function with large local arrays. The Enforcer hit may show access to an address near the end of the stack region, or (more commonly) the crash manifests as a Guru Meditation (alert) rather than a clean Enforcer hit because the stack overflows into unmapped memory.

**Hit Type:** `Alert` (Guru Meditation), or `LONG-WRITE`/`LONG-READ` to an address near the task's stack base. Enforcer's `STACKCHECK` option (requires SegTracker) annotates stack longwords to help identify stack-related crashes.

**Root Cause:** The Amiga default stack is 4KB. Ported POSIX programs often assume much larger stacks (Linux default is 8MB). Deep recursion (e.g., directory traversal, expression parsing) or large local arrays (e.g., `char buf[PATH_MAX]` where `PATH_MAX` is 4096) overflow the stack.

**Fix Template (increase stack cookie):**
```c
/* Add or increase the stack cookie at file scope: */
long __stack = 65536;  /* amiport: 64KB stack for recursive program */

/* For extremely deep recursion, use 131072 (128KB) */
```

**Fix Template (reduce stack usage):**
```c
/* Before (large local array on stack): */
void process(void) {
    char buffer[8192];  /* 8KB on stack — dangerous */
    /* ... */
}

/* After (allocate on heap): */
void process(void) {
    char *buffer = malloc(8192);
    if (buffer == NULL) {
        fprintf(stderr, "error: out of memory\n");
        exit(20);  /* amiport: RETURN_FAIL */
    }
    /* ... */
    free(buffer);
}
```

**Diagnostic Clue:** If the crash happens in a recursive function or a function with large local variables, suspect stack overflow. Check the `__stack` cookie value — if it is missing (defaults to 4KB) or too small for the recursion depth, increase it. If the program uses `alloca()` or variable-length arrays, convert to heap allocation.

**Example Port:** TBD

---

## 4. Silent Wrong Results from st_dev/st_ino Comparison

**Enforcer Signature:** None — this bug produces **wrong output, not a crash**. No Enforcer hits. The program runs cleanly but gives incorrect results.

**Symptom:** Program treats different files as identical, skips comparisons, or reports "no differences" when files clearly differ.

**Root Cause:** Many POSIX programs compare `st_dev` and `st_ino` to detect whether two file paths refer to the same underlying file (hardlinks, symlinks). If the stat shim returned stub values (0 for both), all files on the same volume appeared to be the same file.

**Detection:** This is a **logic bug**, not a memory error. Enforcer won't catch it. Detect by:
- Running the program with files that should produce visible output and getting silence instead
- Searching the ported source for `st_dev` and `st_ino` comparisons
- `grep -n 'st_dev.*st_ino\|st_ino.*st_dev' ported/*.c`

**Fix:** Fixed at the shim level (March 2026) — `amiport_stat()` now populates:
- `st_ino` from `fib_DiskKey` (filesystem block number, unique per file on a volume)
- `st_dev` from `GetDeviceProc()->dvp_Port` (unique per mounted volume)

No per-port fix needed if using the current shim. For legacy ports built before this fix, add `#ifdef __AMIGA__` to skip the inode comparison.

**Programs Known to Be Affected:** Any program with `files_differ()`, `same_file()`, or similar logic: diff, cmp, cp, rsync, tar.

**Example Port:** diff (OpenBSD v1.95) — `diffreg.c:449`

## 5. vsnprintf(NULL, 0, ...) Crash

**Enforcer Signature:** `LONG-WRITE` to address `0x00000000` or very low address. Crash occurs inside `vsnprintf` or `vsprintf`.

**Symptom:** Program crashes immediately on startup or first string formatting operation. The crash happens inside a `vsnprintf(NULL, 0, fmt, ap)` call used to measure the required buffer size.

**Root Cause:** C99 specifies that `vsnprintf(NULL, 0, fmt, ap)` is valid and returns the number of characters that would have been written. However, libnix's `vsnprintf` does NOT support NULL as the destination buffer — it unconditionally writes to the pointer, causing a NULL dereference on the 68000.

This pattern commonly appears when replacing `vasprintf()` (which doesn't exist in libnix) with a two-pass vsnprintf approach. The code-transformer or developer writes `vsnprintf(NULL, 0, ...)` as the "measure" pass, not realizing libnix doesn't support it.

**Fix Template (use probe buffer):**
```c
/* WRONG — crashes on libnix: */
int len = vsnprintf(NULL, 0, fmt, ap);

/* CORRECT — use a probe buffer: */
char probe[1024];
int len = vsnprintf(probe, sizeof(probe), fmt, ap);
/* vsnprintf returns total chars that WOULD be written, even if truncated */
```

The 1024-byte probe buffer is sufficient because `vsnprintf` returns the full required length regardless of truncation. The subsequent allocation and second `vsnprintf` call will use the correct size.

**Detection:** Search for `vsnprintf(NULL` or `snprintf(NULL` in ported source:
```
grep -n 'vsnprintf(NULL\|snprintf(NULL' ported/*.c
```

**Programs Known to Be Affected:** Any program that uses `vasprintf()` or `asprintf()` — these are replaced during transformation and the replacement code must avoid the NULL pattern.

**Example Port:** diff (OpenBSD v1.95) — `xmalloc.c:xasprintf()`

---

## 6. amiport_fopen Sentinel BPTR Heap Corruption (MungWall)

- **Enforcer signature:** MungWall detects a corrupted allocation sentinel. Stack trace shows `malloc` / `fdopen` / `lseek` from libnix on top, with tail's code in the mid-stack. `Hunk 0002 Offset <bss+N>` in the MungWall report pointing at the amiport fd_table BSS. Guru Meditation #80000004 (Illegal Instruction) or #80000008 (Address Error) follows.
- **Root cause:** `amiport_fopen` (triggered by `#include <amiport/stdio.h>` + `#define fopen amiport_fopen`) calls the real libnix `fopen`, then allocates a **fake fd table slot** with sentinel BPTR value `(BPTR)1` (physical address 4). Mixing this sentinel into the fd_table BSS next to libnix's malloc arena causes MungWall to flag corrupted allocation sentinels when libnix's own `fdopen`/`malloc` next scans its heap structures. The crash is a heap integrity violation, not a direct NULL dereference.
- **Fix applied:** Remove `#include <amiport/stdio.h>` from files that use `amiport_stat(fname, ...)` instead of `fstat(fileno(fp), ...)`. The `amiport/stdio.h` shim was added to fix a fileno fd-table mismatch, but that mismatch is better avoided by using filename-based stat. For stdin pipe detection, replace `fstat(fileno(stdin)) + lseek()` with `IsInteractive(Input())` from `<proto/dos.h>`.
- **Port:** tail (OpenBSD v1.24)
- **Date:** 2026-03-21

## 7. vamos Stack Overflow — __stack Cookie Ignored

**Enforcer Signature:** `InvalidMemoryAccessError` with SP in the `0xffff0000` range (e.g., `SP=ffff0060`). Write addresses descend by 2 bytes (`ff005e, ff005c, ...ff0044`). The crash address in 24-bit mode appears as `0x00ff00xx` (wrap of `0xffff00xx`).

**Hit Type:** `WRITE` at descending addresses. Often crashes inside `err()`/`strerror()` or another deep call chain, but the crash location is a symptom — the stack was exhausted long before.

**Root Cause:** vamos has a hardcoded 8KB default stack (`amitools/vamos/cfg/proc.py: "stack": 8`). It does NOT read the `__stack` cookie from the binary's DATA segment. Any program declaring `__stack > 8192` that actually uses more than 8KB will silently overflow the stack, corrupt heap/BSS memory, and eventually wrap the 32-bit SP through zero into the `0xffff0000` range where no memory is mapped.

The overflow is silent because vamos maps all 1MB of RAM (`0x001000`–`0x100000`) as writable — the stack can corrupt heap and binary data without triggering an access error until SP wraps past `0x00000000`.

**Detection:** Check for the characteristic SP range `0xffff00xx` and descending write addresses. Also check if the binary declares `__stack`:
```
m68k-amigaos-objdump -t <binary> | grep __stack
```

**Fix Template (vamos wrapper):**
```bash
# Pass -s <KB> to vamos for programs needing large stacks
vamos -s 128 -- ./program args    # 128KB stack

# The toolchain/scripts/vamos wrapper defaults to 128KB automatically
```

**Fix Template (Makefile test target):**
For programs needing more than 128KB (e.g., diff with its recursive LCS algorithm):
```makefile
test:
	../../toolchain/scripts/vamos -s 192 -- ./$(TARGET) ...
```

**Programs Known to Be Affected:** Any port with `__stack > 8192` — especially recursive programs (diff, find), programs with large local arrays, and programs with deep call chains.

**Example Port:** diff (OpenBSD v1.95) — needed 96KB minimum, 192KB recommended.

---

## 8. FileInfoBlock Alignment — DOS Recoverable Alert (not Enforcer hit)

### Stack-allocated FileInfoBlock causes dos.library Address Error on 68000

- **Enforcer signature:** None — no Enforcer hits. The crash manifests as a **Recoverable Alert** with an address like `0x0100 000F` (type `0x0100` = recoverable, code `0x000F` = 15 = `ERROR_OBJECT_WRONG_TYPE` or address error in DOS). No memory access violation visible to Enforcer because the Address Error trap fires inside the ROM before Enforcer can intercept it.
- **Symptom:** Program crashes with "Error: 0100 000F Task: XXXXXXXX" immediately on file access. ARexx test harness shows `1..N` declared but zero `ok`/`not ok` lines — the binary crashed before producing any output.
- **Root cause:** `struct FileInfoBlock` (260 bytes) must be **longword-aligned** (4-byte boundary) when passed to `Examine()`, `ExamineFH()`, or `ExNext()`. The AmigaOS dos.library performs longword-sized reads on the FIB and will trap with an Address Error on a 68000 if the FIB is at an odd or word-aligned address. With `-m68000` and gcc's default stack layout, stack-allocated structs may only receive 2-byte alignment, causing this trap. The 68000 does not have an MMU, so Enforcer cannot see the crash — it fires as a CPU exception inside the ROM.
- **Fix applied:** Replace `struct FileInfoBlock fib;` (stack allocation) with `AllocDosObject(DOS_FIB, NULL)` in `amiport_stat()` (stat.c) and `amiport_fstat()` (file_io.c). `AllocDosObject` allocates from chip/fast RAM with guaranteed longword alignment and zero-initialises the FIB. Free with `FreeDosObject(DOS_FIB, fib)` when done. Note: `opendir()` in dir_ops.c uses `AllocVec(sizeof(AMIPORT_DIR), MEMF_PUBLIC|MEMF_CLEAR)` which already guarantees alignment — the FIB embedded in `struct _AMIPORT_DIR` is safe.
- **Detection grep:** `struct FileInfoBlock [a-z]` — any stack-allocated FIB in shim code.
- **Port:** tail (OpenBSD v1.24) — crash on first file argument (`WORK:tail WORK:test-tail-input.txt`)
- **Date:** 2026-03-21

---

### #9: libnix exit() hangs on AmigaOS console

- **Symptom:** Program produces correct output but never returns to the shell prompt. Process hangs indefinitely after last output line. Ctrl-C does not break. Not an Enforcer hit — no illegal memory access, just a deadlock.
- **Alert/Error:** None — no Guru Meditation, no Enforcer hits. The process simply never exits.
- **Trigger:** Calling `exit()` in `main()` after stdio output to an AmigaOS console window. Occurs on real hardware and FS-UAE, but NOT in vamos (vamos has its own exit handling).
- **Root cause:** libnix's `exit()` calls atexit handlers that attempt to flush and close all stdio streams. When stdout is connected to an AmigaDOS console (CON: or RAW:), the `fclose(stdout)` or internal stdio cleanup blocks waiting for the console handler to acknowledge. This is a libnix bug — the console handler may not respond to the close request if the process is already shutting down.
- **Fix applied:** Replace `exit(rval)` with `_exit(rval)` at the final exit point in `main()`. Add `fflush(stdout)` immediately before `_exit()` to ensure buffered output is written. `_exit()` bypasses atexit handlers and goes straight to `_exit()` → `Exit()` in AmigaDOS, which works correctly.
- **Detection grep:** `exit(rval)` or `exit(0)` at the end of `main()` — replace with `fflush(stdout); _exit(rval);`
- **Caveat:** Only use `_exit()` at the final exit point in `main()` after explicit cleanup (free, fclose). Do NOT use `_exit()` in error paths where `err()`/`errx()` are called — those still use `exit()` internally and the cleanup matters less for error exits (the program is dying anyway).
- **Port:** tail (OpenBSD v1.24) — hung after displaying output on FS-UAE console
- **Date:** 2026-03-21
