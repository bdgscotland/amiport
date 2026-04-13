/*
 * clone.h -- stub for AmigaOS port (PDR-010, no clone/network support)
 *
 * amiport: git_clone__submodule is an internal function from clone.c
 * which was excluded from this port. The stub declaration allows
 * submodule.c to compile; calls at runtime will be unresolved (submodule
 * clone operations are unsupported on the Amiga target).
 */
#ifndef INCLUDE_clone_h__
#define INCLUDE_clone_h__

#include "git2/clone.h"

/* Internal clone function used by submodule.c */
int git_clone__submodule(git_repository **out, const char *url,
                         const char *local_path,
                         const git_clone_options *opts);

#endif /* INCLUDE_clone_h__ */
