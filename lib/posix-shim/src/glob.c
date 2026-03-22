/*
 * glob.c -- POSIX glob/globfree for AmigaOS
 *
 * amiport: implements glob() using amiport_opendir/readdir + amiport_fnmatch.
 * Supports both Unix and AmigaOS wildcard patterns via translation layer.
 * Written in C89 for AmigaOS 3.x compatibility.
 */

#include <amiport/glob.h>
#include <amiport/fnmatch.h>
#include <amiport/dirent.h>
#include <amiport/unistd.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define GLOB_TRANSLATE_BUF 512

/* --- Amiga-to-Unix pattern translation --- */

/*
 * Translate AmigaOS pattern metacharacters to Unix equivalents.
 * #? -> *, #x -> x* (repeat), (a|b) left for iterative matching.
 * Returns 1 if translation was performed, 0 if pattern was already Unix.
 * Result written to 'out' buffer of size 'outsize'.
 */
static int
translate_amiga_pattern(const char *pattern, char *out, size_t outsize)
{
	const char *p = pattern;
	char *o = out;
	char *end = out + outsize - 1;
	int translated = 0;

	while (*p != '\0' && o < end) {
		if (p[0] == '#' && p[1] == '?') {
			/* #? -> * */
			*o++ = '*';
			p += 2;
			translated = 1;
		} else if (p[0] == '#' && p[1] != '\0' && p[1] != '#') {
			/* #x -> x* (match x zero or more times) */
			if (o + 1 < end) {
				*o++ = p[1];
				*o++ = '*';
			}
			p += 2;
			translated = 1;
		} else {
			*o++ = *p++;
		}
	}
	*o = '\0';
	return translated;
}

/*
 * amiport_glob_has_magic — shared wildcard detection.
 * Checks for Unix (*, ?, [) and AmigaOS (#, ~, () metacharacters.
 */
int
amiport_glob_has_magic(const char *pattern)
{
	const char *p;

	if (pattern == NULL)
		return 0;

	for (p = pattern; *p != '\0'; p++) {
		switch (*p) {
		case '*':
		case '?':
		case '[':
			return 1;
		case '#':
			/* # followed by any char is an Amiga pattern */
			if (p[1] != '\0')
				return 1;
			break;
		case '~':
			/* ~ followed by ( is Amiga negation */
			if (p[1] == '(')
				return 1;
			break;
		case '(':
			/* (a|b) alternation — check for | inside */
			{
				const char *q;
				for (q = p + 1; *q != '\0' && *q != ')'; q++) {
					if (*q == '|')
						return 1;
				}
			}
			break;
		case '\\':
			/* Skip escaped character */
			if (p[1] != '\0')
				p++;
			break;
		}
	}
	return 0;
}

/*
 * Split pattern into directory and filename components.
 * "src/x.c"       -> dir="src",      pat="x.c"
 * "x.c"           -> dir="",         pat="x.c"
 * "Work:src/x.c"  -> dir="Work:src", pat="x.c"
 * "Work:x.c"      -> dir="Work:",    pat="x.c"
 * (where x.c may contain wildcards like * or #?)
 */
static const char *
split_pattern(const char *pattern, char *dir_buf, size_t dir_size)
{
	const char *last_sep = NULL;
	const char *last_colon = NULL;
	const char *p;

	for (p = pattern; *p != '\0'; p++) {
		if (*p == '/')
			last_sep = p;
		else if (*p == ':')
			last_colon = p;
	}

	/* Use whichever separator came last */
	if (last_sep != NULL && (last_colon == NULL || last_sep > last_colon)) {
		size_t len = (size_t)(last_sep - pattern);
		if (len == 0) {
			strncpy(dir_buf, ".", dir_size - 1);
			dir_buf[dir_size - 1] = '\0';
		} else {
			if (len >= dir_size) len = dir_size - 1;
			memcpy(dir_buf, pattern, len);
			dir_buf[len] = '\0';
		}
		return last_sep + 1;
	} else if (last_colon != NULL) {
		/* Amiga volume/assign: include the colon */
		size_t len = (size_t)(last_colon - pattern) + 1;
		if (len >= dir_size) len = dir_size - 1;
		memcpy(dir_buf, pattern, len);
		dir_buf[len] = '\0';
		return last_colon + 1;
	}

	/* No separator — pattern is in current directory */
	dir_buf[0] = '\0';
	return pattern;
}

/*
 * Add a path to the glob result array, growing as needed.
 */
static int
glob_add(amiport_glob_t *pglob, const char *path)
{
	size_t new_count = pglob->gl_pathc + 1;
	char **new_pathv;
	char *copy;

	new_pathv = (char **)realloc(pglob->gl_pathv,
	                             (new_count + 1) * sizeof(char *));
	if (new_pathv == NULL)
		return GLOB_NOSPACE;
	pglob->gl_pathv = new_pathv;

	copy = (char *)malloc(strlen(path) + 1);
	if (copy == NULL)
		return GLOB_NOSPACE;
	strcpy(copy, path);

	pglob->gl_pathv[pglob->gl_pathc] = copy;
	pglob->gl_pathc = new_count;
	pglob->gl_pathv[new_count] = NULL;

	return 0;
}

static int
glob_compare(const void *a, const void *b)
{
	return strcmp(*(const char * const *)a, *(const char * const *)b);
}

/*
 * Build full path from directory and filename.
 */
static void
build_path(const char *dir, const char *name, char *buf, size_t bufsize)
{
	size_t dlen;

	if (dir[0] == '\0') {
		strncpy(buf, name, bufsize - 1);
		buf[bufsize - 1] = '\0';
		return;
	}

	dlen = strlen(dir);
	strncpy(buf, dir, bufsize - 1);
	buf[bufsize - 1] = '\0';

	/* Add separator if needed (not after : which is already a separator) */
	if (dlen > 0 && dir[dlen - 1] != '/' && dir[dlen - 1] != ':') {
		if (dlen + 1 < bufsize) {
			buf[dlen] = '/';
			buf[dlen + 1] = '\0';
			dlen++;
		}
	}
	strncat(buf, name, bufsize - dlen - 1);
}

int
amiport_glob(const char *pattern, int flags,
             int (*errfunc)(const char *, int),
             amiport_glob_t *pglob)
{
	char dir_buf[256];
	char translate_buf[GLOB_TRANSLATE_BUF];
	char path_buf[512];
	const char *file_pattern;
	const char *match_pattern;
	AMIPORT_DIR *dir;
	struct amiport_dirent *ent;
	const char *dir_path;
	size_t pre_count;
	int rc;

	if (pattern == NULL || pglob == NULL)
		return GLOB_ABORTED;

	/* Initialize or preserve based on GLOB_APPEND */
	if (!(flags & GLOB_APPEND)) {
		pglob->gl_pathc = 0;
		pglob->gl_pathv = NULL;
		pglob->gl_offs = 0;
	}

	pre_count = pglob->gl_pathc;

	/* Split pattern into directory and filename */
	file_pattern = split_pattern(pattern, dir_buf, sizeof(dir_buf));

	/* Translate AmigaOS patterns to Unix if needed */
	if (translate_amiga_pattern(file_pattern, translate_buf,
	                            sizeof(translate_buf))) {
		match_pattern = translate_buf;
	} else {
		match_pattern = file_pattern;
	}

	/* If no wildcards in filename part, treat as literal */
	if (!amiport_glob_has_magic(match_pattern)) {
		int fd;
		build_path(dir_buf, file_pattern, path_buf, sizeof(path_buf));

		/* Check if it exists as a file */
		fd = amiport_open(path_buf, 0 /* O_RDONLY */);
		if (fd >= 0) {
			amiport_close(fd);
			rc = glob_add(pglob, path_buf);
			if (rc != 0) return rc;
			goto done;
		}

		/* Check if it exists as a directory */
		{
			AMIPORT_DIR *probe = amiport_opendir(path_buf);
			if (probe != NULL) {
				amiport_closedir(probe);
				if (flags & GLOB_MARK) {
					size_t plen = strlen(path_buf);
					if (plen + 1 < sizeof(path_buf)) {
						path_buf[plen] = '/';
						path_buf[plen + 1] = '\0';
					}
				}
				rc = glob_add(pglob, path_buf);
				if (rc != 0) return rc;
				goto done;
			}
		}

		/* Does not exist */
		if (flags & GLOB_NOCHECK) {
			rc = glob_add(pglob, pattern);
			if (rc != 0) return rc;
			goto done;
		}
		return GLOB_NOMATCH;
	}

	/* Open directory for scanning */
	dir_path = (dir_buf[0] != '\0') ? dir_buf : ".";
	dir = amiport_opendir(dir_path);
	if (dir == NULL) {
		if (errfunc != NULL) {
			if (errfunc(dir_path, errno) != 0 || (flags & GLOB_ERR))
				return GLOB_ABORTED;
		} else if (flags & GLOB_ERR) {
			return GLOB_ABORTED;
		}
		if (flags & GLOB_NOCHECK) {
			rc = glob_add(pglob, pattern);
			return (rc != 0) ? rc : 0;
		}
		return GLOB_NOMATCH;
	}

	/* Iterate directory entries, match against pattern */
	while ((ent = amiport_readdir(dir)) != NULL) {
		/* Skip hidden files unless pattern starts with . */
		if (ent->d_name[0] == '.' && match_pattern[0] != '.')
			continue;

		if (amiport_fnmatch(match_pattern, ent->d_name, 0) == 0) {
			build_path(dir_buf, ent->d_name, path_buf, sizeof(path_buf));

			if ((flags & GLOB_MARK) &&
			    ent->d_type == AMIPORT_DT_DIR) {
				size_t plen = strlen(path_buf);
				if (plen + 1 < sizeof(path_buf)) {
					path_buf[plen] = '/';
					path_buf[plen + 1] = '\0';
				}
			}

			rc = glob_add(pglob, path_buf);
			if (rc != 0) {
				amiport_closedir(dir);
				return rc;
			}
		}
	}

	amiport_closedir(dir);

done:
	/* No new matches found? */
	if (pglob->gl_pathc == pre_count) {
		if (flags & GLOB_NOCHECK) {
			rc = glob_add(pglob, pattern);
			if (rc != 0) return rc;
		} else {
			return GLOB_NOMATCH;
		}
	}

	/* Sort unless GLOB_NOSORT */
	if (!(flags & GLOB_NOSORT) && pglob->gl_pathc > 1) {
		qsort(pglob->gl_pathv, pglob->gl_pathc, sizeof(char *),
		      glob_compare);
	}

	return 0;
}

void
amiport_globfree(amiport_glob_t *pglob)
{
	size_t i;

	if (pglob == NULL)
		return;

	if (pglob->gl_pathv != NULL) {
		for (i = 0; i < pglob->gl_pathc; i++) {
			free(pglob->gl_pathv[i]);
		}
		free(pglob->gl_pathv);
		pglob->gl_pathv = NULL;
	}
	pglob->gl_pathc = 0;
}
