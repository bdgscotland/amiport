/*
 * amiport/scandir.h -- POSIX scandir/alphasort shim for AmigaOS
 *
 * Provides scandir() and alphasort() using amiport directory operations.
 */

#ifndef AMIPORT_SCANDIR_H
#define AMIPORT_SCANDIR_H

#include <amiport/dirent.h>

int amiport_scandir(const char *dirname, struct amiport_dirent ***namelist,
    int (*select)(const struct amiport_dirent *),
    int (*compar)(const struct amiport_dirent **, const struct amiport_dirent **));

int amiport_alphasort(const struct amiport_dirent **a, const struct amiport_dirent **b);

#ifndef AMIPORT_NO_SCANDIR_MACROS
#define scandir   amiport_scandir
#define alphasort amiport_alphasort
#endif

#endif /* AMIPORT_SCANDIR_H */
