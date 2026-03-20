/*
 * amiport/getopt.h — Portable getopt implementation for AmigaOS
 *
 * Based on public domain getopt implementations.
 * Provides getopt(), optarg, optind, opterr, optopt.
 */

#ifndef AMIPORT_GETOPT_H
#define AMIPORT_GETOPT_H

extern char *amiport_optarg;
extern int   amiport_optind;
extern int   amiport_opterr;
extern int   amiport_optopt;

int amiport_getopt(int argc, char * const argv[], const char *optstring);

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_GETOPT_MACROS
#define optarg   amiport_optarg
#define optind   amiport_optind
#define opterr   amiport_opterr
#define optopt   amiport_optopt
#define getopt   amiport_getopt
#endif

#endif /* AMIPORT_GETOPT_H */
