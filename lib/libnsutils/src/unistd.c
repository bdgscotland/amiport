/*
 * Copyright 2015 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnsutils.
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * unistd style operations.
 */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "nsutils/unistd.h"

/* exported interface documented in nsutils/unistd.h */
ssize_t nsu_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
#if (defined(__riscos) || defined(__amiga) || defined(_WIN32) || defined(__serenity__))
        off_t sk;

        sk = lseek(fd, offset, SEEK_SET);
        if (sk == (off_t)-1) {
                /* amiport: original upstream had an `if (errno == ESPIPE)`
                 * fallback that called ftruncate(fd, offset) to extend
                 * the file, then retried the seek. libnix `-noixemul`
                 * does NOT provide ftruncate(); calling SetFileSize()
                 * via dos.library would require the libnix fd ->
                 * AmigaDOS BPTR mapping, which libnix does not expose
                 * publicly. We therefore drop the ftruncate fallback
                 * and return -1 / errno=ESPIPE for "seek past EOF".
                 *
                 * Real-world impact: NetSurf's typical pwrite use is
                 * cache write + download write, both of which write
                 * sequentially and never seek past current EOF. The
                 * dropped path is exercised only by code that
                 * explicitly tries to extend a file via pwrite without
                 * a preceding ftruncate -- which is rare and easy to
                 * work around at the call site.
                 */
                return -1;
        }
        return write(fd, buf, count);
#else
        return pwrite(fd, buf, count, offset);
#endif
}

/* exported interface documented in nsutils/unistd.h */
ssize_t nsu_pread(int fd, void *buf, size_t count, off_t offset)
{
#if (defined(__riscos) || defined(_WIN32) || defined(__serenity__) || (defined(__amiga) && !defined(__amigaos4__)))
        off_t sk;

        sk = lseek(fd, offset, SEEK_SET);
        if (sk == -1) {
                return (off_t)-1;
        }
        return read(fd, buf, count);
#else
        return pread(fd, buf, count, offset);
#endif

}
