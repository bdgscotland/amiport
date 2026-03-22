---
name: Memory safety audit for tee.c port
description: Complete memory leak, double-free, and allocation safety analysis
type: project
---

# Memory Safety Review: tee (OpenBSD v1.15)

## Allocations Found

| Location | Type | Freed? | All paths? | Issue | Severity |
|----------|------|--------|------------|-------|----------|
| tee.c:82 | malloc(sizeof(*p)) in add() | No | N/A | LEAK — err() calls _exit(), list node never freed | CRITICAL |
| tee.c:155 | malloc(BSIZE) | Yes | Yes | OK — freed at line 174 before main exit | OK |
| tee.c:101 | amiport_expand_argv() | Yes | Yes | OK — freed at line 201 before exit | OK |
| tee.c:131,146 | add() calls in main | Partial | LEAK — error path at 141-144 | LEAK — file handle opened but add() not called, fd leaks | CRITICAL |

## Key Issues

### Issue 1: CRITICAL — add() malloc leaks on err() call (line 82-84)

**Code:**
```c
if ((p = malloc(sizeof(*p))) == NULL)
    err(10, NULL);  /* <-- err() calls _exit(10) */
```

**Problem:**
- When malloc fails, `err(10, NULL)` is called
- `err()` does not return — it calls `_exit()` internally
- The struct list node `p` never reaches a corresponding `free()`
- AmigaOS has no process cleanup, so this memory is permanently lost until reboot

**Impact:** If memory allocation fails during startup (lines 82, 131), the list node leaks forever.

**Fix Required:** Restructure `add()` to allocate first, validate separately, and return error status:
```c
static int
add(int fd, char *name)
{
    struct list *p;

    if ((p = malloc(sizeof(*p))) == NULL)
        return -1;  /* caller handles error */
    p->fd = fd;
    p->name = name;
    SLIST_INSERT_HEAD(&head, p, next);
    return 0;
}
```

Then in main:
```c
if (add(fd, *argv) == -1) {
    warn("malloc");
    amiport_close(fd);  /* close the file before bailing */
    exitval = 10;
    continue;
}
```

---

### Issue 2: CRITICAL — File handle leak on amiport_open() error (lines 140-147)

**Code:**
```c
if ((fd = amiport_open(*argv, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC))) == -1) {
    warn("%s", *argv);
    exitval = 10;
} else
    add(fd, *argv);  /* add() only called if open succeeds */
```

**Problem:**
- On error from `amiport_open()`, the file is never opened, so no leak here (fd == -1)
- BUT if `add(fd, *argv)` fails (Issue 1), the **fd is still open** and never closed
- When `add()` calls `err()` and exits, the fd list is incomplete, and the fd is never added to the list for cleanup

**Secondary Impact:** Lines 181-187 iterate over the linked list closing all file descriptors. If a file was opened but never added to the list (because add() failed), that fd remains open and leaks.

**Fix Required:** Change `add()` to return error status and close fd before exiting:
```c
if ((fd = amiport_open(*argv, ...)) == -1) {
    warn("%s", *argv);
    exitval = 10;
} else {
    if (add(fd, *argv) == -1) {
        warn("malloc");
        amiport_close(fd);  /* close before exiting */
        exitval = 10;
    }
}
```

---

### Issue 3: Minor — name pointer lifecycle in struct list

**Code:**
```c
p->name = name;  /* line 86 */
```

**Problem:**
- The `name` field points to `*argv`, which is NOT owned by the struct
- When `amiport_free_argv()` is called at line 201, it may free the argv array
- If the argv pointers are freed before the list cleanup (lines 194-197), the `name` pointers become stale
- **However**, checking the order: list is freed (194-197), THEN argv is freed (201), so this is OK

**Status:** Safe because cleanup order is correct. But **fragile** — order matters.

---

## Exit Path Analysis

### Normal exit (main() success path)
1. Buffer malloc'd at line 155 ✓
2. Buffer freed at line 174 ✓
3. File descriptors closed at lines 183-187 ✓
4. List nodes freed at lines 194-197 ✓
5. argv freed at line 201 ✓
6. fflush(stdout) at line 204 ✓
7. _exit(exitval) at line 205 ✓

**Verdict:** Cleanup order is correct IF all list nodes are successfully added.

### Error path: malloc(sizeof(*p)) fails
- Line 82-84: `err()` called → _exit() → **No cleanup**
- List node allocated but never freed
- Previous list nodes (if any) allocated but never freed

### Error path: amiport_open() fails
- Line 140-144: `warn()` called, `continue` to next file
- File not opened, no fd leak
- But if the **next** add() call fails before list cleanup, fds leak

### Error path: add(fd, *argv) fails (if changed to return -1)
- Currently impossible (add() calls err() on malloc failure)
- With proposed fix: fd is opened but not added to list
- Must close fd before returning from loop

---

## Verdict

### Current Code: UNSAFE — CRITICAL LEAKS

**Status:** NEEDS FIXES

**Root Causes:**
1. `add()` calls `err()` on malloc failure instead of returning error
2. No way to handle malloc failure in add() without exiting
3. No error recovery if add() fails mid-loop

**Summary:**
- Total allocations: 3 (list nodes on each file, buffer, argv)
- Properly freed on all paths: **NO** — list nodes on malloc failure leak
- Leaks found: **1 CRITICAL** (list node per failed malloc)
- Double-free risks: 0
- File handle leaks: **1 CRITICAL** (if add fails, next fd opened but not in list)
- Verdict: **NEEDS FIXES BEFORE AMINET RELEASE**

---

## Fixes Required

### Fix 1: Refactor add() to return error status
**File:** `ports/tee/ported/tee.c:78-88`

Change from void function to int return type. Return -1 on malloc failure, 0 on success.

### Fix 2: Handle add() errors in main()
**File:** `ports/tee/ported/tee.c:131-147`

- At line 131: Check `add(STDOUT_FILENO, "stdout")` return value
- At lines 140-146: Check `add(fd, *argv)` return value, close fd if it fails

### Fix 3: Verify cleanup order
**File:** `ports/tee/ported/tee.c:194-205`

- List cleanup (194-197) happens BEFORE argv cleanup (201) ✓
- This is correct — do not change

---

## Severity Assessment

| Issue | Type | Impact | Likelihood |
|-------|------|--------|------------|
| List node leak on malloc | CRITICAL | Permanent memory loss until reboot | LOW (only on malloc failure) |
| File handle leak | CRITICAL | Permanent fd loss, future files can't open | MEDIUM (if add() fails) |

**Overall Risk:** High — while malloc failures are rare, on AmigaOS with extreme memory constraints, they must be handled gracefully.

---

## Implementation Notes

- err() and warn() are BSD functions that may call exit() or continue respectively
- The current code assumes err() never returns, so no cleanup after it is attempted
- With proper error handling, add() failure should not be fatal — log warning, close fd, continue
- The linked list cleanup at the end is good defensive programming but only works if all allocations succeeded

