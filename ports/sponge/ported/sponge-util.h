/* See LICENSE file for copyright and license details. */
/* amiport: sponge-util.h -- minimal subset of sbase util.h for sponge.
 * Omits #include <regex.h> and all regex/string/misc helpers not used
 * by sponge, concat, writeall, or eprintf.
 */

#ifndef SPONGE_UTIL_H__
#define SPONGE_UTIL_H__

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>

#include "arg.h"

extern char *argv0;

/* error/warning output */
void eprintf(const char *, ...);
void enprintf(int, const char *, ...);
void weprintf(const char *, ...);

/* i/o helpers used by sponge */
/* amiport: ssize_t replaced with long -- ssize_t not in C89 libnix */
long writeall(int, const void *, size_t);
/* amiport: renamed concat -> sponge_concat to avoid clash with libnix string.h
 * which declares: char *concat(const char *, ...) -- different signature */
int  sponge_concat(int, const char *, int, const char *);

#endif /* SPONGE_UTIL_H__ */
