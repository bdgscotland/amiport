/*
 * remote.h -- stub internal header for libgit2 AmigaOS port
 *
 * The full remote/transport layer is excluded from this port (PDR-010:
 * no network support). This stub provides just enough declarations for
 * branch.c to compile. The actual function bodies are not present; any
 * amigit code that calls these functions will get link errors (which is
 * the desired behaviour -- remote operations are unsupported).
 *
 * amiport: PDR-010 Phase 2, Stage 3 stub -- documented in PATCHES.md.
 */

#ifndef INCLUDE_remote_h__
#define INCLUDE_remote_h__

#include "git2/remote.h"
#include "git2/strarray.h"
#include "refspec.h"

/* Default remote name constant */
#define GIT_REMOTE_ORIGIN "origin"

/* Internal remote lookup/free (public API wrappers) */
int git_remote_lookup(git_remote **out, git_repository *repo, const char *name);
void git_remote_free(git_remote *remote);
int git_remote_list(git_strarray *out, git_repository *repo);
int git_remote_create(git_remote **out, git_repository *repo, const char *name, const char *url);

/* Internal refspec matching helpers used by branch.c */
const git_refspec *git_remote__matching_refspec(git_remote *remote, const char *refname);
const git_refspec *git_remote__matching_dst_refspec(git_remote *remote, const char *refname);

#endif /* INCLUDE_remote_h__ */
