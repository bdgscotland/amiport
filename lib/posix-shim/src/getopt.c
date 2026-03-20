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
