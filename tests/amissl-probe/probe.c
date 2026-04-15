/*
 * probe.c -- minimal amisslmaster.library OpenLibrary diagnostic.
 *
 * PDR-012 Phase 3 session 3 blocker isolation. Zero amiport deps --
 * just libnix + exec/dos. Purpose: determine whether the
 * OpenLibrary-returns-NULL issue that amigit hits on the FS-UAE
 * harness is amigit-specific (memory pressure, link order,
 * atexit/startup interaction) or environmental (library file
 * layout, LIBS: walk, Kickstart version).
 *
 * Tests performed:
 *   [1] Dump AvailMem() for CHIP/FAST/PUBLIC (memory-layout sanity).
 *   [2] Lock() each candidate path for both amisslmaster.library
 *       and the 68020-40 backend -- proves file visibility.
 *   [3] OpenLibrary() with name-only (LIBS: walk) at versions
 *       0, 4, 5, 6; then explicit paths to WORK:Libs, RAM:Libs,
 *       LIBS: (primary assign).
 *   [4] OpenLibrary("dos.library", 0) as a known-good sanity check.
 *
 * If [3] succeeds for any variant, the issue is amigit-specific
 * and we bisect amigit startup. If [3] fails uniformly, the issue
 * is environmental and we test hypothesis (a) Kickstart upgrade
 * or (c) run the shipped AmiSSL OpenSSL binary instead.
 */

#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

long __stack = 16384;

static void
try_lock(const char *path)
{
    BPTR lock;
    printf("  Lock(%s): ", path);
    fflush(stdout);
    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (lock == 0) {
        printf("FAIL ioerr=%ld\n", (long)IoErr());
    } else {
        printf("ok\n");
        UnLock(lock);
    }
}

static void
try_open(const char *path, unsigned long version)
{
    struct Library *base;
    printf("  OpenLibrary(%s, v%lu): ", path, version);
    fflush(stdout);
    base = OpenLibrary((CONST_STRPTR)path, version);
    if (base == NULL) {
        printf("NULL\n");
    } else {
        printf("ok v%ld.%ld\n",
               (long)base->lib_Version,
               (long)base->lib_Revision);
        CloseLibrary(base);
    }
}

int
main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("amissl-probe: AmiSSL OpenLibrary diagnostic\n");
    printf("  mem: chip=%lu fast=%lu public=%lu largest=%lu\n",
           (unsigned long)AvailMem(MEMF_CHIP),
           (unsigned long)AvailMem(MEMF_FAST),
           (unsigned long)AvailMem(MEMF_PUBLIC),
           (unsigned long)AvailMem(MEMF_LARGEST));

    printf("\n[1] Lock() tests (file visibility):\n");
    try_lock("WORK:Libs/amisslmaster.library");
    try_lock("RAM:Libs/amisslmaster.library");
    try_lock("LIBS:amisslmaster.library");
    try_lock("WORK:Libs/AmiSSL/68020-40/amissl_v362.library");
    try_lock("RAM:Libs/AmiSSL/68020-40/amissl_v362.library");
    try_lock("LIBS:AmiSSL/68020-40/amissl_v362.library");

    printf("\n[2] OpenLibrary() tests (name-only, LIBS: walk):\n");
    try_open("amisslmaster.library", 0);
    try_open("amisslmaster.library", 4);
    try_open("amisslmaster.library", 5);
    try_open("amisslmaster.library", 6);

    printf("\n[3] OpenLibrary() tests (explicit paths):\n");
    try_open("WORK:Libs/amisslmaster.library", 0);
    try_open("RAM:Libs/amisslmaster.library", 0);
    try_open("LIBS:amisslmaster.library", 0);

    printf("\n[4] Sanity check (known-good):\n");
    try_open("dos.library", 0);

    printf("\namissl-probe: done\n");
    return 0;
}
