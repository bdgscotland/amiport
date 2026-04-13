/*
 * allocators/win32_leakcheck.h -- stub (AmigaOS port, not Win32)
 */
#ifndef GIT_ALLOCATORS_WIN32_LEAKCHECK_H
#define GIT_ALLOCATORS_WIN32_LEAKCHECK_H
/* Win32 leak check allocator -- not used on AmigaOS */
struct git_allocator;
static int git_win32_leakcheck_init_allocator(struct git_allocator *a) { (void)a; return 0; }
#endif
