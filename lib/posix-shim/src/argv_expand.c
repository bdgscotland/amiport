/*
 * argv_expand.c — Automatic argv wildcard expansion for AmigaOS
 *
 * amiport: provides amiport_expand_argv() to expand wildcard arguments
 * in the argv array, matching the convention of SAS/C, DICE, and Lattice
 * compilers. Programs that take pattern arguments (grep, sed) should
 * define `int __nowild = 1;` to suppress expansion.
 *
 * Written in C89 for AmigaOS 3.x compatibility.
 */

#include <amiport/glob.h>

/* From glob.c — shared wildcard detection */
extern int amiport_glob_has_magic(const char *pattern);

#include <stdlib.h>
#include <string.h>

/*
 * __progname — OpenBSD programs use this for error messages.
 * bebbo-gcc libnix does NOT provide it, so we define it here and
 * initialize it from argv[0] inside amiport_expand_argv().
 * GCC weak attribute: if the port defines its own __progname,
 * the port's definition takes precedence.
 */
char *__progname __attribute__((weak)) = "?";

/* Maximum expanded arguments — prevent runaway expansion */
#define MAX_EXPANDED_ARGS 4096

/* Track allocated memory for cleanup */
static char **expanded_argv = NULL;
static char **expanded_strings = NULL;
static int expanded_string_count = 0;
static int did_expand = 0;

/*
 * __nowild opt-out flag — SAS/C convention.
 * Default is 0 (expansion enabled). Ports that take pattern arguments
 * (grep, sed) define `int __nowild = 1;` to suppress expansion.
 * GCC weak attribute: if the port defines __nowild, the port's
 * definition takes precedence over this default.
 */
int __nowild __attribute__((weak)) = 0;

/*
 * Check if an argument should be expanded.
 * Skip argv[0] (program name), options (starting with -), and literals.
 */
static int
should_expand(const char *arg, int index)
{
	if (index == 0)
		return 0;  /* Never expand program name */
	if (arg[0] == '-')
		return 0;  /* Never expand option flags */
	return amiport_glob_has_magic(arg);
}

void
amiport_expand_argv(int *argc_p, char ***argv_p)
{
	int orig_argc;
	char **orig_argv;
	int new_argc;
	int i;
	int str_alloc;
	int has_wild;

	if (argc_p == NULL || argv_p == NULL)
		return;

	/*
	 * Initialize __progname from argv[0].
	 * Strip path prefix (/ for Unix, : for Amiga volume separator).
	 * This runs before __nowild check so progname is always set.
	 */
	if (*argc_p > 0 && (*argv_p)[0] != NULL) {
		char *p = strrchr((*argv_p)[0], '/');
		if (p == NULL)
			p = strrchr((*argv_p)[0], ':');
		__progname = (p != NULL) ? p + 1 : (*argv_p)[0];
	}

	/* Check __nowild opt-out — SAS/C convention */
	if (__nowild != 0)
		return;

	orig_argc = *argc_p;
	orig_argv = *argv_p;

	/* Quick scan: any wildcards at all? */
	has_wild = 0;
	for (i = 1; i < orig_argc; i++) {
		if (should_expand(orig_argv[i], i)) {
			has_wild = 1;
			break;
		}
	}
	if (!has_wild)
		return;

	/* Allocate tracking arrays */
	str_alloc = 64;
	expanded_strings = (char **)malloc((size_t)str_alloc * sizeof(char *));
	if (expanded_strings == NULL)
		return;  /* OOM — leave argv unchanged */
	expanded_string_count = 0;

	/* Build new argv */
	new_argc = 0;
	expanded_argv = (char **)malloc((size_t)(orig_argc + 128) * sizeof(char *));
	if (expanded_argv == NULL) {
		free(expanded_strings);
		expanded_strings = NULL;
		return;
	}

	/* argv[0] always passes through */
	expanded_argv[new_argc++] = orig_argv[0];

	for (i = 1; i < orig_argc; i++) {
		if (should_expand(orig_argv[i], i)) {
			amiport_glob_t g;
			int rc;
			size_t j;

			rc = amiport_glob(orig_argv[i], GLOB_NOCHECK | GLOB_NOSORT,
			                  NULL, &g);
			if (rc == 0 && g.gl_pathc > 0) {
				/* Grow expanded_argv if needed */
				{
					size_t needed = (size_t)new_argc + g.gl_pathc +
					                (size_t)(orig_argc - i) + 1;
					char **tmp = (char **)realloc(expanded_argv,
					                              needed * sizeof(char *));
					if (tmp == NULL) {
						amiport_globfree(&g);
						break;
					}
					expanded_argv = tmp;
				}

				for (j = 0; j < g.gl_pathc; j++) {
					char *copy;
					if (new_argc >= MAX_EXPANDED_ARGS)
						break;

					copy = (char *)malloc(strlen(g.gl_pathv[j]) + 1);
					if (copy == NULL)
						break;
					strcpy(copy, g.gl_pathv[j]);

					expanded_argv[new_argc++] = copy;

					/* Track for cleanup */
					if (expanded_string_count >= str_alloc) {
						int new_alloc = str_alloc * 2;
						char **tmp = (char **)realloc(expanded_strings,
						                (size_t)new_alloc * sizeof(char *));
						if (tmp == NULL)
							break;
						expanded_strings = tmp;
						str_alloc = new_alloc;
					}
					expanded_strings[expanded_string_count++] = copy;
				}

				amiport_globfree(&g);
			} else {
				/* No expansion — keep original */
				expanded_argv[new_argc++] = orig_argv[i];
			}
		} else {
			/* Not a wildcard — pass through */
			expanded_argv[new_argc++] = orig_argv[i];
		}
	}

	expanded_argv[new_argc] = NULL;
	did_expand = 1;

	*argc_p = new_argc;
	*argv_p = expanded_argv;
}

void
amiport_free_argv(void)
{
	int i;

	if (!did_expand)
		return;

	if (expanded_strings != NULL) {
		for (i = 0; i < expanded_string_count; i++) {
			free(expanded_strings[i]);
		}
		free(expanded_strings);
		expanded_strings = NULL;
	}
	expanded_string_count = 0;

	if (expanded_argv != NULL) {
		free(expanded_argv);
		expanded_argv = NULL;
	}

	did_expand = 0;
}
