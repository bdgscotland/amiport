/*
 * amiport-emu/regex.h — POSIX regex emulation for AmigaOS
 *
 * EMULATION NOTICE:
 * This provides a minimal POSIX Basic Regular Expression (BRE) and
 * Extended Regular Expression (ERE) implementation. It is suitable
 * for simple pattern matching (grep, diff -I, sed basics) but has
 * limitations compared to a full POSIX regex:
 *
 * Supported:
 *   - Literal characters, '.', '^', '$'
 *   - Character classes: [abc], [a-z], [^abc]
 *   - Quantifiers: *, +, ? (ERE), \{n,m\} (BRE)
 *   - Alternation: | (ERE only)
 *   - Grouping: () (ERE), \(\) (BRE)
 *   - Backreferences: \1-\9
 *   - Common escapes: \t, \n, \w, \d, \s
 *
 * Limitations:
 *   - No locale-dependent collation ([.ch.], [=a=])
 *   - No POSIX character classes ([:alpha:], [:digit:])
 *   - Maximum pattern length: 512 bytes
 *   - Maximum 9 capture groups
 *   - REG_NEWLINE flag not fully supported
 *   - Performance: backtracking NFA, not suitable for adversarial patterns
 *
 * Based on Henry Spencer's regex library (public domain), adapted for
 * ANSI C89 and AmigaOS.
 */

#ifndef AMIPORT_EMU_REGEX_H
#define AMIPORT_EMU_REGEX_H

#include <stddef.h>

/* Compile flags */
#define REG_EXTENDED  1   /* Use ERE syntax (default is BRE) */
#define REG_ICASE     2   /* Case-insensitive matching */
#define REG_NOSUB     4   /* Don't report match positions */
#define REG_NEWLINE   8   /* ^ and $ match at newlines */

/* Execution flags */
#define REG_NOTBOL    1   /* ^ doesn't match start of string */
#define REG_NOTEOL    2   /* $ doesn't match end of string */

/* Error codes */
#define REG_NOMATCH   1   /* No match */
#define REG_BADPAT    2   /* Invalid pattern */
#define REG_ECOLLATE  3   /* Invalid collating element */
#define REG_ECTYPE    4   /* Invalid character class */
#define REG_EESCAPE   5   /* Trailing backslash */
#define REG_ESUBREG   6   /* Invalid backreference */
#define REG_EBRACK    7   /* Unmatched [ */
#define REG_EPAREN    8   /* Unmatched ( */
#define REG_EBRACE    9   /* Unmatched { */
#define REG_BADBR     10  /* Invalid {} contents */
#define REG_ERANGE    11  /* Invalid range endpoint */
#define REG_ESPACE    12  /* Out of memory */
#define REG_BADRPT    13  /* Invalid repetition */

/* Maximum compiled pattern size */
#define AMIPORT_REGEX_MAX_COMPILED 2048
#define AMIPORT_REGEX_MAX_GROUPS   10

/* Compiled regex (opaque to user) */
typedef struct {
    unsigned char program[AMIPORT_REGEX_MAX_COMPILED];
    int prog_len;
    int cflags;
    size_t re_nsub;   /* Number of subexpressions */
} amiport_emu_regex_t;

/* Match position */
typedef struct {
    int rm_so;  /* Start offset of match (-1 if unused) */
    int rm_eo;  /* End offset of match */
} amiport_emu_regmatch_t;

/*
 * regcomp — compile a regular expression pattern
 *
 * Returns 0 on success, error code on failure.
 */
int amiport_emu_regcomp(amiport_emu_regex_t *preg, const char *pattern,
                         int cflags);

/*
 * regexec — execute a compiled regex against a string
 *
 * Returns 0 on match, REG_NOMATCH on no match.
 * Fills pmatch[0..nmatch-1] with match positions if REG_NOSUB not set.
 */
int amiport_emu_regexec(const amiport_emu_regex_t *preg, const char *string,
                         size_t nmatch, amiport_emu_regmatch_t pmatch[],
                         int eflags);

/*
 * regfree — free compiled regex resources
 *
 * No-op for this implementation (regex_t is stack-allocated),
 * but included for API compatibility.
 */
void amiport_emu_regfree(amiport_emu_regex_t *preg);

/*
 * regerror — get human-readable error string
 *
 * Returns the length of the error message (including NUL).
 */
size_t amiport_emu_regerror(int errcode, const amiport_emu_regex_t *preg,
                             char *errbuf, size_t errbuf_size);

/* Convenience macros for drop-in replacement */
#ifndef AMIPORT_NO_REGEX_MACROS
#define regex_t     amiport_emu_regex_t
#define regmatch_t  amiport_emu_regmatch_t
#define regcomp     amiport_emu_regcomp
#define regexec     amiport_emu_regexec
#define regfree     amiport_emu_regfree
#define regerror    amiport_emu_regerror
#endif

/* Compile-time enable flag */
#ifndef AMIPORT_EMU_REGEX_ENABLED
#define AMIPORT_EMU_REGEX_ENABLED 1
#endif

#endif /* AMIPORT_EMU_REGEX_H */
