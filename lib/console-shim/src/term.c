/*
 * term.c — Terminal capability API for AmigaOS
 *
 * Provides terminfo AND termcap APIs. Returns fixed values since
 * Amiga console.device has known capabilities.
 *
 * Terminfo API: setupterm, tigetnum, tigetflag, tigetstr (long names)
 * Termcap API:  tgetent, tgetnum, tgetflag, tgetstr, tgoto (2-letter codes)
 * Common:       tputs, tparm (shared by both APIs)
 */

#include <amiport-console/term.h>
#include <amiport-console/curses.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int setupterm(const char *term, int fildes, int *errret)
{
    (void)term;
    (void)fildes;

    /* Always succeed — we know our terminal */
    if (errret) *errret = 1;
    return OK;
}

int tigetnum(const char *capname)
{
    if (!capname) return -2;

    if (strcmp(capname, "cols") == 0) return COLS;
    if (strcmp(capname, "lines") == 0) return LINES;
    if (strcmp(capname, "colors") == 0) return 8;
    if (strcmp(capname, "pairs") == 0) return COLOR_PAIRS;

    return -1; /* Capability not found */
}

int tigetflag(const char *capname)
{
    if (!capname) return -1;

    if (strcmp(capname, "am") == 0)   return 1; /* Auto margins */
    if (strcmp(capname, "bce") == 0)  return 0; /* No back color erase */
    if (strcmp(capname, "hs") == 0)   return 0; /* No status line */
    if (strcmp(capname, "mir") == 0)  return 1; /* Move in insert mode ok */
    if (strcmp(capname, "msgr") == 0) return 1; /* Move in standout mode ok */

    return 0; /* Unknown = false */
}

char *tigetstr(const char *capname)
{
    static char buf[64];

    if (!capname) return (char *)-1;

    /* Cursor movement */
    if (strcmp(capname, "cup") == 0)   return "\033[%i%p1%d;%p2%dH";
    if (strcmp(capname, "cuu1") == 0)  return "\033[A";
    if (strcmp(capname, "cud1") == 0)  return "\033[B";
    if (strcmp(capname, "cuf1") == 0)  return "\033[C";
    if (strcmp(capname, "cub1") == 0)  return "\033[D";
    if (strcmp(capname, "home") == 0)  return "\033[H";

    /* Clearing */
    if (strcmp(capname, "clear") == 0) return "\033[2J\033[H";
    if (strcmp(capname, "el") == 0)    return "\033[K";
    if (strcmp(capname, "ed") == 0)    return "\033[J";

    /* Attributes */
    if (strcmp(capname, "bold") == 0)  return "\033[1m";
    if (strcmp(capname, "sgr0") == 0)  return "\033[0m";
    if (strcmp(capname, "smul") == 0)  return "\033[4m";
    if (strcmp(capname, "rmul") == 0)  return "\033[24m";
    if (strcmp(capname, "rev") == 0)   return "\033[7m";

    /* Cursor visibility */
    if (strcmp(capname, "civis") == 0) return "\033[?25l";
    if (strcmp(capname, "cnorm") == 0) return "\033[?25h";

    /* Color */
    if (strcmp(capname, "setaf") == 0) {
        snprintf(buf, sizeof(buf), "\033[3%%p1%%dm");
        return buf;
    }
    if (strcmp(capname, "setab") == 0) {
        snprintf(buf, sizeof(buf), "\033[4%%p1%%dm");
        return buf;
    }

    return (char *)-1; /* Not found */
}

int tputs(const char *str, int affcnt, int (*putc_func)(int))
{
    (void)affcnt;

    if (!str || str == (char *)-1) return ERR;

    while (*str) {
        if (putc_func) {
            putc_func((int)(unsigned char)*str);
        } else {
            putchar((int)(unsigned char)*str);
        }
        str++;
    }
    return OK;
}

char *tparm(const char *str, ...)
{
    /* Full tparm — process terminfo % parameters.
     * Supports: %d (decimal), %i (increment both params),
     * %p1/%p2 (select param), %% (literal %), %c (char) */
    static char result[256];
    char *out = result;
    char *end = result + sizeof(result) - 1;
    va_list ap;
    long params[9];
    int cur_param = 0;
    int i;

    if (!str || str == (char *)-1) return "";

    va_start(ap, str);
    for (i = 0; i < 9; i++)
        params[i] = va_arg(ap, long);
    va_end(ap);

    while (*str && out < end) {
        if (*str != '%') {
            *out++ = *str++;
            continue;
        }
        str++; /* skip % */
        switch (*str) {
        case 'd':
            out += snprintf(out, (size_t)(end - out), "%ld", params[cur_param]);
            str++;
            break;
        case 'c':
            *out++ = (char)params[cur_param];
            str++;
            break;
        case 'i':
            params[0]++;
            params[1]++;
            str++;
            break;
        case 'p':
            str++;
            if (*str >= '1' && *str <= '9') {
                cur_param = *str - '1';
                str++;
            }
            break;
        case '%':
            *out++ = '%';
            str++;
            break;
        case '\0':
            break;
        default:
            /* Unknown % sequence — copy literally */
            *out++ = '%';
            *out++ = *str++;
            break;
        }
    }
    *out = '\0';
    return result;
}

/* ================================================================
 * Termcap API — 2-letter capability codes
 * ================================================================ */

int tgetent(char *bp, const char *name)
{
    (void)bp;
    (void)name;
    /* Always succeed — Amiga console.device has fixed capabilities */
    return 1;
}

int tgetnum(const char *cap)
{
    if (!cap) return -1;

    if (cap[0] == 'c' && cap[1] == 'o') return COLS;   /* columns */
    if (cap[0] == 'l' && cap[1] == 'i') return LINES;  /* lines */
    if (cap[0] == 's' && cap[1] == 'g') return 0;      /* standout glitch width */

    return -1; /* Not found */
}

int tgetflag(const char *cap)
{
    if (!cap) return 0;

    if (cap[0] == 'a' && cap[1] == 'm') return 1; /* auto margins */
    if (cap[0] == 'x' && cap[1] == 'n') return 0; /* newline glitch */
    if (cap[0] == 'b' && cap[1] == 's') return 1; /* backspace works */
    if (cap[0] == 'p' && cap[1] == 't') return 0; /* no hardware tabs */
    if (cap[0] == 'x' && cap[1] == 's') return 0; /* no standout glitch */
    if (cap[0] == 'm' && cap[1] == 's') return 1; /* move in standout ok */

    return 0; /* Unknown = false */
}

char *tgetstr(const char *cap, char **area)
{
    const char *val = NULL;

    if (!cap) return NULL;

    /* Cursor motion — parameterized, used with tgoto() */
    if (cap[0] == 'c' && cap[1] == 'm') val = "\033[%i%p1%d;%p2%dH";

    /* Clearing */
    else if (cap[0] == 'c' && cap[1] == 'e') val = "\033[K";       /* clear to EOL */
    else if (cap[0] == 'c' && cap[1] == 'l') val = "\033[2J\033[H"; /* clear screen */
    else if (cap[0] == 'c' && cap[1] == 'd') val = "\033[J";       /* clear to EOS */

    /* Init/deinit terminal */
    else if (cap[0] == 't' && cap[1] == 'i') val = "\0337\033[?47h";
    else if (cap[0] == 't' && cap[1] == 'e') val = "\033[?47l\0338";

    /* Standout (reverse video) */
    else if (cap[0] == 's' && cap[1] == 'o') val = "\033[7m";
    else if (cap[0] == 's' && cap[1] == 'e') val = "\033[27m";

    /* Underline */
    else if (cap[0] == 'u' && cap[1] == 's') val = "\033[4m";
    else if (cap[0] == 'u' && cap[1] == 'e') val = "\033[24m";

    /* Bold / all attrs off */
    else if (cap[0] == 'm' && cap[1] == 'd') val = "\033[1m";
    else if (cap[0] == 'm' && cap[1] == 'e') val = "\033[0m";

    /* Home, carriage return */
    else if (cap[0] == 'h' && cap[1] == 'o') val = "\033[H";
    else if (cap[0] == 'c' && cap[1] == 'r') val = "\r";

    /* Scroll forward/reverse */
    else if (cap[0] == 's' && cap[1] == 'f') val = "\n";
    else if (cap[0] == 's' && cap[1] == 'r') val = "\033M";

    /* Insert/delete line */
    else if (cap[0] == 'a' && cap[1] == 'l') val = "\033[L";
    else if (cap[0] == 'd' && cap[1] == 'l') val = "\033[M";

    /* Cursor movement (single-char) */
    else if (cap[0] == 'u' && cap[1] == 'p') val = "\033[A";  /* cursor up */
    else if (cap[0] == 'd' && cap[1] == 'o') val = "\033[B";  /* cursor down */
    else if (cap[0] == 'n' && cap[1] == 'd') val = "\033[C";  /* cursor right */
    else if (cap[0] == 'l' && cap[1] == 'e') val = "\b";      /* cursor left (backspace) */
    else if (cap[0] == 'b' && cap[1] == 'c') val = "\b";      /* backspace char */

    /* Cursor visibility */
    else if (cap[0] == 'v' && cap[1] == 'i') val = "\033[?25l"; /* invisible */
    else if (cap[0] == 'v' && cap[1] == 'e') val = "\033[?25h"; /* normal */
    else if (cap[0] == 'v' && cap[1] == 's') val = "\033[?25h"; /* very visible = normal */

    /* Padding character */
    else if (cap[0] == 'p' && cap[1] == 'c') val = "";

    /* Bell */
    else if (cap[0] == 'b' && cap[1] == 'l') val = "\007";

    if (!val) return NULL;

    /* If area pointer provided, copy string there and advance */
    if (area && *area) {
        char *p = *area;
        strcpy(p, val);
        *area = p + strlen(val) + 1;
        return p;
    }

    /* Otherwise return pointer to the string literal */
    return (char *)val;
}

char *tgoto(const char *cm, int col, int row)
{
    /* Format cursor motion: substitute col and row into cm string.
     * The cm string uses terminfo % parameters internally. */
    static char result[32];

    if (!cm) return "";

    /* Use tparm to process the cm string with row, col as parameters.
     * Note: termcap tgoto takes (cm, col, row) but the cm string
     * expects (row, col) as %p1, %p2. We pass row first. */
    snprintf(result, sizeof(result), "\033[%d;%dH", row + 1, col + 1);
    return result;
}
