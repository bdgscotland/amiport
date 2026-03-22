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

/* Argv wildcard expansion — call at top of main() */
void amiport_expand_argv(int *argc, char ***argv);
void amiport_free_argv(void);

#ifndef AMIPORT_NO_GLOB_MACROS
#define glob_t    amiport_glob_t
#define glob      amiport_glob
#define globfree  amiport_globfree
#endif

#endif /* AMIPORT_GLOB_H */
