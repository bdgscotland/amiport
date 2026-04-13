/*
 * cmd_version.c -- `amigit version`
 *
 * Prints three lines identifying the amigit build, the linked libgit2
 * version, and the amiport shim that makes the port possible. Takes no
 * arguments; any extra argv after "version" is ignored with a warning.
 *
 * Exit: 0 on success. Never errors under normal conditions.
 */

#include <stdio.h>
#include "git2.h"
#include <dos/dos.h>    /* RETURN_OK, RETURN_WARN */

#include "amigit.h"

int amigit_cmd_version(int argc, char **argv)
{
    int major = 0, minor = 0, patch = 0;

    (void)argv;

    if (argc > 2) {
        /* amigit version takes no args. Don't hard-fail -- just note
         * it. Return RETURN_WARN so scripts that check IF WARN can
         * detect the mistake but scripts that check IF ERROR still
         * see success. */
        fprintf(stderr, "amigit: 'version' takes no arguments\n");
    }

    /* libgit2 exposes its version via git_libgit2_version. Safe to
     * call before git_libgit2_init (no allocation, no state). */
    (void)git_libgit2_version(&major, &minor, &patch);

    printf("amigit %s (built 2026-04-13)\n", AMIGIT_VERSION);
    printf("libgit2 %d.%d.%d\n", major, minor, patch);
    printf("amiport posix-shim available\n");

    if (argc > 2) {
        return RETURN_WARN;
    }
    return RETURN_OK;
}
