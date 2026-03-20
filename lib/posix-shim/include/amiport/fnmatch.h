/*
 * amiport/fnmatch.h -- Shell-style pattern matching for AmigaOS
 *
 * Provides fnmatch() for glob-style wildcard matching.
 */

#ifndef AMIPORT_FNMATCH_H
#define AMIPORT_FNMATCH_H

#define FNM_NOMATCH    1
#define FNM_PATHNAME   (1 << 0)
#define FNM_PERIOD     (1 << 1)
#define FNM_NOESCAPE   (1 << 2)

int amiport_fnmatch(const char *pattern, const char *string, int flags);

#ifndef AMIPORT_NO_FNMATCH_MACROS
#define fnmatch amiport_fnmatch
#endif

#endif /* AMIPORT_FNMATCH_H */
