---
name: memory-audit-awk
description: ports/awk 2024.12.25 memory safety review - CRITICAL LEAKS on error paths and global state
type: project
---

# Memory Safety Audit: ports/awk (2024.12.25)

**Status: CRITICAL LEAKS FOUND**
**Verdict: Cannot ship — 5 critical issues unfixable without code changes**

## Summary

BWK "One True Awk" is a full scripting interpreter with extensive dynamic memory usage:
- Symbol tables (symtab, ARGV, ENVIRON, user arrays)
- Field arrays (fldtab, NF tracking)
- File descriptor table (files array for >, >>, |)
- Regex NFA compilation (parse trees, finite automata)
- Temporary cell pool (runtime expression evaluation)
- Parse tree nodes and function frames
- Pattern-action program structure

The port is clean on successful execution paths. However, it leaks memory on:
1. Early exit from main() before run() (pfile array, symtab)
2. FATAL() errors during compilation (parse tree, symtab not freed)
3. FATAL() errors during execution (files array, frame stack, temp cells)
4. stdinit() failure (no cleanup of partial allocations)
5. Regex compilation failure in makedfa() (NFA not freed on FATAL)

FATAL() is `noreturn` — it calls `exit(2)` directly without cleanup.

## Allocations Found

| Location | Type | Freed? | All Paths? | Issue |
|----------|------|--------|------------|-------|
| main.c:217 | realloc(pfile) | No | No — exits before return | **CRITICAL: pfile leak on FATAL during -f parsing** |
| lib.c:67-70 | malloc/calloc (record, fields, fldtab) | Yes (atexit?) | No — FATAL exits before cleanup | **CRITICAL: recinit allocations not freed on error** |
| tran.c:163-164 | malloc/calloc (symtab table) | Yes | No — FATAL during syminit/arginit exits | **CRITICAL: symtab leaks on compilation error** |
| run.c:2268 | calloc(files) | No | No — FATAL exits, closeall() unreached | **CRITICAL: files array never freed** |
| run.c:267, 301 | calloc/realloc (frame stack) | No | No — FATAL exits | **CRITICAL: function frame stack leaks on error** |
| b.c:240, 156-166 | calloc/realloc (fa struct) | Yes | No — FATAL on NFA construction | **CRITICAL: regex NFA leaks when makedfa() hits FATAL** |
| run.c:965 | calloc(tmps pool) | No | No — FATAL exits | **CRITICAL: temp cell pool leaks** |
| tran.c:237-238 | malloc (Cell->nval/sval strings) | Yes | No — setsymtab() FATAL exits without freeing cells | **CRITICAL: symbol names/values leak on OOM** |

## Critical Leaks Detailed

### 1. pfile Array — main.c:217

**Allocation:**
```c
static char **pfile;        /* line 58 */
static size_t maxpfile;

if (npfile >= maxpfile) {
    maxpfile += 20;
    pfile = (char **) realloc(pfile, maxpfile * sizeof(*pfile));  /* line 217 */
    if (pfile == NULL)
        FATAL("error allocating space for -f options");  /* line 219 — exits without freeing */
}
pfile[npfile++] = fn;  /* line 221 */
```

**Issue:** `pfile` is a global array that accumulates option filenames from multiple `-f file1 -f file2` arguments. If realloc fails, or if any FATAL error occurs after pfile is allocated (during compilation at line 270), the array is never freed.

**Paths that leak:**
- FATAL at line 219 (realloc OOM) — pfile + accumulated filenames lost
- FATAL at line 231 (invalid -v option) — pfile + old filenames lost
- FATAL at line 255 (no program) — pfile lost
- FATAL at line 298 (can't open -f file) — pfile + open success lost
- Syntax error during yyparse (line 270) — pfile never freed before returning

**Impact:** Small leak (20-200 bytes per -f option, rare multi-file invocations), but permanent until reboot.

**Fix required:** Add atexit cleanup:
```c
static void cleanup_pfile(void) {
    if (pfile) { free(pfile); pfile = NULL; }
}
/* After line 191 in main(): */
atexit(cleanup_pfile);
```

---

### 2. Record/Field Arrays — lib.c:67-70

**Allocation:**
```c
void recinit(unsigned int n) {
    if ( (record = (char *) malloc(n)) == NULL
      || (fields = (char *) malloc(n+1)) == NULL
      || (fldtab = (Cell **) calloc(nfields+2, sizeof(*fldtab))) == NULL
      || (fldtab[0] = (Cell *) malloc(sizeof(**fldtab))) == NULL)
        FATAL("out of space for $0 and fields");  /* line 71 */
    /* ... fldtab[i] allocations in makefields() ... */
}
```

**Issue:** recinit() is called at main.c:262 with global `record`, `fields`, `fldtab[0..nfields]`. If any allocation fails, FATAL() exits. These buffers are used for every input record during execution. They're never explicitly freed — the port assumes process exit auto-frees.

On AmigaOS with `-noixemul`, exit() does NOT trigger atexit handlers if malloc/free is corrupted or if the C runtime hasn't finished initialization.

**Paths that leak:**
- OOM during recinit (line 71) — all 4 allocations + prior fldtab entries lost
- FATAL during arginit (line 267) — recinit buffers lost
- FATAL during yyparse (line 270) — recinit buffers lost
- FATAL during execution (run.c:execute) — recinit buffers lost
- Successful execution with FATAL during closeall() (run.c:2414) — buffers not freed before exit

**Impact:** ~16-32 KB per invocation (RECSIZE=8192, plus field array).

**Fix required:** Add atexit cleanup:
```c
static void cleanup_records(void) {
    size_t i;
    if (record) { free(record); record = NULL; }
    if (fields) { free(fields); fields = NULL; }
    if (fldtab) {
        for (i = 0; i <= nfields; i++)
            if (fldtab[i]) { free(fldtab[i]); fldtab[i] = NULL; }
        free(fldtab);
        fldtab = NULL;
    }
}
/* In recinit() after successful allocation: */
atexit(cleanup_records);
```

But this is RISKY: fldtab entries are created dynamically in makefields(), and we don't track all allocated entries. See issue #5 below.

---

### 3. Symbol Table — tran.c:163-164, 224-236

**Allocation:**
```c
Array *makesymtab(int n) {
    Array *ap;
    Cell **tp;
    ap = (Array *) malloc(sizeof(*ap));      /* line 163 */
    tp = (Cell **) calloc(n, sizeof(*tp));   /* line 164 */
    if (ap == NULL || tp == NULL)
        FATAL("out of space in makesymtab");  /* line 166 — exits without freeing */
    /* ... */
    return(ap);
}

Cell *setsymtab(const char *n, const char *s, Awkfloat f, unsigned t, Array *tp) {
    Cell *p = (Cell *) malloc(sizeof(*p));  /* line 234 */
    if (p == NULL)
        FATAL("out of space for symbol table at %s", n);  /* line 236 — exits */
    p->nval = tostring(n);   /* malloc'd name */
    p->sval = tostring(s);   /* malloc'd string */
    /* ... */
}
```

**Issue:** symtab is created at main.c:191 (`symtab = makesymtab(...)`). If symtab allocation fails, FATAL exits. After successful creation, setsymtab() is called for every built-in variable and user variable. If setsymtab() allocation fails at any point, FATAL exits without freeing the partially-built symtab or the Cell strings allocated via tostring().

The symtab is freed via freesymtab() (tran.c:173), but only if the program completes. On FATAL during compilation or execution, freesymtab() is never called.

**Paths that leak:**
- OOM in makesymtab (line 166) — Array + hash table lost
- OOM in setsymtab (line 236) — Cell + nval string + sval string lost + all prior table entries
- FATAL during arginit (line 267 in main) — entire symtab lost
- FATAL during yyparse (line 270) — symtab lost
- FATAL during run() — symtab lost

**Impact:** Unbounded (depends on user variables in awk program). Typical: 1-5 KB for built-in symbols + user symbols.

**Fix required:** Add atexit cleanup:
```c
static void cleanup_symtab(void) {
    if (symtab) {
        freesymtab(symtabloc);  /* symtabloc is a Cell pointing to symtab */
        symtab = NULL;
    }
}
/* In main() after symtab = makesymtab(...): */
atexit(cleanup_symtab);
```

This is RISKY: freesymtab() has assertions about element counts (line 195-196). If the symtab is incomplete due to OOM, the count check may fail and trigger WARNING, but exit continues.

---

### 4. Files Array — run.c:2268, 2309

**Allocation:**
```c
struct files {
    FILE *fp;
    const char *fname;
    int mode;
} *files;

static void stdinit(void) {
    nfiles = FOPEN_MAX;
    files = (struct files *) calloc(nfiles, sizeof(*files));  /* line 2268 */
    if (files == NULL)
        FATAL("can't allocate file memory for %lu files", ...);  /* exits without freeing */
    files[0].fname = tostring("/dev/stdin");   /* malloc'd strings */
    files[1].fname = tostring("/dev/stdout");
    files[2].fname = tostring("/dev/stderr");
}

FILE *openfile(int a, const char *us, bool *pnewflag) {
    /* ... on growth: */
    nf = (struct files *) realloc(files, nnf * sizeof(*nf));  /* line 2309 */
    if (nf == NULL)
        FATAL("cannot grow files for %s", ...);  /* exits without freeing */
    files = nf;
}
```

**Issue:** stdinit() is called at run.c:163 during run(). It allocates the files array and initializes 3 entries with malloc'd fname strings. Later, openfile() grows the array as needed. If any FATAL error occurs (malloc failure, open failure at line 2337), the files array and all accumulated fname strings are leaked.

closeall() (run.c:2401) frees the fname strings via loop iterations, but it's only called at the end of run() — if run() never returns (FATAL exit), closeall() is never reached.

**Paths that leak:**
- OOM in stdinit (line 2270) — files array + 3 fname strings lost
- OOM in openfile grow (line 2311) — files array + all fname strings lost
- FATAL during file open (line 2252) — files array + fname strings lost
- FATAL in dosub/pclose (line 2414, 2422) — files array never freed before exit
- FATAL during expression evaluation — files array leaked

**Impact:** ~1-2 KB per invocation (FOPEN_MAX=20 entries, each fname is ~20-30 bytes).

**Fix required:** Add atexit cleanup:
```c
static void cleanup_files(void) {
    size_t i;
    if (files) {
        for (i = 0; i < nfiles; i++) {
            if (files[i].fname) { free((void*)(intptr_t)files[i].fname); }
            if (files[i].fp && files[i].fp != stdin && files[i].fp != stdout && files[i].fp != stderr)
                fclose(files[i].fp);
        }
        free(files);
        files = NULL;
    }
}
/* In stdinit() after successful calloc: */
atexit(cleanup_files);
```

But this CLOSES files on atexit, which may cause I/O errors if called during normal exit (double-close). Safe approach: only free the array, call closeall() explicitly in main() before returning, and add cleanup for the case where FATAL is called.

---

### 5. Function Frame Stack — run.c:267, 301

**Allocation:**
```c
struct Frame *frame = NULL;
int nframe = 0;

Cell *call(Node **a, int n) {
    if (frame == NULL) {
        frp = frame = (struct Frame *) calloc(nframe += 100, sizeof(*frame));  /* line 267 */
        if (frame == NULL)
            FATAL("out of space for stack frames calling %s", s);  /* exits */
    }
    if (frp >= frame + nframe) {
        frame = (struct Frame *) realloc(frame, (nframe += 100) * sizeof(*frame));  /* line 301 */
        if (frame == NULL)
            FATAL("out of space for stack frames in %s", s);  /* exits */
        frp = frame + dfp;
    }
}
```

**Issue:** call() allocates and grows the frame stack dynamically. If any allocation fails, FATAL exits. The frame stack is never explicitly freed — it persists until process exit.

Frames contain Cell* arrays and tempcel entries. If a function call fails mid-way, those allocations are never freed.

**Paths that leak:**
- OOM in call (line 269) — frame + all accumulated frame data lost
- OOM in call realloc (line 303) — frame + all prior frames lost
- FATAL during nested function call — frame stack leaked

**Impact:** ~400+ bytes per NARGS=50 frame (50 Cell* slots per frame).

**Fix required:** Add atexit cleanup:
```c
static void cleanup_frames(void) {
    if (frame) { free(frame); frame = NULL; nframe = 0; frp = NULL; }
}
/* In call() after frame allocation: */
atexit(cleanup_frames);
```

---

### 6. Regex NFA — b.c:240, 156-166

**Allocation:**
```c
fa *makedfa(const char *s, bool anchor) {
    fa *f;
    /* ... NFA construction ... */
    if ((f = (fa *) calloc(1, sizeof(fa) + poscnt * sizeof(rrow))) == NULL)  /* line 240 */
        FATAL("malloc NFA for re %s, size %zu", s, sizeof(fa) + poscnt * sizeof(rrow));  /* exits */
    /* ... build NFA: */
    f->gototab[i].entries = (gtte *) calloc(NCHARS, sizeof(gtte));  /* line 172 */
    /* ... realloc patterns */
    setvec = (int *) realloc(setvec, maxsetvec * sizeof(*setvec));  /* line 137 */
}
```

**Issue:** makedfa() is called during program compilation (awkgram.tab.c:2901, etc.) to build regex NFAs. If allocation fails at any point, FATAL is called. The partially-constructed fa struct with its gototab entries is not freed.

The static setvec array (line 137) is a global buffer that's grown as needed. If realloc fails, setvec leaks.

freefa() (b.c:1560) frees the fa, but it's only called when the fa is explicitly deleted or when the program completes.

**Paths that leak:**
- OOM in makedfa (line 240) — fa + all gototab entries + setvec lost
- OOM in gototab growth (line 615) — partial fa lost
- Syntax error after makedfa (line 270 in main) — all compiled NFA structures lost

**Impact:** Variable (depends on regex complexity and count). Typical: 1-5 KB per regex.

**Fix required:** Track fa allocations in a cleanup function:
```c
#define MAX_FA_PTRS 100
static fa *fa_ptrs[MAX_FA_PTRS];
static int fa_count = 0;

/* In makedfa() before FATAL: */
if (f == NULL) {
    if (fa_count < MAX_FA_PTRS)
        fa_ptrs[fa_count++] = f;  /* saves NULL but marks position */
    FATAL(...);
}
fa_ptrs[fa_count++] = f;  /* save successful allocations */

static void cleanup_fas(void) {
    int i;
    for (i = 0; i < fa_count; i++)
        if (fa_ptrs[i]) freefa(fa_ptrs[i]);
}
/* After stdinit in run.c or in main(): */
atexit(cleanup_fas);
```

This is RISKY: fNAs are freed when programs complete normally (via winner tree cleanup), so double-freeing could occur.

---

### 7. Temporary Cell Pool — run.c:965

**Allocation:**
```c
Cell *gettemp(void) {
    static int ngettemp = 0;
    if (tmps == NULL || ngettemp > NTEMPS) {
        tmps = (Cell *) calloc(100, sizeof(*tmps));  /* line 965 */
        if (tmps == NULL)
            FATAL("out of memory");  /* exits */
        /* ... */
    }
}
```

**Issue:** The temp cell pool is lazily allocated during execution. If allocation fails, FATAL exits. The pool is never explicitly freed — it persists until process exit.

**Paths that leak:**
- OOM in gettemp (line 965) — tmps pool + all accumulated temp cells lost
- FATAL during expression evaluation — tmps pool leaked

**Impact:** Small (100 Cell structs = ~2-3 KB).

**Fix required:** Add atexit cleanup:
```c
extern Cell *tmps;  /* from run.c */
static void cleanup_tmps(void) {
    if (tmps) { free(tmps); tmps = NULL; }
}
/* In gettemp() after allocation: */
atexit(cleanup_tmps);
```

---

## Summary Table

| Issue | Type | Location | Size | Severity | Unfixable? |
|-------|------|----------|------|----------|------------|
| pfile array never freed | malloc/realloc | main.c:217 | 20-200 B | CRITICAL | No — add atexit cleanup |
| record/fields/fldtab never freed | malloc/calloc | lib.c:67-70 | 16-32 KB | CRITICAL | No — but risky (untracked entries) |
| symtab never freed | malloc/calloc | tran.c:163-164 | 1-5 KB | CRITICAL | No — but risky (incomplete table) |
| files array never freed | calloc/realloc | run.c:2268 | 1-2 KB | CRITICAL | No — but risky (closes files) |
| frame stack never freed | calloc/realloc | run.c:267 | 400+ B | CRITICAL | No — add atexit cleanup |
| regex NFA never freed | calloc/realloc | b.c:240 | 1-5 KB | CRITICAL | No — but risky (double-free) |
| temp cell pool never freed | calloc | run.c:965 | 2-3 KB | CRITICAL | No — add atexit cleanup |

**Total permanent leak per invocation:** ~25-50 KB (worst case with large programs and many files/regexes)

## Verdict

**CANNOT SHIP** — 5-7 critical memory leaks unfixable without code changes.

The leaks are caused by the original upstream design: awk was written for Unix systems where process exit automatically frees all memory. On AmigaOS with `-noixemul`, every allocation must be manually freed or registered with atexit().

Recommended fixes:
1. Add atexit cleanup functions for pfile, symtab, frame stack, temp cells
2. For record/fields/fldtab: add cleanup but accept 200-byte leak (uninitialized fldtab[i] pointers)
3. For files array: add cleanup but ensure closeall() is called before atexit
4. For regex NFA: track allocations separately or accept leak (rarely 5+ regexes per program)

Estimated effort: 50-100 lines of cleanup code across 3-4 files (main.c, lib.c, run.c, b.c).

This is NOT a blocker if the user accepts the 25-50 KB leak per invocation. For Aminet publishing, I recommend fixing at least issues 1, 3, and 5 (highest impact).
