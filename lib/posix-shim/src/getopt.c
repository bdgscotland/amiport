/*
 * getopt.c — Portable getopt/getopt_long implementation
 *
 * Provides POSIX getopt() + GNU getopt_long() extensions for AmigaOS:
 *   - optind=0 restart (GNU extension)
 *   - Argument permutation in getopt_long (GNU extension):
 *     options can appear after non-option arguments.
 */

#include <amiport/getopt.h>
#include <stdio.h>
#include <string.h>

char *amiport_optarg = NULL;
int   amiport_optind = 1;
int   amiport_opterr = 1;
int   amiport_optopt = '?';

static int optpos = 0;

/*
 * amiport_getopt — POSIX getopt (no permutation).
 * Stops at first non-option argument.
 */
int amiport_getopt(int argc, char * const argv[], const char *optstring)
{
    const char *p;

    if (amiport_optind == 0) {
        amiport_optind = 1;
        optpos = 0;
    }

    if (amiport_optind >= argc || argv[amiport_optind] == NULL)
        return -1;

    if (argv[amiport_optind][0] != '-' || argv[amiport_optind][1] == '\0')
        return -1;

    if (argv[amiport_optind][1] == '-' && argv[amiport_optind][2] == '\0') {
        amiport_optind++;
        return -1;
    }

    if (optpos == 0)
        optpos = 1;

    amiport_optopt = argv[amiport_optind][optpos];
    optpos++;

    p = strchr(optstring, amiport_optopt);
    if (p == NULL || amiport_optopt == ':') {
        if (amiport_opterr && optstring[0] != ':')
            fprintf(stderr, "%s: illegal option -- %c\n",
                    argv[0], amiport_optopt);
        if (argv[amiport_optind][optpos] == '\0') {
            amiport_optind++;
            optpos = 0;
        }
        return '?';
    }

    if (p[1] == ':') {
        if (argv[amiport_optind][optpos] != '\0') {
            amiport_optarg = &argv[amiport_optind][optpos];
        } else {
            amiport_optind++;
            if (amiport_optind >= argc) {
                if (amiport_opterr && optstring[0] != ':')
                    fprintf(stderr, "%s: option requires an argument -- %c\n",
                            argv[0], amiport_optopt);
                optpos = 0;
                return (optstring[0] == ':') ? ':' : '?';
            }
            amiport_optarg = argv[amiport_optind];
        }
        amiport_optind++;
        optpos = 0;
    } else {
        amiport_optarg = NULL;
        if (argv[amiport_optind][optpos] == '\0') {
            amiport_optind++;
            optpos = 0;
        }
    }

    return amiport_optopt;
}

/*
 * Rotate a single argv element from position 'from' to position 'to',
 * shifting elements [to..from-1] right by one. Casts away const.
 */
static void
shift_arg(char * const argv[], int to, int from)
{
    char **av = (char **)argv;
    char *tmp = av[from];
    int k;
    for (k = from; k > to; k--)
        av[k] = av[k - 1];
    av[to] = tmp;
}

/*
 * Determine how many argv elements an option at argv[pos] consumes.
 * Returns 1 (just the option) or 2 (option + separate argument).
 */
static int
option_arg_count(char * const argv[], int pos, int argc,
                 const char *optstring, const struct option *longopts)
{
    const char *arg = argv[pos];

    if (arg[1] == '-') {
        /* Long option --name or --name=value */
        if (strchr(arg, '=') != NULL)
            return 1; /* value embedded in = */
        /* Check longopts for has_arg */
        const char *name = arg + 2;
        size_t nlen = strlen(name);
        int i;
        for (i = 0; longopts[i].name != NULL; i++) {
            if (strncmp(longopts[i].name, name, nlen) == 0 &&
                strlen(longopts[i].name) == nlen) {
                if (longopts[i].has_arg == required_argument &&
                    pos + 1 < argc)
                    return 2;
                return 1;
            }
        }
        return 1; /* Unknown option, consume just the flag */
    }

    /* Short option -X or -Xvalue */
    if (arg[2] != '\0')
        return 1; /* Value embedded: -Ofile */
    /* Single-char option: check optstring for ':' */
    const char *p = strchr(optstring, arg[1]);
    if (p && p[1] == ':' && pos + 1 < argc)
        return 2; /* Separate argument */
    return 1;
}

/*
 * getopt_long — GNU extension with argument permutation.
 *
 * When a non-option is found, scans forward for the next option,
 * rotates it (and its argument) into position, then processes it.
 * After all options are consumed, optind points to the first non-option.
 */
int amiport_getopt_long(int argc, char * const argv[],
                        const char *optstring,
                        const struct option *longopts, int *longindex)
{
    const char *arg;
    int i;
    const char *eq;
    size_t namelen;

    if (amiport_optind == 0) {
        amiport_optind = 1;
        optpos = 0;
    }

    if (amiport_optind >= argc || argv[amiport_optind] == NULL)
        return -1;

    arg = argv[amiport_optind];

    /* If current arg is a non-option, find the next option and rotate it here */
    if (arg[0] != '-' || arg[1] == '\0') {
        /* Scan forward for next option */
        int j;
        for (j = amiport_optind + 1; j < argc; j++) {
            if (argv[j] == NULL)
                break;
            if (argv[j][0] == '-' && argv[j][1] != '\0')
                break; /* Found option */
        }
        if (j >= argc)
            return -1; /* No more options — optind stays at first non-option */

        /* Check for "--" end-of-options marker */
        if (argv[j][1] == '-' && argv[j][2] == '\0') {
            /* Rotate "--" to optind, so non-options after it stay in order */
            shift_arg(argv, amiport_optind, j);
            amiport_optind++;
            return -1;
        }

        /* Determine how many args the option consumes */
        int consume = option_arg_count(argv, j, argc, optstring, longopts);

        /* Rotate option (and its arg) from position j to optind */
        shift_arg(argv, amiport_optind, j);
        if (consume == 2 && j + 1 < argc) {
            /* The argument was at j+1, now at j (shifted by previous rotate).
               Move it to optind+1. */
            shift_arg(argv, amiport_optind + 1, j + 1);
        }

        arg = argv[amiport_optind];
    }

    /* Short option: delegate to amiport_getopt */
    if (arg[1] != '-')
        return amiport_getopt(argc, argv, optstring);

    /* "--" alone ends option processing */
    if (arg[2] == '\0') {
        amiport_optind++;
        return -1;
    }

    /* Parse "--name" or "--name=value" */
    arg += 2;
    eq = strchr(arg, '=');
    namelen = eq ? (size_t)(eq - arg) : strlen(arg);

    for (i = 0; longopts[i].name != NULL; i++) {
        if (strncmp(longopts[i].name, arg, namelen) != 0)
            continue;
        if (strlen(longopts[i].name) != namelen)
            continue;

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

    if (amiport_opterr)
        fprintf(stderr, "%s: unrecognized option '--%.*s'\n",
                argv[0], (int)namelen, arg);
    amiport_optind++;
    return '?';
}
