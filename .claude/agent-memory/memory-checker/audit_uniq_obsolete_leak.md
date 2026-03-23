---
name: uniq obsolete() malloc leak pattern
description: OpenBSD uniq 1.33 leaks malloc'd strings from old-style option transformation — pattern appears in tail, tr, and other ports with obsolete() function
type: reference
---

# obsolete() malloc Leak Pattern

## The Problem

When porting OpenBSD utilities, many have an `obsolete()` function that transforms old-style numeric options (like `uniq +2` or `tail -3n`) into modern getopt-compatible syntax.

The function typically:
1. Walks argv array
2. Detects old-style options (e.g., `+N` or `-N` where N is a digit)
3. Malloc's a new buffer
4. Constructs new option string (e.g., `-f N`)
5. **Overwrites argv[i] pointer with malloc'd buffer**
6. **Never tracks the allocation** — the malloc'd memory is lost when the function returns

## Example from uniq.c:296

```c
void obsolete(char *argv[]) {
    size_t len;
    char *ap, *p, *start;

    while ((ap = *++argv)) {
        if (ap[0] != '-') {
            if (ap[0] != '+')
                return;
        } else if (ap[1] == '-')
            return;
        if (!isdigit((unsigned char)ap[1]))
            continue;
        /* Old-style option detected — transform it */
        len = strlen(ap) + 3;
        if ((start = p = malloc(len)) == NULL)
            err(10, "malloc");  /* EXITS HERE — malloc not freed */
        *p++ = '-';
        *p++ = ap[0] == '+' ? 's' : 'f';
        (void)strlcpy(p, ap + 1, len - 2);
        *argv = start;  /* Overwrite argv[i] with malloc'd pointer */
        /* No tracking of 'start' — pointer is lost */
    }
}
```

## Why It Leaks

1. **No tracking mechanism** — obsolete() doesn't record which argv entries are malloc'd
2. **Argv mutation** — The original argv array is modified in-place with malloc'd pointers
3. **No cleanup path** — The allocated memory is never freed
4. **atexit() insufficient** — `amiport_free_argv()` only frees the expanded argv structure, not the data that obsolete() malloc'd (separate allocation pool)
5. **Early exits** — err()/errx()/usage() calls terminate before cleanup code could run

## Exit Path Leak

Every exit path after `obsolete()` is called (line 111 in uniq) leaks:
- err() at lines 109, 151, 155, 166, 207
- errx() at lines 123, 131
- usage() at lines 137, 148
- Normal exit at line 213

## Impact Assessment

| Case | Leak Size | Frequency |
|------|-----------|-----------|
| No old-style options | 0 | Common — modern scripts use `-f N` |
| One old-style option | ~15 bytes | Rare |
| Five old-style options | ~75 bytes | Very rare |

**Permanent on AmigaOS:** With `-noixemul`, every byte leaks until reboot. No automatic cleanup.

## Fix Strategy

**Option 1: Track allocations (Most robust)**

Add static struct in obsolete() to track malloc'd pointers:

```c
static struct {
    char **ptrs;
    int count;
    int capacity;
} obsolete_allocs = {NULL, 0, 0};

/* In obsolete() when malloc is called: */
if ((start = p = malloc(len)) == NULL)
    err(10, "malloc");

/* Track it: */
if (obsolete_allocs.count >= obsolete_allocs.capacity) {
    obsolete_allocs.capacity = obsolete_allocs.capacity ?
        obsolete_allocs.capacity * 2 : 10;
    char **tmp = realloc(obsolete_allocs.ptrs,
        obsolete_allocs.capacity * sizeof(char *));
    if (tmp == NULL) err(10, "malloc");
    obsolete_allocs.ptrs = tmp;
}
obsolete_allocs.ptrs[obsolete_allocs.count++] = start;

/* Add cleanup function */
static void cleanup_obsolete(void) {
    if (obsolete_allocs.ptrs) {
        for (int i = 0; i < obsolete_allocs.count; i++)
            free(obsolete_allocs.ptrs[i]);
        free(obsolete_allocs.ptrs);
        obsolete_allocs.ptrs = NULL;
    }
}

/* Call from main's cleanup() before atexit cleanup */
```

**Option 2: Use static buffer (Simpler but risky)**

Replace malloc with static buffer (works only if at most one old-style option):

```c
static char buf[256];
p = buf;
/* ... populate buf ... */
*argv = buf;
```

Risk: If two old-style options appear, the second overwrites the first's buffer before it's processed by getopt.

**Option 3: Use alloca() (Stack allocation)**

```c
start = p = alloca(len);
```

Works but uses stack space. Less safe if malloc failure handling is important.

## Affected Ports

- **tail** — reallocarray never freed, obsolete() malloc leaks
- **uniq** — obsolete() malloc leaks (verified 2026-03-22)
- **tr** — may have obsolete() function (needs audit)
- Any OpenBSD utility with `obsolete()` function

## Code-Transformer Notes

When fixing these ports:
1. Add tracking struct inside obsolete() or above it
2. Add cleanup_obsolete() function
3. Call cleanup_obsolete() from main's cleanup() or atexit handler
4. Test with repeated invocations to verify no accumulation

## Prevention

For future ports with obsolete() function:
1. Flag during source-analyzer stage
2. Require cleanup code in code-transformer stage
3. Memory audit is mandatory (as always)
4. FS-UAE test with old-style options if supported

---

**Last Updated:** 2026-03-22 (uniq 1.33 audit)
