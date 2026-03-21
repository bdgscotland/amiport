/*
 * getopt.c — Portable getopt implementation
 *
 * Based on public domain implementations.
 * Provides POSIX-compatible getopt() for AmigaOS.
 */

#include <amiport/getopt.h>
#include <stdio.h>
#include <string.h>

char *amiport_optarg = NULL;
int   amiport_optind = 1;
int   amiport_opterr = 1;
int   amiport_optopt = '?';

static int optpos = 0;

int amiport_getopt(int argc, char * const argv[], const char *optstring)
{
    const char *p;

    if (amiport_optind >= argc || argv[amiport_optind] == NULL) {
        return -1;
    }

    if (argv[amiport_optind][0] != '-' || argv[amiport_optind][1] == '\0') {
        return -1;
    }

    if (argv[amiport_optind][1] == '-' && argv[amiport_optind][2] == '\0') {
        amiport_optind++;
        return -1;
    }

    if (optpos == 0) {
        optpos = 1;
    }

    amiport_optopt = argv[amiport_optind][optpos];
    optpos++;

    p = strchr(optstring, amiport_optopt);
    if (p == NULL || amiport_optopt == ':') {
        if (amiport_opterr && optstring[0] != ':') {
            fprintf(stderr, "%s: illegal option -- %c\n",
                    argv[0], amiport_optopt);
        }
        if (argv[amiport_optind][optpos] == '\0') {
            amiport_optind++;
            optpos = 0;
        }
        return '?';
    }

    if (p[1] == ':') {
        /* Option requires an argument */
        if (argv[amiport_optind][optpos] != '\0') {
            /* Argument is rest of current argv element */
            amiport_optarg = &argv[amiport_optind][optpos];
        } else {
            /* Argument is next argv element */
            amiport_optind++;
            if (amiport_optind >= argc) {
                if (amiport_opterr && optstring[0] != ':') {
                    fprintf(stderr, "%s: option requires an argument -- %c\n",
                            argv[0], amiport_optopt);
                }
                optpos = 0;
                return (optstring[0] == ':') ? ':' : '?';
            }
            amiport_optarg = argv[amiport_optind];
        }
        amiport_optind++;
        optpos = 0;
    } else {
        /* No argument */
        amiport_optarg = NULL;
        if (argv[amiport_optind][optpos] == '\0') {
            amiport_optind++;
            optpos = 0;
        }
    }

    return amiport_optopt;
}

/*
 * getopt_long — parse long options (GNU extension)
 *
 * Handles --option, --option=value, and abbreviated long options.
 * Falls through to getopt() for short options.
 */
int amiport_getopt_long(int argc, char * const argv[],
                        const char *optstring,
                        const struct option *longopts, int *longindex)
{
    const char *arg;
    int i;
    const char *eq;
    size_t namelen;

    if (amiport_optind >= argc || argv[amiport_optind] == NULL)
        return -1;

    arg = argv[amiport_optind];

    /* Not a long option — delegate to getopt */
    if (arg[0] != '-' || arg[1] != '-')
        return amiport_getopt(argc, argv, optstring);

    /* "--" alone ends option processing */
    if (arg[2] == '\0') {
        amiport_optind++;
        return -1;
    }

    /* Parse "--name" or "--name=value" */
    arg += 2; /* skip "--" */
    eq = strchr(arg, '=');
    namelen = eq ? (size_t)(eq - arg) : strlen(arg);

    /* Search longopts for a match */
    for (i = 0; longopts[i].name != NULL; i++) {
        if (strncmp(longopts[i].name, arg, namelen) != 0)
            continue;
        if (strlen(longopts[i].name) != namelen)
            continue;

        /* Exact match found */
        if (longindex)
            *longindex = i;

        amiport_optind++;

        if (longopts[i].has_arg == required_argument) {
            if (eq) {
                amiport_optarg = (char *)(eq + 1);
            } else if (amiport_optind < argc) {
                amiport_optarg = argv[amiport_optind++];
            } else {
                if (amiport_opterr)
                    fprintf(stderr, "%s: option '--%s' requires an argument\n",
                            argv[0], longopts[i].name);
                return '?';
            }
        } else if (longopts[i].has_arg == optional_argument) {
            amiport_optarg = eq ? (char *)(eq + 1) : NULL;
        } else {
            amiport_optarg = NULL;
            if (eq) {
                if (amiport_opterr)
                    fprintf(stderr, "%s: option '--%s' doesn't allow an argument\n",
                            argv[0], longopts[i].name);
                return '?';
            }
        }

        if (longopts[i].flag) {
            *longopts[i].flag = longopts[i].val;
            return 0;
        }
        return longopts[i].val;
    }

    /* No match found */
    if (amiport_opterr)
        fprintf(stderr, "%s: unrecognized option '--%.*s'\n",
                argv[0], (int)namelen, arg);
    amiport_optind++;
    return '?';
}
