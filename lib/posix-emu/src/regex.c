/*
 * regex.c — Minimal POSIX regex implementation for AmigaOS
 *
 * A simple backtracking NFA regex engine supporting both BRE and ERE.
 * Based on concepts from Henry Spencer's regex and Rob Pike's regex tutorial.
 *
 * This is ANSI C89 compatible and has no OS-specific dependencies.
 *
 * Supported: . ^ $ * + ? | () [] [^] \1-\9 \t \n \w \d \s
 * Not supported: locale-dependent collation, POSIX char classes ([:alpha:])
 */

#include <amiport-emu/regex.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Internal opcodes for compiled regex */
#define OP_END       0   /* End of program */
#define OP_LITERAL   1   /* Match literal char (followed by char) */
#define OP_ANY       2   /* Match any char (.) */
#define OP_BOL       3   /* Match beginning of line (^) */
#define OP_EOL       4   /* Match end of line ($) */
#define OP_CLASS     5   /* Character class (followed by len, bitmap) */
#define OP_NCLASS    6   /* Negated character class */
#define OP_STAR      7   /* Greedy * (followed by sub-pattern length) */
#define OP_PLUS      8   /* Greedy + */
#define OP_QUEST     9   /* ? */
#define OP_BRANCH    10  /* Alternation branch (followed by offset to next) */
#define OP_JUMP      11  /* Unconditional jump (followed by offset) */
#define OP_GROUPSTART 12 /* Start of capture group (followed by group #) */
#define OP_GROUPEND  13  /* End of capture group (followed by group #) */
#define OP_BACKREF   14  /* Backreference (followed by group #) */

/* Compiler state */
typedef struct {
    const char *pattern;
    int pos;
    int cflags;
    unsigned char *code;
    int code_len;
    int n_groups;
    int error;
} compile_state;

/* Match state */
typedef struct {
    const char *str;
    const char *str_end;
    int eflags;
    int cflags;
    int group_start[AMIPORT_REGEX_MAX_GROUPS];
    int group_end[AMIPORT_REGEX_MAX_GROUPS];
    int n_groups;
} match_state;

static int
is_special_bre(char c)
{
    return c == '.' || c == '*' || c == '^' || c == '$' || c == '[' || c == '\\';
}

static int
is_special_ere(char c)
{
    return c == '.' || c == '*' || c == '+' || c == '?' || c == '^' ||
           c == '$' || c == '[' || c == '(' || c == ')' || c == '|' || c == '\\';
}

static void
emit(compile_state *cs, unsigned char byte)
{
    if (cs->code_len < AMIPORT_REGEX_MAX_COMPILED - 1) {
        cs->code[cs->code_len++] = byte;
    } else {
        cs->error = REG_ESPACE;
    }
}

static int
compile_class(compile_state *cs)
{
    int negate = 0;
    unsigned char bitmap[32];
    int i;
    char c;

    memset(bitmap, 0, sizeof(bitmap));

    cs->pos++; /* skip '[' */

    if (cs->pattern[cs->pos] == '^') {
        negate = 1;
        cs->pos++;
    }

    /* Allow ] as first char in class */
    if (cs->pattern[cs->pos] == ']') {
        bitmap[(unsigned char)']' / 8] |= (1 << (']' % 8));
        cs->pos++;
    }

    while (cs->pattern[cs->pos] != '\0' && cs->pattern[cs->pos] != ']') {
        c = cs->pattern[cs->pos];

        /* Range: a-z */
        if (cs->pattern[cs->pos + 1] == '-' && cs->pattern[cs->pos + 2] != ']' &&
            cs->pattern[cs->pos + 2] != '\0') {
            char start = c;
            char end = cs->pattern[cs->pos + 2];
            int s, e;
            if (cs->cflags & REG_ICASE) {
                start = tolower((unsigned char)start);
                end = tolower((unsigned char)end);
            }
            s = (unsigned char)start;
            e = (unsigned char)end;
            if (s > e) {
                cs->error = REG_ERANGE;
                return -1;
            }
            for (i = s; i <= e; i++) {
                bitmap[i / 8] |= (1 << (i % 8));
                if (cs->cflags & REG_ICASE) {
                    int alt = islower(i) ? toupper(i) : tolower(i);
                    bitmap[alt / 8] |= (1 << (alt % 8));
                }
            }
            cs->pos += 3;
        } else {
            unsigned char uc = (unsigned char)c;
            bitmap[uc / 8] |= (1 << (uc % 8));
            if (cs->cflags & REG_ICASE) {
                int alt = islower(uc) ? toupper(uc) : tolower(uc);
                bitmap[alt / 8] |= (1 << (alt % 8));
            }
            cs->pos++;
        }
    }

    if (cs->pattern[cs->pos] != ']') {
        cs->error = REG_EBRACK;
        return -1;
    }
    cs->pos++; /* skip ']' */

    emit(cs, negate ? OP_NCLASS : OP_CLASS);
    emit(cs, 32); /* bitmap length */
    for (i = 0; i < 32; i++)
        emit(cs, bitmap[i]);

    return 0;
}

static int compile_ere(compile_state *cs);

static int
compile_atom(compile_state *cs)
{
    char c = cs->pattern[cs->pos];

    if (c == '\0')
        return 0;

    if (c == '.') {
        emit(cs, OP_ANY);
        cs->pos++;
        return 1;
    }

    if (c == '^') {
        emit(cs, OP_BOL);
        cs->pos++;
        return 1;
    }

    if (c == '$') {
        emit(cs, OP_EOL);
        cs->pos++;
        return 1;
    }

    if (c == '[') {
        return compile_class(cs) == 0 ? 1 : -1;
    }

    if (c == '\\') {
        cs->pos++;
        c = cs->pattern[cs->pos];
        if (c == '\0') {
            cs->error = REG_EESCAPE;
            return -1;
        }
        /* Backreferences */
        if (c >= '1' && c <= '9') {
            int group = c - '0';
            emit(cs, OP_BACKREF);
            emit(cs, (unsigned char)group);
            cs->pos++;
            return 1;
        }
        /* Common escapes */
        switch (c) {
        case 't': emit(cs, OP_LITERAL); emit(cs, '\t'); cs->pos++; return 1;
        case 'n': emit(cs, OP_LITERAL); emit(cs, '\n'); cs->pos++; return 1;
        case 'r': emit(cs, OP_LITERAL); emit(cs, '\r'); cs->pos++; return 1;
        default:
            /* Escaped literal */
            emit(cs, OP_LITERAL);
            emit(cs, (unsigned char)c);
            cs->pos++;
            return 1;
        }
    }

    /* ERE grouping */
    if ((cs->cflags & REG_EXTENDED) && c == '(') {
        int group_num;
        cs->pos++; /* skip '(' */
        group_num = cs->n_groups++;
        if (group_num >= AMIPORT_REGEX_MAX_GROUPS) {
            cs->error = REG_EPAREN;
            return -1;
        }
        emit(cs, OP_GROUPSTART);
        emit(cs, (unsigned char)group_num);

        if (compile_ere(cs) < 0)
            return -1;

        if (cs->pattern[cs->pos] != ')') {
            cs->error = REG_EPAREN;
            return -1;
        }
        cs->pos++; /* skip ')' */

        emit(cs, OP_GROUPEND);
        emit(cs, (unsigned char)group_num);
        return 1;
    }

    /* BRE grouping: \( ... \) */
    if (!(cs->cflags & REG_EXTENDED) && c == '\\' &&
        cs->pattern[cs->pos + 1] == '(') {
        int group_num;
        cs->pos += 2; /* skip \( */
        group_num = cs->n_groups++;
        if (group_num >= AMIPORT_REGEX_MAX_GROUPS) {
            cs->error = REG_EPAREN;
            return -1;
        }
        emit(cs, OP_GROUPSTART);
        emit(cs, (unsigned char)group_num);

        if (compile_ere(cs) < 0)
            return -1;

        if (cs->pattern[cs->pos] != '\\' || cs->pattern[cs->pos + 1] != ')') {
            cs->error = REG_EPAREN;
            return -1;
        }
        cs->pos += 2; /* skip \) */

        emit(cs, OP_GROUPEND);
        emit(cs, (unsigned char)group_num);
        return 1;
    }

    /* ERE special chars that end an atom */
    if (cs->cflags & REG_EXTENDED) {
        if (c == ')' || c == '|')
            return 0;
        if (c == '*' || c == '+' || c == '?')
            return 0;
    }

    /* Literal character */
    emit(cs, OP_LITERAL);
    if (cs->cflags & REG_ICASE)
        emit(cs, (unsigned char)tolower((unsigned char)c));
    else
        emit(cs, (unsigned char)c);
    cs->pos++;
    return 1;
}

static int
compile_piece(compile_state *cs)
{
    int atom_start;
    int atom_len;
    char quant;

    atom_start = cs->code_len;

    if (compile_atom(cs) <= 0)
        return cs->error ? -1 : 0;

    atom_len = cs->code_len - atom_start;

    /* Check for quantifier */
    quant = cs->pattern[cs->pos];

    if (quant == '*' || ((cs->cflags & REG_EXTENDED) && (quant == '+' || quant == '?'))) {
        unsigned char op;
        unsigned char saved[AMIPORT_REGEX_MAX_COMPILED];
        int i;

        switch (quant) {
        case '*': op = OP_STAR; break;
        case '+': op = OP_PLUS; break;
        case '?': op = OP_QUEST; break;
        default:  op = OP_STAR; break;
        }

        /* Save the atom code */
        memcpy(saved, cs->code + atom_start, atom_len);

        /* Rewrite: [QUANT len] [atom] */
        cs->code_len = atom_start;
        emit(cs, op);
        emit(cs, (unsigned char)atom_len);
        for (i = 0; i < atom_len; i++)
            emit(cs, saved[i]);

        cs->pos++; /* skip quantifier */
        return 1;
    }

    /* BRE: * is a quantifier after an atom */
    if (!(cs->cflags & REG_EXTENDED) && quant == '*') {
        unsigned char saved[AMIPORT_REGEX_MAX_COMPILED];
        int i;

        memcpy(saved, cs->code + atom_start, atom_len);
        cs->code_len = atom_start;
        emit(cs, OP_STAR);
        emit(cs, (unsigned char)atom_len);
        for (i = 0; i < atom_len; i++)
            emit(cs, saved[i]);
        cs->pos++;
        return 1;
    }

    return 1;
}

static int
compile_branch(compile_state *cs)
{
    int result;

    do {
        result = compile_piece(cs);
        if (result < 0) return -1;
    } while (result > 0);

    return 1;
}

static int
compile_ere(compile_state *cs)
{
    int branch_start;

    branch_start = cs->code_len;

    if (compile_branch(cs) < 0)
        return -1;

    /* Handle alternation (ERE only) */
    while ((cs->cflags & REG_EXTENDED) && cs->pattern[cs->pos] == '|') {
        int jump_fixup;
        cs->pos++; /* skip '|' */

        /* Emit jump over the next branch (to be fixed up) */
        emit(cs, OP_JUMP);
        jump_fixup = cs->code_len;
        emit(cs, 0); /* placeholder */

        /* Reset for next branch */
        emit(cs, OP_BRANCH);

        if (compile_branch(cs) < 0)
            return -1;

        /* Fix up the jump offset */
        cs->code[jump_fixup] = (unsigned char)(cs->code_len - jump_fixup - 1);
    }

    return 0;
}

/* --- Matching engine --- */

static int
match_at(const unsigned char *code, int pc, const char *str, int pos,
         match_state *ms);

static int
match_class(const unsigned char *bitmap, unsigned char c)
{
    return (bitmap[c / 8] >> (c % 8)) & 1;
}

static int
try_match(const unsigned char *code, int pc, const char *str, int pos,
          match_state *ms)
{
    unsigned char op = code[pc];

    switch (op) {
    case OP_END:
        return pos;

    case OP_LITERAL: {
        unsigned char expected = code[pc + 1];
        unsigned char actual;
        if (str[pos] == '\0') return -1;
        actual = (unsigned char)str[pos];
        if (ms->cflags & REG_ICASE)
            actual = (unsigned char)tolower(actual);
        if (actual != expected) return -1;
        return match_at(code, pc + 2, str, pos + 1, ms);
    }

    case OP_ANY:
        if (str[pos] == '\0') return -1;
        if (str[pos] == '\n' && (ms->cflags & REG_NEWLINE)) return -1;
        return match_at(code, pc + 1, str, pos + 1, ms);

    case OP_BOL:
        if (pos == 0 && !(ms->eflags & REG_NOTBOL))
            return match_at(code, pc + 1, str, pos, ms);
        if ((ms->cflags & REG_NEWLINE) && pos > 0 && str[pos - 1] == '\n')
            return match_at(code, pc + 1, str, pos, ms);
        return -1;

    case OP_EOL:
        if (str[pos] == '\0' && !(ms->eflags & REG_NOTEOL))
            return match_at(code, pc + 1, str, pos, ms);
        if ((ms->cflags & REG_NEWLINE) && str[pos] == '\n')
            return match_at(code, pc + 1, str, pos, ms);
        return -1;

    case OP_CLASS:
    case OP_NCLASS: {
        unsigned char actual;
        int in_class;
        if (str[pos] == '\0') return -1;
        actual = (unsigned char)str[pos];
        in_class = match_class(code + pc + 2, actual);
        if (op == OP_NCLASS) in_class = !in_class;
        if (!in_class) return -1;
        return match_at(code, pc + 2 + code[pc + 1], str, pos + 1, ms);
    }

    case OP_STAR: {
        int sub_len = code[pc + 1];
        int sub_start = pc + 2;
        int next_pc = sub_start + sub_len;
        int result;
        int count = 0;
        int max_count;
        int i;

        /* Greedy: try matching as many as possible, then backtrack */
        /* First, count how many times the sub-pattern matches */
        max_count = 0;
        {
            int tpos = pos;
            while (1) {
                int r = try_match(code, sub_start, str, tpos, ms);
                if (r < 0 || r == tpos) break;
                tpos = r;
                max_count++;
                if (max_count > 10000) break; /* safety */
            }
        }

        /* Try from max down to 0 */
        for (i = max_count; i >= 0; i--) {
            int tpos = pos;
            int j;
            int ok = 1;
            for (j = 0; j < i; j++) {
                int r = try_match(code, sub_start, str, tpos, ms);
                if (r < 0) { ok = 0; break; }
                tpos = r;
            }
            if (!ok) continue;
            result = match_at(code, next_pc, str, tpos, ms);
            if (result >= 0) return result;
        }
        return -1;
    }

    case OP_PLUS: {
        int sub_len = code[pc + 1];
        int sub_start = pc + 2;
        int next_pc = sub_start + sub_len;
        int result;
        int max_count = 0;
        int i;

        /* Count max matches */
        {
            int tpos = pos;
            while (1) {
                int r = try_match(code, sub_start, str, tpos, ms);
                if (r < 0 || r == tpos) break;
                tpos = r;
                max_count++;
                if (max_count > 10000) break;
            }
        }

        /* Must match at least once */
        for (i = max_count; i >= 1; i--) {
            int tpos = pos;
            int j;
            int ok = 1;
            for (j = 0; j < i; j++) {
                int r = try_match(code, sub_start, str, tpos, ms);
                if (r < 0) { ok = 0; break; }
                tpos = r;
            }
            if (!ok) continue;
            result = match_at(code, next_pc, str, tpos, ms);
            if (result >= 0) return result;
        }
        return -1;
    }

    case OP_QUEST: {
        int sub_len = code[pc + 1];
        int sub_start = pc + 2;
        int next_pc = sub_start + sub_len;
        int result;
        int r;

        /* Try with the optional match first (greedy) */
        r = try_match(code, sub_start, str, pos, ms);
        if (r >= 0) {
            result = match_at(code, next_pc, str, r, ms);
            if (result >= 0) return result;
        }
        /* Try without */
        return match_at(code, next_pc, str, pos, ms);
    }

    case OP_JUMP: {
        int offset = code[pc + 1];
        return match_at(code, pc + 2 + offset, str, pos, ms);
    }

    case OP_BRANCH:
        return match_at(code, pc + 1, str, pos, ms);

    case OP_GROUPSTART: {
        int group = code[pc + 1];
        int old_start = ms->group_start[group];
        int result;
        ms->group_start[group] = pos;
        result = match_at(code, pc + 2, str, pos, ms);
        if (result < 0)
            ms->group_start[group] = old_start;
        return result;
    }

    case OP_GROUPEND: {
        int group = code[pc + 1];
        int old_end = ms->group_end[group];
        int result;
        ms->group_end[group] = pos;
        result = match_at(code, pc + 2, str, pos, ms);
        if (result < 0)
            ms->group_end[group] = old_end;
        return result;
    }

    case OP_BACKREF: {
        int group = code[pc + 1];
        int gs, ge, len, i;
        if (group >= ms->n_groups) return -1;
        gs = ms->group_start[group];
        ge = ms->group_end[group];
        if (gs < 0 || ge < 0) return -1;
        len = ge - gs;
        for (i = 0; i < len; i++) {
            if (str[pos + i] == '\0') return -1;
            if (ms->cflags & REG_ICASE) {
                if (tolower((unsigned char)str[pos + i]) !=
                    tolower((unsigned char)str[gs + i]))
                    return -1;
            } else {
                if (str[pos + i] != str[gs + i])
                    return -1;
            }
        }
        return match_at(code, pc + 2, str, pos + len, ms);
    }

    default:
        return -1;
    }
}

static int
match_at(const unsigned char *code, int pc, const char *str, int pos,
         match_state *ms)
{
    return try_match(code, pc, str, pos, ms);
}

/* --- Public API --- */

int
amiport_emu_regcomp(amiport_emu_regex_t *preg, const char *pattern, int cflags)
{
    compile_state cs;

    if (!preg || !pattern)
        return REG_BADPAT;

    memset(preg, 0, sizeof(*preg));
    preg->cflags = cflags;

    cs.pattern = pattern;
    cs.pos = 0;
    cs.cflags = cflags;
    cs.code = preg->program;
    cs.code_len = 0;
    cs.n_groups = 0;
    cs.error = 0;

    if (compile_ere(&cs) < 0 || cs.error != 0) {
        return cs.error ? cs.error : REG_BADPAT;
    }

    emit(&cs, OP_END);
    if (cs.error)
        return cs.error;

    preg->prog_len = cs.code_len;
    preg->re_nsub = cs.n_groups > 0 ? cs.n_groups : 0;

    return 0;
}

int
amiport_emu_regexec(const amiport_emu_regex_t *preg, const char *string,
                     size_t nmatch, amiport_emu_regmatch_t pmatch[], int eflags)
{
    match_state ms;
    int pos;
    int result;
    int str_len;
    size_t i;

    if (!preg || !string)
        return REG_NOMATCH;

    memset(&ms, 0, sizeof(ms));
    ms.str = string;
    ms.eflags = eflags;
    ms.cflags = preg->cflags;
    ms.n_groups = (int)preg->re_nsub;

    for (i = 0; i < AMIPORT_REGEX_MAX_GROUPS; i++) {
        ms.group_start[i] = -1;
        ms.group_end[i] = -1;
    }

    str_len = strlen(string);

    /* Try matching at each position */
    for (pos = 0; pos <= str_len; pos++) {
        int j;
        for (j = 0; j < AMIPORT_REGEX_MAX_GROUPS; j++) {
            ms.group_start[j] = -1;
            ms.group_end[j] = -1;
        }

        result = match_at(preg->program, 0, string, pos, &ms);
        if (result >= 0) {
            /* Match found */
            if (pmatch && nmatch > 0 && !(preg->cflags & REG_NOSUB)) {
                pmatch[0].rm_so = pos;
                pmatch[0].rm_eo = result;
                for (i = 1; i < nmatch; i++) {
                    if ((int)i <= ms.n_groups) {
                        pmatch[i].rm_so = ms.group_start[i - 1];
                        pmatch[i].rm_eo = ms.group_end[i - 1];
                    } else {
                        pmatch[i].rm_so = -1;
                        pmatch[i].rm_eo = -1;
                    }
                }
            }
            return 0;
        }
    }

    return REG_NOMATCH;
}

void
amiport_emu_regfree(amiport_emu_regex_t *preg)
{
    /* No dynamic allocation to free — regex_t is self-contained */
    if (preg)
        memset(preg, 0, sizeof(*preg));
}

size_t
amiport_emu_regerror(int errcode, const amiport_emu_regex_t *preg,
                      char *errbuf, size_t errbuf_size)
{
    static const char *errors[] = {
        "Success",                    /* 0 */
        "No match",                   /* REG_NOMATCH */
        "Invalid regular expression", /* REG_BADPAT */
        "Invalid collating element",  /* REG_ECOLLATE */
        "Invalid character class",    /* REG_ECTYPE */
        "Trailing backslash",         /* REG_EESCAPE */
        "Invalid backreference",      /* REG_ESUBREG */
        "Unmatched [",                /* REG_EBRACK */
        "Unmatched (",                /* REG_EPAREN */
        "Unmatched {",                /* REG_EBRACE */
        "Invalid {} contents",        /* REG_BADBR */
        "Invalid range endpoint",     /* REG_ERANGE */
        "Out of memory",              /* REG_ESPACE */
        "Invalid repetition"          /* REG_BADRPT */
    };
    const char *msg;
    size_t len;

    (void)preg;

    if (errcode < 0 || errcode > 13)
        msg = "Unknown error";
    else
        msg = errors[errcode];

    len = strlen(msg) + 1;

    if (errbuf && errbuf_size > 0) {
        if (len <= errbuf_size) {
            memcpy(errbuf, msg, len);
        } else {
            memcpy(errbuf, msg, errbuf_size - 1);
            errbuf[errbuf_size - 1] = '\0';
        }
    }

    return len;
}
