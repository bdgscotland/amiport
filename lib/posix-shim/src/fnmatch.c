/*
 * fnmatch.c -- Shell-style pattern matching for AmigaOS
 *
 * amiport: implements POSIX fnmatch() with *, ?, and [...] support.
 * Written in C89 for AmigaOS 3.x compatibility.
 */

#include <amiport/fnmatch.h>
#include <string.h>

/* Check if c is a path separator */
#define IS_SEP(c) ((c) == '/')

/*
 * Match a bracket expression [abc], [a-z], [!abc].
 * Returns pointer past ']' on match, NULL on no match.
 */
static const char *
match_bracket(const char *pattern, char test)
{
	int negate = 0;
	int matched = 0;
	const char *p = pattern;

	if (*p == '!' || *p == '^') {
		negate = 1;
		p++;
	}

	/* Empty bracket expression is invalid; treat ']' as literal if first */
	if (*p == ']') {
		if (test == ']')
			matched = 1;
		p++;
	}

	while (*p != '\0' && *p != ']') {
		char c1 = *p++;
		if (*p == '-' && *(p + 1) != '\0' && *(p + 1) != ']') {
			/* Range: c1-c2 */
			char c2;
			p++; /* skip '-' */
			c2 = *p++;
			if ((unsigned char)test >= (unsigned char)c1 &&
			    (unsigned char)test <= (unsigned char)c2)
				matched = 1;
		} else {
			if (test == c1)
				matched = 1;
		}
	}

	if (*p != ']')
		return NULL; /* unterminated bracket */

	if (negate)
		matched = !matched;

	return matched ? p + 1 : NULL;
}

int
amiport_fnmatch(const char *pattern, const char *string, int flags)
{
	const char *p = pattern;
	const char *s = string;

	while (*p != '\0') {
		switch (*p) {
		case '?':
			if (*s == '\0')
				return FNM_NOMATCH;
			if ((flags & FNM_PATHNAME) && IS_SEP(*s))
				return FNM_NOMATCH;
			if ((flags & FNM_PERIOD) && *s == '.' &&
			    (s == string || ((flags & FNM_PATHNAME) && IS_SEP(*(s - 1)))))
				return FNM_NOMATCH;
			s++;
			p++;
			break;

		case '*': {
			/* Collapse multiple stars */
			while (*p == '*')
				p++;

			/* Check leading dot restriction */
			if ((flags & FNM_PERIOD) && *s == '.' &&
			    (s == string || ((flags & FNM_PATHNAME) && IS_SEP(*(s - 1)))))
				return FNM_NOMATCH;

			/* Trailing * matches everything (respecting PATHNAME) */
			if (*p == '\0') {
				if (flags & FNM_PATHNAME) {
					return (strchr(s, '/') != NULL) ? FNM_NOMATCH : 0;
				}
				return 0;
			}

			/* Try matching * against each position */
			while (*s != '\0') {
				if (amiport_fnmatch(p, s, flags & ~FNM_PERIOD) == 0)
					return 0;
				if ((flags & FNM_PATHNAME) && IS_SEP(*s))
					break;
				s++;
			}
			return FNM_NOMATCH;
		}

		case '[': {
			const char *result;
			if (*s == '\0')
				return FNM_NOMATCH;
			if ((flags & FNM_PATHNAME) && IS_SEP(*s))
				return FNM_NOMATCH;
			if ((flags & FNM_PERIOD) && *s == '.' &&
			    (s == string || ((flags & FNM_PATHNAME) && IS_SEP(*(s - 1)))))
				return FNM_NOMATCH;
			result = match_bracket(p + 1, *s);
			if (result == NULL)
				return FNM_NOMATCH;
			p = result;
			s++;
			break;
		}

		case '\\':
			if (!(flags & FNM_NOESCAPE)) {
				p++;
				if (*p == '\0')
					return FNM_NOMATCH;
			}
			/* FALLTHROUGH */
		default:
			if (*p != *s)
				return FNM_NOMATCH;
			p++;
			s++;
			break;
		}
	}

	return (*s == '\0') ? 0 : FNM_NOMATCH;
}
