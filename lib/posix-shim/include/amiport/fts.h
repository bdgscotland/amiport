/*
 * amiport/fts.h -- File tree traversal for AmigaOS
 *
 * Tier 1 shim: recursive directory walk using Lock()/Examine()/ExNext()
 * via the existing amiport_opendir()/amiport_readdir() infrastructure.
 *
 * amiport: AmigaOS has no symlinks on classic FFS, so FTS_PHYSICAL and
 * FTS_LOGICAL behave identically. FTS_NOCHDIR is always in effect
 * (AmigaOS Lock-based traversal doesn't require chdir). FTS_NOSTAT
 * is accepted but stat info is always available via Examine().
 *
 * Supports: fts_open, fts_read, fts_close, fts_set, fts_children
 * Unlocks: ls, du, find, cp, rm, chmod
 *
 * Based on the proven FTS replacement from the grep 1.68 port,
 * extended with full FTSENT fields for ls/find/cp compatibility.
 */

#ifndef AMIPORT_FTS_H
#define AMIPORT_FTS_H

#include <stddef.h>           /* size_t */
#include <amiport/sys/stat.h>

/* FTS open options */
#define AMIPORT_FTS_PHYSICAL   0x01  /* Physical walk (no symlink follow) */
#define AMIPORT_FTS_LOGICAL    0x02  /* Logical walk (follow symlinks) */
#define AMIPORT_FTS_NOCHDIR    0x04  /* Don't chdir (always in effect) */
#define AMIPORT_FTS_NOSTAT     0x08  /* Don't stat (ignored -- always stat) */
#define AMIPORT_FTS_SEEDOT     0x10  /* Return . and .. entries */
#define AMIPORT_FTS_XDEV       0x20  /* Don't cross devices */
#define AMIPORT_FTS_COMFOLLOW  0x40  /* Follow symlinks on command line */

/* FTSENT info values */
#define AMIPORT_FTS_D       1   /* Directory (preorder visit) */
#define AMIPORT_FTS_DC      2   /* Directory causing cycle (not possible on Amiga) */
#define AMIPORT_FTS_DEFAULT 3   /* Not a directory or special */
#define AMIPORT_FTS_DNR     4   /* Unreadable directory */
#define AMIPORT_FTS_DOT     5   /* Dot file (. or ..) */
#define AMIPORT_FTS_DP      6   /* Directory (postorder visit) */
#define AMIPORT_FTS_ERR     7   /* Error */
#define AMIPORT_FTS_F       8   /* Regular file */
#define AMIPORT_FTS_NS      9   /* No stat info available */
#define AMIPORT_FTS_NSOK   10   /* No stat requested */
#define AMIPORT_FTS_SL     11   /* Symbolic link */
#define AMIPORT_FTS_SLNONE 12   /* Broken symlink */

/* fts_set instructions */
#define AMIPORT_FTS_AGAIN   1   /* Re-visit this entry */
#define AMIPORT_FTS_FOLLOW  2   /* Follow this symlink */
#define AMIPORT_FTS_SKIP    4   /* Skip children of this directory */

/* Maximum recursion depth */
#define AMIPORT_FTS_MAX_DEPTH 64

/* Path buffer size */
#ifndef PATH_MAX
#define PATH_MAX 256
#endif

/* Forward declarations */
typedef struct _amiport_ftsent AMIPORT_FTSENT;
typedef struct _amiport_fts    AMIPORT_FTS;

/* FTSENT -- entry in file tree traversal */
struct _amiport_ftsent {
    char               *fts_path;      /* full path from root */
    size_t              fts_pathlen;   /* strlen(fts_path) */
    char               *fts_name;     /* filename component */
    short               fts_namelen;  /* strlen(fts_name) */
    short               fts_info;     /* FTS_D, FTS_F, etc. */
    int                 fts_errno;    /* errno for FTS_ERR/FTS_DNR */
    long                fts_number;   /* user-settable field */
    void               *fts_pointer;  /* user-settable pointer */
    short               fts_level;    /* depth (0 = root arg) */
    char               *fts_accpath;  /* access path (same as fts_path) */
    struct amiport_stat *fts_statp;   /* stat info */
    AMIPORT_FTSENT     *fts_parent;   /* parent directory entry */
    AMIPORT_FTSENT     *fts_link;     /* next entry in fts_children list */
};

/* FTS -- traversal state (opaque to caller) */
struct _amiport_fts {
    char              **root_argv;      /* argv from fts_open */
    int                 root_idx;       /* current argv index */
    void               *dir_stack[AMIPORT_FTS_MAX_DEPTH]; /* DIR* handles */
    char                path_stack[AMIPORT_FTS_MAX_DEPTH][PATH_MAX];
    int                 depth;          /* current depth (-1 = not started) */
    AMIPORT_FTSENT      entry;          /* current entry */
    AMIPORT_FTSENT      parent_entries[AMIPORT_FTS_MAX_DEPTH]; /* parent chain */
    struct amiport_stat statbuf;        /* stat buffer for current entry */
    char                pathbuf[PATH_MAX * 2]; /* temp path buffer */
    int                 flags;          /* open flags */
    int                 skip;           /* skip flag from fts_set */
    int (*compar)(const AMIPORT_FTSENT **, const AMIPORT_FTSENT **);
    AMIPORT_FTSENT     *children;       /* children list head */
    int                 children_valid;  /* children list is populated */
};

/* API functions */
AMIPORT_FTS    *amiport_fts_open(char * const *argv, int options,
                   int (*compar)(const AMIPORT_FTSENT **,
                                 const AMIPORT_FTSENT **));
AMIPORT_FTSENT *amiport_fts_read(AMIPORT_FTS *ftsp);
int             amiport_fts_close(AMIPORT_FTS *ftsp);
int             amiport_fts_set(AMIPORT_FTS *ftsp, AMIPORT_FTSENT *f,
                   int instr);
AMIPORT_FTSENT *amiport_fts_children(AMIPORT_FTS *ftsp, int options);

/* Convenience macros */
#ifndef AMIPORT_NO_FTS_MACROS
#define FTS_PHYSICAL   AMIPORT_FTS_PHYSICAL
#define FTS_LOGICAL    AMIPORT_FTS_LOGICAL
#define FTS_NOCHDIR    AMIPORT_FTS_NOCHDIR
#define FTS_NOSTAT     AMIPORT_FTS_NOSTAT
#define FTS_SEEDOT     AMIPORT_FTS_SEEDOT
#define FTS_XDEV       AMIPORT_FTS_XDEV
#define FTS_COMFOLLOW  AMIPORT_FTS_COMFOLLOW

#define FTS_D       AMIPORT_FTS_D
#define FTS_DC      AMIPORT_FTS_DC
#define FTS_DEFAULT AMIPORT_FTS_DEFAULT
#define FTS_DNR     AMIPORT_FTS_DNR
#define FTS_DOT     AMIPORT_FTS_DOT
#define FTS_DP      AMIPORT_FTS_DP
#define FTS_ERR     AMIPORT_FTS_ERR
#define FTS_F       AMIPORT_FTS_F
#define FTS_NS      AMIPORT_FTS_NS
#define FTS_NSOK    AMIPORT_FTS_NSOK
#define FTS_SL      AMIPORT_FTS_SL
#define FTS_SLNONE  AMIPORT_FTS_SLNONE

#define FTS_AGAIN   AMIPORT_FTS_AGAIN
#define FTS_FOLLOW  AMIPORT_FTS_FOLLOW
#define FTS_SKIP    AMIPORT_FTS_SKIP

#define FTS         AMIPORT_FTS
#define FTSENT      AMIPORT_FTSENT
#define fts_open    amiport_fts_open
#define fts_read    amiport_fts_read
#define fts_close   amiport_fts_close
#define fts_set     amiport_fts_set
#define fts_children amiport_fts_children
#endif

#endif /* AMIPORT_FTS_H */
