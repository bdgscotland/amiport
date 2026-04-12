/* See LICENSE file for copyright and license details. */
/* amiport: added AmigaOS shim headers, replaced POSIX file I/O */
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/* amiport: use libnix native unistd.h for read/write/lseek/close --
 * sponge passes libnix fd 0 (stdin) to concat(), so ALL fd operations
 * must use the libnix fd namespace, not amiport's (crash-patterns #12).
 * libnix provides mkstemp, open, close, read, write, lseek natively. */
#include <unistd.h>
/* amiport: replaced <stdlib.h> with amiport/stdlib.h for exit() -> amiport_exit() */
#include <amiport/stdlib.h>
/* amiport: argv wildcard expansion */
#include <amiport/glob.h>

#include "sponge-util.h"

/* amiport: version string */
static const char *verstag = "$VER: sponge 0.1 (11.04.2026)";

/* amiport: stack cookie -- sponge is not recursive but uses mkstemp/concat */
long __stack = 16384;

/* amiport: temp file path for atexit cleanup */
static char s_tmp[32];
static int  s_tmp_active = 0;

static void
cleanup(void)
{
    /* amiport: free wildcard-expanded argv */
    amiport_free_argv();
    /* amiport: remove temp file if it still exists.
     * Use remove() (libnix) not amiport_unlink() since we're
     * using libnix fd namespace throughout sponge. */
    if (s_tmp_active) {
        remove(s_tmp);
        s_tmp_active = 0;
    }
    (void)fflush(stdout);
}

static void
usage(void)
{
    eprintf("usage: %s file\n", argv0);
}

int
main(int argc, char *argv[])
{
    int fd, tmpfd;

    /* amiport: expand AmigaOS wildcards before ARGBEGIN processes argv */
    amiport_expand_argv(&argc, &argv);
    atexit(cleanup);

    ARGBEGIN {
    default:
        usage();
    } ARGEND

    if (argc != 1)
        usage();

    /* amiport: replaced "/tmp/sponge-XXXXXX" with "T:sponge-XXXXXX"
     * (no /tmp on AmigaOS; T: maps to RAM:T/)
     * Copy into static s_tmp so atexit cleanup can call amiport_unlink(s_tmp). */
    strcpy(s_tmp, "T:sponge-XXXXXX");

    /* amiport: mkstemp -> amiport_mkstemp (via macro in amiport/stdio_ext.h) */
    if ((tmpfd = mkstemp(s_tmp)) < 0)
        eprintf("mkstemp:");

    /* amiport: deferred unlink -- do NOT unlink here.
     * On AmigaOS, unlinking an open file deletes it immediately (no
     * Unix-style "unlink but keep open" semantics).  Instead we record
     * the path and clean up in the atexit handler after all I/O is done.
     */
    s_tmp_active = 1;

    if (sponge_concat(0, "<stdin>", tmpfd, "<tmpfile>") < 0) {
        /* amiport: exit(1) -> exit(10) -- Amiga error return code */
        exit(10);
    }

    /* amiport: lseek -> amiport_lseek (via macro in amiport/unistd.h) */
    if (lseek(tmpfd, 0, SEEK_SET) < 0)
        eprintf("lseek:");

    /* amiport: creat() -> amiport_open() with O_WRONLY|O_CREAT|O_TRUNC.
     * creat() is not shimmed; use open() (mapped to amiport_open via macro). */
    if ((fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC)) < 0)
        eprintf("creat %s:", argv[0]);

    if (sponge_concat(tmpfd, "<tmpfile>", fd, argv[0]) < 0) {
        /* amiport: exit(1) -> exit(10) */
        exit(10);
    }

    /* amiport: close fds before atexit cleanup removes the temp file */
    close(tmpfd);
    close(fd);

    /* amiport: temp file is removed by atexit cleanup() */

    return 0;
}
