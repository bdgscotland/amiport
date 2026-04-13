/*
 * allocators/debugalloc.h -- stub for AmigaOS port
 * GIT_DEBUG_STRICT_ALLOC is never defined in this port; the debug
 * allocator path in alloc.c is dead code. This header satisfies
 * the #include without providing a real implementation.
 */
#ifndef GIT_ALLOCATORS_DEBUGALLOC_H
#define GIT_ALLOCATORS_DEBUGALLOC_H
#include "git2_util.h"
struct git_allocator;
int git_debugalloc_init_allocator(struct git_allocator *allocator);
#endif
