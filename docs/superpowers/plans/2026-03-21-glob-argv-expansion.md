# Glob & Argv Wildcard Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add POSIX glob()/globfree() and automatic argv wildcard expansion to the posix-shim library, matching classic Amiga C compiler conventions.

**Architecture:** Two-layer design in `lib/posix-shim/`. Layer 1: POSIX `glob()` built on existing `amiport_opendir()`/`amiport_readdir()` + `amiport_fnmatch()`, with an Amiga-to-Unix pattern translation layer. Layer 2: `amiport_expand_argv()` called explicitly at top of `main()`, with `int __nowild = 1;` opt-out matching SAS/C convention. Shared `glob_has_magic()` helper in `internal.h` avoids duplication.

**Tech Stack:** C89, bebbo-gcc (m68k-amigaos-gcc), libnix, vamos for testing.

**Spec:** `docs/superpowers/specs/2026-03-21-glob-argv-expansion-design.md`

---

## File Map

```
lib/posix-shim/
├── include/amiport/
│   ├── glob.h              ← NEW: glob_t, GLOB_* constants, amiport_glob/globfree macros
│   └── internal.h          ← MODIFY: add glob_has_magic() declaration
├── src/
│   ├── glob.c              ← NEW: glob(), globfree(), Amiga→Unix pattern translation
│   └── argv_expand.c       ← NEW: expand_argv(), free_argv(), __nowild check
└── Makefile                ← MODIFY: add glob.c, argv_expand.c to SRCS

tests/shim/
├── test_glob.c             ← NEW: glob API + pattern translation tests
├── test_argv_expand.c      ← NEW: argv expansion unit + integration tests
└── Makefile                ← MODIFY: add test_glob, test_argv_expand to TESTS

Agent/skill/doc updates:
├── .claude/skills/transform-source/references/transformation-rules.md  ← MODIFY
├── .claude/skills/analyze-source/references/common-patterns.md         ← MODIFY
├── docs/posix-tiers.md                                                 ← MODIFY
├── docs/references/newlib-availability.md                              ← MODIFY
├── CLAUDE.md                                                           ← MODIFY
└── TODOS.md                                                            ← MODIFY
```

---

### Task 1: Header and Shared Helper

**Files:**
- Create: `lib/posix-shim/include/amiport/glob.h`
- Modify: `lib/posix-shim/include/amiport/internal.h`

- [ ] **Step 1: Create `amiport/glob.h`**

```c
/*
 * amiport/glob.h -- POSIX glob/globfree for AmigaOS
 *
 * Provides glob() for expanding wildcard patterns against the filesystem.
 * Supports both Unix (*, ?, [...]) and AmigaOS (#?, ~(...), (a|b)) patterns.
 * Written in C89 for AmigaOS 3.x compatibility.
 */

#ifndef AMIPORT_GLOB_H
#define AMIPORT_GLOB_H

#include <stdlib.h>  /* for size_t */

/* Flags */
#define GLOB_ERR       (1 << 0)  /* Return on read errors */
#define GLOB_MARK      (1 << 1)  /* Append / to directory names */
#define GLOB_NOSORT    (1 << 2)  /* Don't sort results */
#define GLOB_NOCHECK   (1 << 4)  /* Return pattern if no matches */
#define GLOB_APPEND    (1 << 5)  /* Append to existing results */

/* Return values */
#define GLOB_NOSPACE   1  /* Memory allocation failed */
#define GLOB_ABORTED   2  /* Read error (and GLOB_ERR set) */
#define GLOB_NOMATCH   3  /* No matches found */

typedef struct {
    size_t   gl_pathc;   /* Count of matched paths */
    char   **gl_pathv;   /* List of matched paths */
    size_t   gl_offs;    /* Reserved slots at start of gl_pathv (unused, always 0) */
} amiport_glob_t;

int  amiport_glob(const char *pattern, int flags,
                  int (*errfunc)(const char *, int),
                  amiport_glob_t *pglob);
void amiport_globfree(amiport_glob_t *pglob);

#ifndef AMIPORT_NO_GLOB_MACROS
#define glob_t    amiport_glob_t
#define glob      amiport_glob
#define globfree  amiport_globfree
#endif

#endif /* AMIPORT_GLOB_H */
```

- [ ] **Step 2: Add `glob_has_magic()` to `internal.h`**

Add before the closing `#endif`:

```c
/*
 * Check if a string contains wildcard/glob metacharacters.
 * Detects both Unix (*, ?, [) and AmigaOS (#, ~, () patterns.
 * Returns 1 if wildcards found, 0 if literal.
 *
 * Used by glob.c and argv_expand.c — single source of truth.
 */
int amiport_glob_has_magic(const char *pattern);
```

- [ ] **Step 3: Commit**

```bash
git add lib/posix-shim/include/amiport/glob.h lib/posix-shim/include/amiport/internal.h
git commit -m "feat: add glob.h header and glob_has_magic declaration"
```

---

### Task 2: glob() Implementation

**Files:**
- Create: `lib/posix-shim/src/glob.c`
- Modify: `lib/posix-shim/Makefile`

- [ ] **Step 1: Create `glob.c`**

```c
/*
 * glob.c -- POSIX glob/globfree for AmigaOS
 *
 * amiport: implements glob() using amiport_opendir/readdir + amiport_fnmatch.
 * Supports both Unix and AmigaOS wildcard patterns via translation layer.
 * Written in C89 for AmigaOS 3.x compatibility.
 */

#include <amiport/glob.h>
#include <amiport/fnmatch.h>
#include <amiport/dirent.h>
#include <amiport/internal.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define GLOB_INITIAL_SIZE 32
#define GLOB_TRANSLATE_BUF 512

/* --- Amiga-to-Unix pattern translation --- */

/*
 * Translate AmigaOS pattern metacharacters to Unix equivalents.
 * #? -> *, #x -> x* (repeat), (a|b) passed through for iterative matching.
 * Returns 1 if translation was performed, 0 if pattern was already Unix.
 * Result written to 'out' buffer of size 'outsize'.
 */
static int
translate_amiga_pattern(const char *pattern, char *out, size_t outsize)
{
    const char *p = pattern;
    char *o = out;
    char *end = out + outsize - 1;
    int translated = 0;

    while (*p != '\0' && o < end) {
        if (p[0] == '#' && p[1] == '?') {
            /* #? -> * */
            *o++ = '*';
            p += 2;
            translated = 1;
        } else if (p[0] == '#' && p[1] != '\0' && p[1] != '#') {
            /* #x -> x* (match x zero or more times — approximate as x*) */
            if (o + 1 < end) {
                *o++ = p[1];
                *o++ = '*';
            }
            p += 2;
            translated = 1;
        } else {
            *o++ = *p++;
        }
    }
    *o = '\0';
    return translated;
}

/*
 * amiport_glob_has_magic — shared wildcard detection.
 * Checks for Unix (*, ?, [) and AmigaOS (#, ~, () metacharacters.
 */
int
amiport_glob_has_magic(const char *pattern)
{
    const char *p;
    if (pattern == NULL)
        return 0;
    for (p = pattern; *p != '\0'; p++) {
        switch (*p) {
        case '*':
        case '?':
        case '[':
            return 1;
        case '#':
            /* # followed by ? or a char is an Amiga pattern */
            if (p[1] != '\0')
                return 1;
            break;
        case '~':
            /* ~ followed by ( is Amiga negation */
            if (p[1] == '(')
                return 1;
            break;
        case '(':
            /* (a|b) alternation — check for | inside */
            {
                const char *q;
                for (q = p + 1; *q != '\0' && *q != ')'; q++) {
                    if (*q == '|')
                        return 1;
                }
            }
            break;
        case '\\':
            /* Skip escaped character */
            if (p[1] != '\0')
                p++;
            break;
        }
    }
    return 0;
}

/*
 * Split pattern into directory and filename components.
 * "src/*.c"     -> dir="src",  pat="*.c"
 * "*.c"         -> dir=".",    pat="*.c"  (or "" on AmigaOS)
 * "Work:src/*.c"-> dir="Work:src", pat="*.c"
 *
 * Returns pointer to filename part; writes directory to 'dir_buf'.
 */
static const char *
split_pattern(const char *pattern, char *dir_buf, size_t dir_size)
{
    const char *last_sep = NULL;
    const char *last_colon = NULL;
    const char *p;

    /* Find last / or : separator */
    for (p = pattern; *p != '\0'; p++) {
        if (*p == '/')
            last_sep = p;
        else if (*p == ':')
            last_colon = p;
    }

    /* Use whichever separator came last */
    if (last_sep != NULL && (last_colon == NULL || last_sep > last_colon)) {
        /* Unix-style path separator */
        size_t len = (size_t)(last_sep - pattern);
        if (len == 0) {
            /* Pattern like "/*.c" — root, but on Amiga this is relative */
            strncpy(dir_buf, ".", dir_size - 1);
        } else {
            if (len >= dir_size) len = dir_size - 1;
            memcpy(dir_buf, pattern, len);
            dir_buf[len] = '\0';
        }
        return last_sep + 1;
    } else if (last_colon != NULL) {
        /* Amiga volume/assign: "Work:*.c" or "Work:src/*.c" */
        size_t len = (size_t)(last_colon - pattern) + 1; /* include the colon */
        if (last_sep != NULL && last_sep > last_colon) {
            /* "Work:src/*.c" — include up to last / */
            len = (size_t)(last_sep - pattern);
        }
        if (len >= dir_size) len = dir_size - 1;
        memcpy(dir_buf, pattern, len);
        dir_buf[len] = '\0';
        return (last_sep != NULL && last_sep > last_colon) ?
               last_sep + 1 : last_colon + 1;
    }

    /* No separator — pattern is in current directory */
    dir_buf[0] = '\0';
    return pattern;
}

/*
 * Add a path to the glob result array, growing as needed.
 */
static int
glob_add(amiport_glob_t *pglob, const char *path)
{
    size_t new_count = pglob->gl_pathc + 1;
    char **new_pathv;
    char *copy;

    /* Grow array: always keep room for NULL terminator */
    new_pathv = (char **)realloc(pglob->gl_pathv,
                                 (new_count + 1) * sizeof(char *));
    if (new_pathv == NULL)
        return GLOB_NOSPACE;
    pglob->gl_pathv = new_pathv;

    copy = (char *)malloc(strlen(path) + 1);
    if (copy == NULL)
        return GLOB_NOSPACE;
    strcpy(copy, path);

    pglob->gl_pathv[pglob->gl_pathc] = copy;
    pglob->gl_pathc = new_count;
    pglob->gl_pathv[new_count] = NULL;

    return 0;
}

/*
 * Compare strings for qsort (glob result sorting).
 */
static int
glob_compare(const void *a, const void *b)
{
    return strcmp(*(const char * const *)a, *(const char * const *)b);
}

/*
 * Build full path from directory and filename.
 */
static void
build_path(const char *dir, const char *name, char *buf, size_t bufsize)
{
    size_t dlen;
    if (dir[0] == '\0') {
        /* No directory component — just the filename */
        strncpy(buf, name, bufsize - 1);
        buf[bufsize - 1] = '\0';
        return;
    }
    dlen = strlen(dir);
    strncpy(buf, dir, bufsize - 1);
    buf[bufsize - 1] = '\0';
    /* Add separator if needed (not after : which is already a separator) */
    if (dlen > 0 && dir[dlen - 1] != '/' && dir[dlen - 1] != ':') {
        if (dlen + 1 < bufsize) {
            buf[dlen] = '/';
            buf[dlen + 1] = '\0';
            dlen++;
        }
    }
    strncat(buf, name, bufsize - dlen - 1);
}

int
amiport_glob(const char *pattern, int flags,
             int (*errfunc)(const char *, int),
             amiport_glob_t *pglob)
{
    char dir_buf[256];
    char translate_buf[GLOB_TRANSLATE_BUF];
    char path_buf[512];
    const char *file_pattern;
    const char *match_pattern;
    AMIPORT_DIR *dir;
    struct amiport_dirent *ent;
    int rc;
    const char *dir_path;

    if (pattern == NULL || pglob == NULL)
        return GLOB_ABORTED;

    /* Initialize or preserve based on GLOB_APPEND */
    if (!(flags & GLOB_APPEND)) {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
        pglob->gl_offs = 0;
    }

    /* Split pattern into directory and filename */
    file_pattern = split_pattern(pattern, dir_buf, sizeof(dir_buf));

    /* Translate AmigaOS patterns to Unix if needed */
    if (translate_amiga_pattern(file_pattern, translate_buf,
                                sizeof(translate_buf))) {
        match_pattern = translate_buf;
    } else {
        match_pattern = file_pattern;
    }

    /* If no wildcards in filename part, check if literal file exists */
    if (!amiport_glob_has_magic(match_pattern)) {
        /* Literal path — just check existence */
        build_path(dir_buf, file_pattern, path_buf, sizeof(path_buf));
        /* Try to Lock it to verify existence */
        {
            AMIPORT_DIR *probe = amiport_opendir(path_buf);
            if (probe != NULL) {
                amiport_closedir(probe);
                /* It's a directory */
                if (flags & GLOB_MARK) {
                    strncat(path_buf, "/",
                            sizeof(path_buf) - strlen(path_buf) - 1);
                }
                rc = glob_add(pglob, path_buf);
                if (rc != 0) return rc;
            } else {
                /* Try as a file — use open to check */
                int fd = amiport_open(path_buf, 0 /* O_RDONLY */);
                if (fd >= 0) {
                    amiport_close(fd);
                    rc = glob_add(pglob, path_buf);
                    if (rc != 0) return rc;
                } else if (flags & GLOB_NOCHECK) {
                    rc = glob_add(pglob, pattern);
                    if (rc != 0) return rc;
                } else {
                    return GLOB_NOMATCH;
                }
            }
        }
        goto done;
    }

    /* Open directory for scanning */
    dir_path = (dir_buf[0] != '\0') ? dir_buf : ".";
    dir = amiport_opendir(dir_path);
    if (dir == NULL) {
        if (errfunc != NULL) {
            if (errfunc(dir_path, errno) != 0 || (flags & GLOB_ERR))
                return GLOB_ABORTED;
        } else if (flags & GLOB_ERR) {
            return GLOB_ABORTED;
        }
        if (flags & GLOB_NOCHECK) {
            rc = glob_add(pglob, pattern);
            return (rc != 0) ? rc : 0;
        }
        return GLOB_NOMATCH;
    }

    /* Iterate directory entries, match against pattern */
    while ((ent = amiport_readdir(dir)) != NULL) {
        /* Skip hidden files (starting with .) unless pattern starts with . */
        if (ent->d_name[0] == '.' && match_pattern[0] != '.')
            continue;

        if (amiport_fnmatch(match_pattern, ent->d_name, 0) == 0) {
            build_path(dir_buf, ent->d_name, path_buf, sizeof(path_buf));

            /* Append / for directories if GLOB_MARK */
            if ((flags & GLOB_MARK) &&
                ent->d_type == AMIPORT_DT_DIR) {
                size_t plen = strlen(path_buf);
                if (plen + 1 < sizeof(path_buf)) {
                    path_buf[plen] = '/';
                    path_buf[plen + 1] = '\0';
                }
            }

            rc = glob_add(pglob, path_buf);
            if (rc != 0) {
                amiport_closedir(dir);
                return rc;
            }
        }
    }

    amiport_closedir(dir);

done:
    /* No matches found? */
    if (pglob->gl_pathc == 0 ||
        (!(flags & GLOB_APPEND) && pglob->gl_pathc == 0)) {
        if (flags & GLOB_NOCHECK) {
            rc = glob_add(pglob, pattern);
            if (rc != 0) return rc;
        } else {
            return GLOB_NOMATCH;
        }
    }

    /* Sort unless GLOB_NOSORT */
    if (!(flags & GLOB_NOSORT) && pglob->gl_pathc > 1) {
        qsort(pglob->gl_pathv, pglob->gl_pathc, sizeof(char *),
              glob_compare);
    }

    return 0;
}

void
amiport_globfree(amiport_glob_t *pglob)
{
    size_t i;
    if (pglob == NULL)
        return;
    if (pglob->gl_pathv != NULL) {
        for (i = 0; i < pglob->gl_pathc; i++) {
            free(pglob->gl_pathv[i]);
        }
        free(pglob->gl_pathv);
        pglob->gl_pathv = NULL;
    }
    pglob->gl_pathc = 0;
}
```

- [ ] **Step 2: Add to Makefile**

In `lib/posix-shim/Makefile`, add `$(SRCDIR)/glob.c` and `$(SRCDIR)/argv_expand.c` to the SRCS list (argv_expand added here proactively since it'll be needed in Task 3):

```makefile
SRCS = $(SRCDIR)/file_io.c \
       $(SRCDIR)/dir_ops.c \
       $(SRCDIR)/getopt.c \
       $(SRCDIR)/signal_emu.c \
       $(SRCDIR)/errno_map.c \
       $(SRCDIR)/time_funcs.c \
       $(SRCDIR)/stat.c \
       $(SRCDIR)/process.c \
       $(SRCDIR)/fnmatch.c \
       $(SRCDIR)/scandir.c \
       $(SRCDIR)/string_bsd.c \
       $(SRCDIR)/stdio_ext.c \
       $(SRCDIR)/glob.c \
       $(SRCDIR)/argv_expand.c
```

- [ ] **Step 3: Build the shim library**

Run: `make build-shim`
Expected: Clean compilation, no warnings. `lib/posix-shim/libamiport.a` updated.

- [ ] **Step 4: Commit**

```bash
git add lib/posix-shim/src/glob.c lib/posix-shim/Makefile
git commit -m "feat: add POSIX glob()/globfree() to posix-shim

Implements glob(3) using amiport_opendir/readdir + amiport_fnmatch.
Supports both Unix (*, ?, [...]) and AmigaOS (#?, (a|b)) patterns
via transparent Amiga-to-Unix translation layer.
Shared glob_has_magic() helper for wildcard detection."
```

---

### Task 3: glob() Tests

**Files:**
- Create: `tests/shim/test_glob.c`
- Modify: `tests/shim/Makefile`

- [ ] **Step 1: Write `test_glob.c`**

```c
/*
 * test_glob.c — Tests for amiport glob/globfree
 *
 * Creates temp files in T:glob_test/ and verifies glob expansion.
 */

#include "test_framework.h"
#include <amiport/glob.h>
#include <amiport/dirent.h>
#include <amiport/unistd.h>
#include <amiport/internal.h>
#include <string.h>

static const char *verstag = "$VER: test_glob 1.0 (21.03.2026)";
long __stack = 32768;

#define TEST_DIR "T:glob_test"

/* Helper: create a file with given name */
static void create_file(const char *path)
{
    int fd = amiport_open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        amiport_write(fd, "x", 1);
        amiport_close(fd);
    }
}

/* Helper: set up test directory with known files */
static void setup(void)
{
    amiport_mkdir(TEST_DIR, 0755);
    create_file(TEST_DIR "/foo.c");
    create_file(TEST_DIR "/bar.c");
    create_file(TEST_DIR "/baz.h");
    create_file(TEST_DIR "/readme.txt");
}

/* Helper: tear down test directory */
static void teardown(void)
{
    amiport_unlink(TEST_DIR "/foo.c");
    amiport_unlink(TEST_DIR "/bar.c");
    amiport_unlink(TEST_DIR "/baz.h");
    amiport_unlink(TEST_DIR "/readme.txt");
    amiport_rmdir(TEST_DIR);
}

/* --- glob_has_magic tests --- */

TEST(magic_star)
{
    ASSERT_EQ(amiport_glob_has_magic("*.c"), 1);
}

TEST(magic_question)
{
    ASSERT_EQ(amiport_glob_has_magic("?.c"), 1);
}

TEST(magic_bracket)
{
    ASSERT_EQ(amiport_glob_has_magic("[abc].c"), 1);
}

TEST(magic_amiga_hashq)
{
    ASSERT_EQ(amiport_glob_has_magic("#?.c"), 1);
}

TEST(magic_amiga_tilde)
{
    ASSERT_EQ(amiport_glob_has_magic("~(test)"), 1);
}

TEST(magic_amiga_alternation)
{
    ASSERT_EQ(amiport_glob_has_magic("(foo|bar)"), 1);
}

TEST(magic_literal)
{
    ASSERT_EQ(amiport_glob_has_magic("hello.txt"), 0);
}

TEST(magic_null)
{
    ASSERT_EQ(amiport_glob_has_magic(NULL), 0);
}

TEST(magic_empty)
{
    ASSERT_EQ(amiport_glob_has_magic(""), 0);
}

/* --- glob() tests --- */

TEST(glob_star_c)
{
    amiport_glob_t g;
    int rc;
    int found_foo = 0, found_bar = 0;
    size_t i;

    setup();
    rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 2);

    for (i = 0; i < g.gl_pathc; i++) {
        if (strstr(g.gl_pathv[i], "foo.c")) found_foo = 1;
        if (strstr(g.gl_pathv[i], "bar.c")) found_bar = 1;
    }
    ASSERT(found_foo);
    ASSERT(found_bar);

    amiport_globfree(&g);
    teardown();
}

TEST(glob_sorted)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 2);

    /* Results should be sorted: bar.c before foo.c */
    ASSERT(strstr(g.gl_pathv[0], "bar.c") != NULL);
    ASSERT(strstr(g.gl_pathv[1], "foo.c") != NULL);

    amiport_globfree(&g);
    teardown();
}

TEST(glob_nosort)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/*.c", GLOB_NOSORT, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 2);
    /* Just verify count — order is undefined */

    amiport_globfree(&g);
    teardown();
}

TEST(glob_nomatch)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/*.zzz", 0, NULL, &g);
    ASSERT_EQ(rc, GLOB_NOMATCH);

    teardown();
}

TEST(glob_nocheck)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/*.zzz", GLOB_NOCHECK, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 1);
    /* Should return the original pattern as-is */
    ASSERT(strstr(g.gl_pathv[0], "*.zzz") != NULL);

    amiport_globfree(&g);
    teardown();
}

TEST(glob_append)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 2);

    rc = amiport_glob(TEST_DIR "/*.h", GLOB_APPEND, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 3);  /* 2 .c + 1 .h */

    amiport_globfree(&g);
    teardown();
}

TEST(glob_err_bad_dir)
{
    amiport_glob_t g;
    int rc;

    rc = amiport_glob("T:nonexistent_glob_dir_999/*.c", GLOB_ERR, NULL, &g);
    ASSERT_EQ(rc, GLOB_ABORTED);
}

TEST(glob_amiga_hashq)
{
    amiport_glob_t g;
    int rc;

    setup();
    /* #?.c should expand to *.c via translation */
    rc = amiport_glob(TEST_DIR "/#?.c", 0, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 2);

    amiport_globfree(&g);
    teardown();
}

TEST(glob_question_mark)
{
    amiport_glob_t g;
    int rc;

    setup();
    /* ???.c should match foo.c and bar.c (3-char names) */
    rc = amiport_glob(TEST_DIR "/???.c", 0, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 2);

    amiport_globfree(&g);
    teardown();
}

TEST(glob_literal)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/foo.c", 0, NULL, &g);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(g.gl_pathc, 1);
    ASSERT(strstr(g.gl_pathv[0], "foo.c") != NULL);

    amiport_globfree(&g);
    teardown();
}

TEST(glob_literal_noexist)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/nope.c", 0, NULL, &g);
    ASSERT_EQ(rc, GLOB_NOMATCH);

    teardown();
}

TEST(globfree_null_safe)
{
    amiport_glob_t g;
    g.gl_pathc = 0;
    g.gl_pathv = NULL;
    /* Should not crash */
    amiport_globfree(&g);
    amiport_globfree(NULL);
}

TEST(globfree_double_free)
{
    amiport_glob_t g;
    int rc;

    setup();
    rc = amiport_glob(TEST_DIR "/*.c", 0, NULL, &g);
    ASSERT_EQ(rc, 0);

    amiport_globfree(&g);
    /* Second free should be safe — gl_pathv set to NULL */
    ASSERT_NULL(g.gl_pathv);
    ASSERT_EQ(g.gl_pathc, 0);

    teardown();
}

int main(void)
{
    (void)verstag;
    printf("=== Glob Tests ===\n");

    /* glob_has_magic */
    RUN_TEST(magic_star);
    RUN_TEST(magic_question);
    RUN_TEST(magic_bracket);
    RUN_TEST(magic_amiga_hashq);
    RUN_TEST(magic_amiga_tilde);
    RUN_TEST(magic_amiga_alternation);
    RUN_TEST(magic_literal);
    RUN_TEST(magic_null);
    RUN_TEST(magic_empty);

    /* glob() */
    RUN_TEST(glob_star_c);
    RUN_TEST(glob_sorted);
    RUN_TEST(glob_nosort);
    RUN_TEST(glob_nomatch);
    RUN_TEST(glob_nocheck);
    RUN_TEST(glob_append);
    RUN_TEST(glob_err_bad_dir);
    RUN_TEST(glob_amiga_hashq);
    RUN_TEST(glob_question_mark);
    RUN_TEST(glob_literal);
    RUN_TEST(glob_literal_noexist);

    /* globfree() */
    RUN_TEST(globfree_null_safe);
    RUN_TEST(globfree_double_free);

    return test_summary();
}
```

- [ ] **Step 2: Add to test Makefile**

In `tests/shim/Makefile`, add `test_glob` and `test_argv_expand` to the TESTS list:

```makefile
TESTS = test_file_io \
        test_getopt \
        test_errno \
        test_dir_ops \
        test_stat \
        test_signal \
        test_process \
        test_time \
        test_misc \
        test_string_bsd \
        test_stdio_ext \
        test_dup \
        test_new_funcs \
        test_glob \
        test_argv_expand
```

- [ ] **Step 3: Build and run glob tests**

Run: `make test-shim`
Expected: `test_glob` compiles and passes all 22 tests. Other tests unaffected.

- [ ] **Step 4: Commit**

```bash
git add tests/shim/test_glob.c tests/shim/Makefile
git commit -m "test: add comprehensive glob API test suite (22 tests)

Tests glob_has_magic (Unix + AmigaOS patterns), glob() with
NOSORT/NOCHECK/APPEND/ERR flags, AmigaOS #? pattern translation,
literal paths, and globfree safety (null, double-free)."
```

---

### Task 4: argv_expand Implementation

**Files:**
- Create: `lib/posix-shim/src/argv_expand.c`

- [ ] **Step 1: Create `argv_expand.c`**

```c
/*
 * argv_expand.c — Automatic argv wildcard expansion for AmigaOS
 *
 * amiport: provides amiport_expand_argv() to expand wildcard arguments
 * in the argv array, matching the convention of SAS/C, DICE, and Lattice
 * compilers. Programs that take pattern arguments (grep, sed) should
 * define `int __nowild = 1;` to suppress expansion.
 *
 * Written in C89 for AmigaOS 3.x compatibility.
 */

#include <amiport/glob.h>
#include <amiport/internal.h>

#include <stdlib.h>
#include <string.h>

/* Maximum expanded arguments — prevent runaway expansion */
#define MAX_EXPANDED_ARGS 4096

/* Track allocated memory for cleanup */
static char **expanded_argv = NULL;
static char **expanded_strings = NULL;
static int expanded_string_count = 0;
static int did_expand = 0;

/*
 * Weak reference to __nowild — if the port defines it, we check it.
 * If not defined, the weak symbol resolves to NULL (address 0).
 *
 * Note: GCC weak symbols work differently than SAS/C's link-time check.
 * We use a weak extern and check the value at runtime.
 */
#pragma weak __nowild
extern int __nowild;

/*
 * Check if an argument should be expanded.
 * Skip argv[0] (program name), options (starting with -), and literals.
 */
static int
should_expand(const char *arg, int index)
{
    if (index == 0)
        return 0;  /* Never expand program name */
    if (arg[0] == '-')
        return 0;  /* Never expand option flags */
    return amiport_glob_has_magic(arg);
}

void
amiport_expand_argv(int *argc_p, char ***argv_p)
{
    int orig_argc;
    char **orig_argv;
    int new_argc;
    int i;
    int str_alloc;

    if (argc_p == NULL || argv_p == NULL)
        return;

    /* Check __nowild opt-out.
     * With weak linkage, if __nowild is not defined anywhere,
     * &__nowild may be NULL. If defined and set to non-zero, skip. */
    if (&__nowild != NULL && __nowild != 0)
        return;

    orig_argc = *argc_p;
    orig_argv = *argv_p;

    /* Quick scan: any wildcards at all? */
    {
        int has_wild = 0;
        for (i = 1; i < orig_argc; i++) {
            if (should_expand(orig_argv[i], i)) {
                has_wild = 1;
                break;
            }
        }
        if (!has_wild)
            return;  /* Nothing to expand */
    }

    /* Allocate tracking arrays */
    str_alloc = 64;
    expanded_strings = (char **)malloc((size_t)str_alloc * sizeof(char *));
    if (expanded_strings == NULL)
        return;  /* OOM — leave argv unchanged */
    expanded_string_count = 0;

    /* Build new argv — start with generous allocation */
    new_argc = 0;
    expanded_argv = (char **)malloc((size_t)(orig_argc + 128) * sizeof(char *));
    if (expanded_argv == NULL) {
        free(expanded_strings);
        expanded_strings = NULL;
        return;
    }

    /* argv[0] always passes through */
    expanded_argv[new_argc++] = orig_argv[0];

    for (i = 1; i < orig_argc; i++) {
        if (should_expand(orig_argv[i], i)) {
            amiport_glob_t g;
            int rc;
            size_t j;

            rc = amiport_glob(orig_argv[i], GLOB_NOCHECK | GLOB_NOSORT,
                              NULL, &g);
            if (rc == 0 && g.gl_pathc > 0) {
                /* Grow expanded_argv if needed */
                {
                    size_t needed = (size_t)new_argc + g.gl_pathc +
                                    (size_t)(orig_argc - i) + 1;
                    char **tmp = (char **)realloc(expanded_argv,
                                                  needed * sizeof(char *));
                    if (tmp == NULL) {
                        amiport_globfree(&g);
                        /* OOM — stop expanding, keep what we have */
                        break;
                    }
                    expanded_argv = tmp;
                }

                for (j = 0; j < g.gl_pathc; j++) {
                    char *copy;
                    if (new_argc >= MAX_EXPANDED_ARGS)
                        break;

                    copy = (char *)malloc(strlen(g.gl_pathv[j]) + 1);
                    if (copy == NULL)
                        break;
                    strcpy(copy, g.gl_pathv[j]);

                    expanded_argv[new_argc++] = copy;

                    /* Track for cleanup */
                    if (expanded_string_count >= str_alloc) {
                        int new_alloc = str_alloc * 2;
                        char **tmp = (char **)realloc(expanded_strings,
                                        (size_t)new_alloc * sizeof(char *));
                        if (tmp == NULL)
                            break;
                        expanded_strings = tmp;
                        str_alloc = new_alloc;
                    }
                    expanded_strings[expanded_string_count++] = copy;
                }

                amiport_globfree(&g);
            } else {
                /* No expansion — keep original */
                expanded_argv[new_argc++] = orig_argv[i];
            }
        } else {
            /* Not a wildcard — pass through */
            expanded_argv[new_argc++] = orig_argv[i];
        }
    }

    expanded_argv[new_argc] = NULL;
    did_expand = 1;

    *argc_p = new_argc;
    *argv_p = expanded_argv;
}

void
amiport_free_argv(void)
{
    int i;

    if (!did_expand)
        return;

    /* Free all strings we allocated */
    if (expanded_strings != NULL) {
        for (i = 0; i < expanded_string_count; i++) {
            free(expanded_strings[i]);
        }
        free(expanded_strings);
        expanded_strings = NULL;
    }
    expanded_string_count = 0;

    /* Free the argv array itself */
    if (expanded_argv != NULL) {
        free(expanded_argv);
        expanded_argv = NULL;
    }

    did_expand = 0;
}
```

- [ ] **Step 2: Build the shim library**

Run: `make build-shim`
Expected: Clean compilation. Both `glob.o` and `argv_expand.o` in the archive.

- [ ] **Step 3: Commit**

```bash
git add lib/posix-shim/src/argv_expand.c
git commit -m "feat: add argv wildcard expansion with __nowild opt-out

Matches SAS/C/DICE convention: amiport_expand_argv() called at top
of main() expands wildcard args via glob(). Programs taking pattern
arguments (grep, sed) define 'int __nowild = 1;' to suppress.
Companion amiport_free_argv() for cleanup before _exit()."
```

---

### Task 5: argv_expand Tests

**Files:**
- Create: `tests/shim/test_argv_expand.c`

- [ ] **Step 1: Write `test_argv_expand.c`**

```c
/*
 * test_argv_expand.c — Tests for amiport argv wildcard expansion
 *
 * Tests amiport_expand_argv() and amiport_free_argv() directly.
 */

#include "test_framework.h"
#include <amiport/glob.h>
#include <amiport/dirent.h>
#include <amiport/unistd.h>
#include <amiport/internal.h>
#include <string.h>

static const char *verstag = "$VER: test_argv_expand 1.0 (21.03.2026)";
long __stack = 32768;

/* Declared in argv_expand.c */
extern void amiport_expand_argv(int *argc, char ***argv);
extern void amiport_free_argv(void);

#define TEST_DIR "T:argv_test"

static void create_file(const char *path)
{
    int fd = amiport_open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        amiport_write(fd, "x", 1);
        amiport_close(fd);
    }
}

static void setup(void)
{
    amiport_mkdir(TEST_DIR, 0755);
    create_file(TEST_DIR "/alpha.c");
    create_file(TEST_DIR "/beta.c");
    create_file(TEST_DIR "/gamma.h");
}

static void teardown(void)
{
    amiport_unlink(TEST_DIR "/alpha.c");
    amiport_unlink(TEST_DIR "/beta.c");
    amiport_unlink(TEST_DIR "/gamma.h");
    amiport_rmdir(TEST_DIR);
}

TEST(no_wildcards_unchanged)
{
    char *args[] = {"prog", "file.c", "-v", NULL};
    char **argv = args;
    int argc = 3;

    amiport_expand_argv(&argc, &argv);

    ASSERT_EQ(argc, 3);
    ASSERT_STR_EQ(argv[0], "prog");
    ASSERT_STR_EQ(argv[1], "file.c");
    ASSERT_STR_EQ(argv[2], "-v");

    amiport_free_argv();
}

TEST(expand_star_c)
{
    char *args[] = {"prog", TEST_DIR "/*.c", NULL};
    char **argv = args;
    int argc = 2;

    setup();
    amiport_expand_argv(&argc, &argv);

    /* Should expand to 2 .c files */
    ASSERT_EQ(argc, 3);  /* prog + alpha.c + beta.c */
    ASSERT_STR_EQ(argv[0], "prog");
    /* Verify both .c files are present (order may vary with NOSORT) */
    {
        int found_alpha = 0, found_beta = 0;
        int i;
        for (i = 1; i < argc; i++) {
            if (strstr(argv[i], "alpha.c")) found_alpha = 1;
            if (strstr(argv[i], "beta.c")) found_beta = 1;
        }
        ASSERT(found_alpha);
        ASSERT(found_beta);
    }

    amiport_free_argv();
    teardown();
}

TEST(option_args_not_expanded)
{
    char *args[] = {"prog", "-e", "*.c", TEST_DIR "/*.c", NULL};
    char **argv = args;
    int argc = 4;

    setup();
    amiport_expand_argv(&argc, &argv);

    /* -e should not be expanded, "*.c" after -e IS expanded because
     * it doesn't start with - (it's a positional arg to the program).
     * But "*.c" without a path won't match anything in CWD.
     * The TEST_DIR/*.c should expand. */
    ASSERT_STR_EQ(argv[0], "prog");
    ASSERT_STR_EQ(argv[1], "-e");
    /* argv[2] = "*.c" — has wildcards but GLOB_NOCHECK keeps it */

    amiport_free_argv();
    teardown();
}

TEST(argv0_never_expanded)
{
    /* Even if argv[0] contains wildcards, never expand it */
    char *args[] = {"prog*", TEST_DIR "/*.c", NULL};
    char **argv = args;
    int argc = 2;

    setup();
    amiport_expand_argv(&argc, &argv);

    ASSERT_STR_EQ(argv[0], "prog*");

    amiport_free_argv();
    teardown();
}

TEST(no_matches_keeps_literal)
{
    char *args[] = {"prog", TEST_DIR "/*.zzz", NULL};
    char **argv = args;
    int argc = 2;

    setup();
    amiport_expand_argv(&argc, &argv);

    /* GLOB_NOCHECK: pattern with no matches kept as literal */
    ASSERT_EQ(argc, 2);
    ASSERT(strstr(argv[1], "*.zzz") != NULL);

    amiport_free_argv();
    teardown();
}

TEST(mixed_literal_and_wildcard)
{
    char *args[] = {"prog", "literal.txt", TEST_DIR "/*.c", "-v", NULL};
    char **argv = args;
    int argc = 4;

    setup();
    amiport_expand_argv(&argc, &argv);

    /* literal.txt stays, *.c expands to 2, -v stays */
    ASSERT_EQ(argc, 5);  /* prog + literal.txt + alpha.c + beta.c + -v */
    ASSERT_STR_EQ(argv[0], "prog");
    ASSERT_STR_EQ(argv[1], "literal.txt");
    /* argv[2] and argv[3] are the .c files */
    ASSERT_STR_EQ(argv[4], "-v");

    amiport_free_argv();
    teardown();
}

TEST(amiga_hashq_pattern)
{
    char *args[] = {"prog", TEST_DIR "/#?.c", NULL};
    char **argv = args;
    int argc = 2;

    setup();
    amiport_expand_argv(&argc, &argv);

    /* #?.c should expand like *.c */
    ASSERT_EQ(argc, 3);

    amiport_free_argv();
    teardown();
}

TEST(free_argv_safe_double_call)
{
    /* Calling free_argv twice should not crash */
    amiport_free_argv();
    amiport_free_argv();
}

TEST(free_argv_no_expand)
{
    /* free_argv when no expansion happened is a no-op */
    amiport_free_argv();
}

TEST(null_params_safe)
{
    /* NULL argc or argv should not crash */
    amiport_expand_argv(NULL, NULL);

    {
        int argc = 1;
        amiport_expand_argv(&argc, NULL);
    }
    {
        char **argv = NULL;
        amiport_expand_argv(NULL, &argv);
    }
}

int main(void)
{
    (void)verstag;
    printf("=== Argv Expand Tests ===\n");

    RUN_TEST(no_wildcards_unchanged);
    RUN_TEST(expand_star_c);
    RUN_TEST(option_args_not_expanded);
    RUN_TEST(argv0_never_expanded);
    RUN_TEST(no_matches_keeps_literal);
    RUN_TEST(mixed_literal_and_wildcard);
    RUN_TEST(amiga_hashq_pattern);
    RUN_TEST(free_argv_safe_double_call);
    RUN_TEST(free_argv_no_expand);
    RUN_TEST(null_params_safe);

    return test_summary();
}
```

- [ ] **Step 2: Build and run all shim tests**

Run: `make test-shim`
Expected: Both `test_glob` (22 tests) and `test_argv_expand` (10 tests) pass. All existing tests unaffected.

- [ ] **Step 3: Commit**

```bash
git add tests/shim/test_argv_expand.c
git commit -m "test: add argv expansion test suite (10 tests)

Tests wildcard expansion, option/argv0 preservation, GLOB_NOCHECK
literal fallback, AmigaOS #? pattern support, mixed args, cleanup
safety (double-free, no-expand, null params)."
```

---

### Task 6: Add glob.h to Transformation Rules + Update Agents

**Files:**
- Modify: `.claude/skills/transform-source/references/transformation-rules.md`
- Modify: `.claude/skills/analyze-source/references/common-patterns.md`

- [ ] **Step 1: Add glob header replacement to transformation-rules.md**

Add to Section 1 (Header Replacements), after the fnmatch entry:

```c
// Before:
#include <glob.h>
// After:
#include <amiport/glob.h>
```

- [ ] **Step 2: Add Section 10 to transformation-rules.md**

Add after Section 9 (Exit Code Fixup):

```markdown
## 10. Wildcard/Glob Handling

### argv wildcard expansion
Every ported program SHOULD call `amiport_expand_argv()` at the top of `main()` and
`amiport_free_argv()` before `_exit()`. This expands wildcard arguments (*.c, #?.c)
since AmigaOS shells do not glob-expand like Unix.

```c
/* RULE: Add argv wildcard expansion to main() */

#include <amiport/glob.h>

int main(int argc, char *argv[])
{
    /* amiport: expand wildcard args — Amiga shell doesn't glob */
    amiport_expand_argv(&argc, &argv);

    /* ... original main body ... */

    /* amiport: free expanded argv before exit */
    amiport_free_argv();
    fflush(stdout);
    _exit(rval);
}
```

### __nowild opt-out for pattern-argument programs
Programs that accept regex or pattern arguments (grep -e PATTERN, sed SCRIPT,
find -name PATTERN, awk PROGRAM) MUST define `__nowild` to prevent expansion of
those arguments:

```c
/* RULE: Suppress argv expansion for programs taking pattern args */
/* amiport: suppress wildcard expansion — program takes pattern arguments */
int __nowild = 1;
```

Programs that need `__nowild`: grep, sed, awk, find, expr, test, and any program
where a non-option argument is a pattern/regex rather than a filename.

### glob()/globfree() replacement
```c
/* RULE: Replace POSIX glob with amiport shim wrapper */

// Before:
#include <glob.h>
glob_t g;
glob("*.c", 0, NULL, &g);
globfree(&g);

// After:
#include <amiport/glob.h>
/* amiport: replaced glob() */
amiport_glob_t g;
amiport_glob("*.c", 0, NULL, &g);
amiport_globfree(&g);
```

Note: With AMIPORT_NO_GLOB_MACROS not defined, convenience macros allow using
the original POSIX names. Just include `<amiport/glob.h>`.
```

- [ ] **Step 3: Add wildcard pattern to common-patterns.md**

Add a new pattern section:

```markdown
## Pattern N: Wildcard Arguments

**Frequency**: Common in CLI tools that process files

**What to check**: Does the program accept filename arguments that users might
pass with wildcards (`*.c`, `*.txt`)? If so, it needs argv expansion since AmigaOS
shells don't expand wildcards.

**Action**: Add `amiport_expand_argv(&argc, &argv)` at top of `main()` and
`amiport_free_argv()` before `_exit()`.

**Exception**: Programs that accept regex/pattern arguments (grep, sed, awk, find)
need `int __nowild = 1;` to prevent expansion of pattern args.

**Severity**: needs-shim
```

- [ ] **Step 4: Commit**

```bash
git add .claude/skills/transform-source/references/transformation-rules.md \
        .claude/skills/analyze-source/references/common-patterns.md
git commit -m "docs: add glob/argv expansion rules to transformer and analyzer agents

Section 10 in transformation-rules.md: argv expansion call, __nowild
opt-out for pattern programs, glob() replacement. New 'Wildcard
Arguments' pattern in common-patterns.md for source-analyzer."
```

---

### Task 7: Documentation Updates

**Files:**
- Modify: `docs/posix-tiers.md`
- Modify: `docs/references/newlib-availability.md`
- Modify: `CLAUDE.md`
- Modify: `TODOS.md`

- [ ] **Step 1: Update `docs/posix-tiers.md`**

Move `glob`/`globfree` from wherever they are (or add) to the Tier 1 table:

```
|`glob()`       |`amiport_glob()`        |opendir+readdir+fnmatch    |Expands wildcard patterns against filesystem; supports Amiga patterns |
|`globfree()`   |`amiport_globfree()`    |free loop                  |Frees glob result array                                              |
```

Add `amiport_expand_argv` and `amiport_free_argv` to the Tier 1 table as well:

```
|`—` (startup)  |`amiport_expand_argv()` |amiport_glob()             |Expands wildcard argv entries; SAS/C __nowild convention              |
|`—` (cleanup)  |`amiport_free_argv()`   |free loop                  |Frees expanded argv memory                                           |
```

- [ ] **Step 2: Update `docs/references/newlib-availability.md`**

Change the Glob section:

```markdown
## Glob / Pattern Matching

| Function | Status | Notes |
|----------|--------|-------|
| `fnmatch` | Missing | Use `amiport_fnmatch()` |
| `glob`, `globfree` | Missing | Use `amiport_glob()` / `amiport_globfree()` from posix-shim |
```

- [ ] **Step 3: Update CLAUDE.md Known Pitfalls**

Add a new pitfall section:

```markdown
### __nowild for pattern-argument programs
Programs that accept regex or pattern arguments (grep, sed, find -name, awk) MUST define `int __nowild = 1;` at file scope. Without this, `amiport_expand_argv()` will expand pattern arguments like `*.c` before the program sees them. This matches the SAS/C convention where `__nowild` suppressed automatic argv expansion.
```

- [ ] **Step 4: Update TODOS.md**

Mark the glob item as done. Add the __nowild retrofit as a new TODO:

```markdown
### ~~Amiga wildcard/glob expansion shim~~ — DONE

Implemented both POSIX `glob()`/`globfree()` (Tier 1) and automatic argv wildcard expansion via `amiport_expand_argv()`. Supports both Unix (*, ?, [...]) and AmigaOS (#?, (a|b)) patterns. Opt-out via `int __nowild = 1;` matching SAS/C convention. 32 tests across two test suites.

---

### Retrofit __nowild to existing pattern-argument ports

**What:** Add `int __nowild = 1;` to grep and sed ports.

**Why:** With argv expansion now in libamiport, these ports will incorrectly expand pattern arguments unless suppressed. E.g., `grep *.c file.txt` would expand `*.c` before grep sees it.

**Priority:** High — required before next libamiport release.

**Depends on:** Glob/argv shim being complete (done).
```

- [ ] **Step 5: Commit**

```bash
git add docs/posix-tiers.md docs/references/newlib-availability.md CLAUDE.md TODOS.md
git commit -m "docs: update tiers, availability, pitfalls, and TODOs for glob shim

glob/globfree moved to Tier 1. newlib-availability updated.
__nowild added to Known Pitfalls. TODOS: glob done, __nowild
retrofit captured as follow-up."
```

---

### Task 8: Final Verification

- [ ] **Step 1: Build everything**

Run: `make build-shim`
Expected: Clean build, no warnings.

- [ ] **Step 2: Run all shim tests**

Run: `make test-shim`
Expected: All test suites pass, including new test_glob (22 tests) and test_argv_expand (10 tests).

- [ ] **Step 3: Run doc consistency check**

Run: `make check-docs`
Expected: No doc drift detected.

- [ ] **Step 4: Check for stray files**

Run: `git status`
Expected: No untracked files in project root. All changes inside `lib/`, `tests/`, `docs/`, `.claude/`.

- [ ] **Step 5: Run existing port tests (regression)**

Run: `make test-ports`
Expected: All existing ports still pass. The new glob/argv_expand code in libamiport.a doesn't break existing links since `amiport_expand_argv` is only called when explicitly invoked.

- [ ] **Step 6: Commit any final adjustments, then verify clean state**

Run: `git status`
Expected: Clean working tree. All changes committed.
