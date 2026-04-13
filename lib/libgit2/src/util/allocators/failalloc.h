/*
 * allocators/failalloc.h -- fail-before-init allocator for libgit2
 * Used as the default allocator before git_libgit2_init() is called.
 * These functions set an error and return NULL/do nothing.
 */
#ifndef GIT_ALLOCATORS_FAILALLOC_H
#define GIT_ALLOCATORS_FAILALLOC_H
#include <stddef.h>
void *git_failalloc_malloc(size_t n, const char *file, int line);
void *git_failalloc_realloc(void *ptr, size_t n, const char *file, int line);
void  git_failalloc_free(void *ptr);
#endif
