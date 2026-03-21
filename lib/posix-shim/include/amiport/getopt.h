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

/*
 * getopt_long — parse long options (GNU extension)
 *
 * Supports --option, --option=value, and mixed short/long parsing.
 * longopts is a NULL-terminated array of struct option.
 */
struct option {
    const char *name;   /* long option name */
    int  has_arg;       /* no_argument, required_argument, optional_argument */
    int *flag;          /* if non-NULL, set *flag to val and return 0 */
    int  val;           /* value to return (or store in *flag) */
};

#define no_argument        0
#define required_argument  1
#define optional_argument  2

int amiport_getopt_long(int argc, char * const argv[],
                        const char *optstring,
                        const struct option *longopts, int *longindex);

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_GETOPT_MACROS
#define optarg       amiport_optarg
#define optind       amiport_optind
#define opterr       amiport_opterr
#define optopt       amiport_optopt
#define getopt       amiport_getopt
#define getopt_long  amiport_getopt_long
#endif

#endif /* AMIPORT_GETOPT_H */
