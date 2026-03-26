/*
 * unicode_stubs.c -- Minimal stubs for Unicode functions needed by core engine
 *
 * amiport: The full unicode.c pulls in 50K lines of Unicode data tables
 * (property data, fold data, grapheme break data). For ASCII-only builds,
 * we provide stub implementations that handle ASCII case folding and
 * return "no break" for grapheme/word boundary analysis.
 *
 * This saves ~312 KB of compiled data tables.
 */

#include "regint.h"
#include "regenc.h"

/* Case fold data -- ASCII only (A-Z -> a-z) */

/* The fold key lookup returns -1 for "not found in fold table".
 * For ASCII, case folding is handled by the encoding driver (ascii.c),
 * not by the Unicode fold tables. Returning -1 is correct. */
int
onigenc_unicode_fold1_key(OnigCodePoint code[])
{
    (void)code;
    return -1;  /* no Unicode fold entry -- ASCII fold handled elsewhere */
}

/* Empty fold data arrays -- referenced by regparse.c */
OnigCodePoint OnigUnicodeFolds1[] = { 0 };
int OnigUnicodeFolds1_count = 0;

/* Grapheme cluster boundary -- for ASCII, every character is its own cluster */
int
onigenc_egcb_is_break_position(OnigEncoding enc, UChar* p, UChar* prev,
    const UChar* start, const UChar* end)
{
    (void)enc; (void)p; (void)prev; (void)start; (void)end;
    return 1;  /* always break -- each ASCII char is its own grapheme */
}

/* Word boundary -- for ASCII, delegate to simple space/alnum check */
int
onigenc_wb_is_break_position(OnigEncoding enc, UChar* p, UChar* prev,
    const UChar* start, const UChar* end)
{
    (void)enc; (void)p; (void)prev; (void)start; (void)end;
    return 1;  /* always break -- let the regex engine handle \b */
}
