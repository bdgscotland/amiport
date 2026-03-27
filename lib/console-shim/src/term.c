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

#ifdef __AMIGA__
#include <proto/dos.h>
#endif

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

    /* Cursor visibility — AmigaOS console.device SET CURSOR RENDITION
     * VT220 uses ESC[?25l/h but AmigaOS uses ESC[0 p (off) / ESC[ p (on).
     * See ADCD: "4 / Set Graphic Rendition Implementation Notes" */
    if (strcmp(capname, "civis") == 0) return "\033[0 p";
    if (strcmp(capname, "cnorm") == 0) return "\033[ p";

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

    /*
     * amiport: AmigaOS console.device has no hardware padding (PC=0).
     * Short-circuit the per-character callback with fputs() when possible.
     * This reduces ~500 function calls per frame to a handful of fputs()
     * calls — 10-30x speedup on the escape sequence output path.
     */
    if (putc_func == NULL) {
        fputs(str, stdout);
        return OK;
    }

    while (*str)
        putc_func((int)(unsigned char)*str++);
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

#ifdef __AMIGA__
    /*
     * Query actual console window size via CSI Window Status Request.
     * Send: 0x9B '0' ' ' 'q'
     * Response: 0x9B '1' ';' '1' ';' rows ';' cols ' ' 'r'
     * Updates COLS/LINES globals so tgetnum("co"/"li") returns real values.
     * Falls back to 80x24 if query fails (e.g., on vamos).
     *
     * TODO: This only works when Input() is interactive (normal shell).
     * Programs launched via Execute script (ITEST harness) have non-interactive
     * Input() and get 80x24 defaults. Fixing this requires a deeper
     * investigation of the console handle lifecycle. See known-pitfalls.md
     * "Console-Shim tgetnum() Returns Hardcoded 80x24".
     */
    {
        BPTR fh = Input();
        if (fh && IsInteractive(fh)) {
            char buf[64];
            LONG len;
            int i, field;
            int values[4];

            buf[0] = (char)0x9B;
            buf[1] = '0';
            buf[2] = ' ';
            buf[3] = 'q';
            Write(fh, buf, 4);

            if (WaitForChar(fh, 200000)) {
                len = Read(fh, buf, sizeof(buf) - 1);
                if (len > 0) {
                    buf[len] = '\0';
                    i = 0;
                    if ((unsigned char)buf[0] == 0x9B) i = 1;
                    field = 0;
                    values[0] = values[1] = values[2] = values[3] = 0;
                    while (i < len && field < 4) {
                        if (buf[i] >= '0' && buf[i] <= '9') {
                            values[field] = values[field] * 10 + (buf[i] - '0');
                        } else if (buf[i] == ';') {
                            field++;
                        } else {
                            break;
                        }
                        i++;
                    }
                    /* fields: [0]=status, [1]=top, [2]=rows, [3]=cols */
                    if (field >= 3 && values[2] > 0 && values[3] > 0) {
                        LINES = values[2];
                        COLS = values[3];
                    }
                }
            }
        }
    }
#endif

    /* Always succeed -- Amiga console.device has known capabilities */
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

    /* Init/deinit terminal — Amiga console.device does not support
     * alternate screen buffers (?47h/?47l). Use clear screen instead. */
    else if (cap[0] == 't' && cap[1] == 'i') val = "\033[2J\033[H";
    else if (cap[0] == 't' && cap[1] == 'e') val = "\033[2J\033[H";

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

    /* Cursor visibility — AmigaOS SET CURSOR RENDITION (not VT220 ?25h/l) */
    else if (cap[0] == 'v' && cap[1] == 'i') val = "\033[0 p"; /* invisible */
    else if (cap[0] == 'v' && cap[1] == 'e') val = "\033[ p";  /* normal (visible) */
    else if (cap[0] == 'v' && cap[1] == 's') val = "\033[ p";  /* very visible = normal */

    /* Padding character */
    else if (cap[0] == 'p' && cap[1] == 'c') val = "";

    /* Key sequences */
    else if (cap[0] == 'k' && cap[1] == 'b') val = "\010"; /* key_backspace = ^H (0x08) */
    else if (cap[0] == 'k' && cap[1] == 'u') val = "\033[A"; /* key_up */
    else if (cap[0] == 'k' && cap[1] == 'd') val = "\033[B"; /* key_down */
    else if (cap[0] == 'k' && cap[1] == 'r') val = "\033[C"; /* key_right */
    else if (cap[0] == 'k' && cap[1] == 'l') val = "\033[D"; /* key_left */

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
    /*
     * amiport: fast tgoto() — direct digit emission instead of snprintf().
     * On 68000, snprintf() format parsing costs ~300-500 cycles per call.
     * This is called per changed cell in scr_update() (~20 times/frame).
     * Direct emission saves ~5000-8000 cycles per frame.
     */
    static char result[16];
    char *p = result;
    int v;

    (void)cm;   /* always ESC [ row ; col H on AmigaOS console.device */

    *p++ = '\033'; *p++ = '[';
    /* row+1 (1-based) */
    v = row + 1;
    if (v >= 100) { *p++ = '0' + (v / 100); v %= 100; }
    if (v >= 10)  *p++ = '0' + (v / 10);
    *p++ = '0' + (v % 10);
    *p++ = ';';
    /* col+1 (1-based) */
    v = col + 1;
    if (v >= 100) { *p++ = '0' + (v / 100); v %= 100; }
    if (v >= 10)  *p++ = '0' + (v / 10);
    *p++ = '0' + (v % 10);
    *p++ = 'H'; *p = '\0';
    return result;
}
