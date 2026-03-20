/*
 * output.c — Character and string output via ANSI escape sequences
 *
 * All output goes through the window buffer and is flushed on refresh().
 * Cursor positioning uses CSI y;x H sequences.
 */

#include <amiport-console/curses.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* --- Internal: emit ANSI sequences for current attributes --- */

static attr_t last_emitted_attrs = A_NORMAL;
static short  last_emitted_pair = 0;

/* Color pair storage — defined in color.c, shared via extern */
struct amiport_color_pair { short fg, bg; };
extern struct amiport_color_pair amiport_color_pairs[COLOR_PAIRS];

static void emit_attrs(attr_t attrs)
{
    short pair;

    if (attrs == last_emitted_attrs) return;

    /* Reset first, then apply */
    printf("\033[0");

    if (attrs & A_BOLD)      printf(";1");
    if (attrs & A_DIM)       printf(";2");
    if (attrs & A_UNDERLINE) printf(";4");
    if (attrs & A_BLINK)     printf(";5");
    if (attrs & A_REVERSE)   printf(";7");
    if (attrs & A_INVIS)     printf(";8");

    /* Color pair */
    pair = (short)PAIR_NUMBER(attrs);
    if (pair > 0 && pair < COLOR_PAIRS) {
        if (amiport_color_pairs[pair].fg >= 0)
            printf(";%d", 30 + amiport_color_pairs[pair].fg);
        if (amiport_color_pairs[pair].bg >= 0)
            printf(";%d", 40 + amiport_color_pairs[pair].bg);
    }

    printf("m");
    last_emitted_attrs = attrs;
}

/* --- Internal: put character at position in buffer --- */

static void buf_putch(WINDOW *win, int y, int x, chtype ch)
{
    if (!win || !win->buf) return;
    if (y < 0 || y >= win->maxy || x < 0 || x >= win->maxx) return;
    win->buf[y * win->maxx + x] = ch | (win->attrs & ~A_CHARTEXT);
    win->dirty = 1;
}

/* --- Public API: addch variants --- */

int waddch(WINDOW *win, chtype ch)
{
    if (!win) return ERR;

    buf_putch(win, win->cury, win->curx, ch);

    /* Advance cursor */
    win->curx++;
    if (win->curx >= win->maxx) {
        win->curx = 0;
        win->cury++;
        if (win->cury >= win->maxy) {
            if (win->scrollok) {
                win->cury = win->maxy - 1;
                /* Scroll would happen at refresh time */
            } else {
                win->cury = win->maxy - 1;
                win->curx = win->maxx - 1;
            }
        }
    }

    return OK;
}

int addch(chtype ch)
{
    return waddch(stdscr, ch);
}

int mvaddch(int y, int x, chtype ch)
{
    if (wmove(stdscr, y, x) == ERR) return ERR;
    return waddch(stdscr, ch);
}

int mvwaddch(WINDOW *win, int y, int x, chtype ch)
{
    if (wmove(win, y, x) == ERR) return ERR;
    return waddch(win, ch);
}

/* --- Public API: addstr variants --- */

int waddstr(WINDOW *win, const char *str)
{
    if (!win || !str) return ERR;
    while (*str) {
        if (waddch(win, (chtype)(unsigned char)*str) == ERR) return ERR;
        str++;
    }
    return OK;
}

int addstr(const char *str)
{
    return waddstr(stdscr, str);
}

int addnstr(const char *str, int n)
{
    int i;
    if (!str) return ERR;
    for (i = 0; i < n && str[i]; i++) {
        if (waddch(stdscr, (chtype)(unsigned char)str[i]) == ERR) return ERR;
    }
    return OK;
}

int mvaddstr(int y, int x, const char *str)
{
    if (wmove(stdscr, y, x) == ERR) return ERR;
    return waddstr(stdscr, str);
}

int mvwaddstr(WINDOW *win, int y, int x, const char *str)
{
    if (wmove(win, y, x) == ERR) return ERR;
    return waddstr(win, str);
}

/* --- Public API: printw variants --- */

int wprintw(WINDOW *win, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return waddstr(win, buf);
}

int printw(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return waddstr(stdscr, buf);
}

int mvprintw(int y, int x, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    if (wmove(stdscr, y, x) == ERR) return ERR;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return waddstr(stdscr, buf);
}

int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    if (wmove(win, y, x) == ERR) return ERR;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return waddstr(win, buf);
}

/* --- Public API: Clearing --- */

int werase(WINDOW *win)
{
    int i, total;
    if (!win) return ERR;
    total = win->maxy * win->maxx;
    for (i = 0; i < total; i++) {
        win->buf[i] = ' ';
    }
    win->cury = 0;
    win->curx = 0;
    win->dirty = 1;
    return OK;
}

int wclear(WINDOW *win)
{
    werase(win);
    /* Also request a full repaint on next refresh */
    if (win && win->oldbuf) {
        memset(win->oldbuf, 0xFF, win->maxy * win->maxx * sizeof(chtype));
    }
    return OK;
}

int clear(void)
{
    return wclear(stdscr);
}

int erase(void)
{
    return werase(stdscr);
}

int wclrtoeol(WINDOW *win)
{
    int x;
    if (!win) return ERR;
    for (x = win->curx; x < win->maxx; x++) {
        win->buf[win->cury * win->maxx + x] = ' ';
    }
    win->dirty = 1;
    return OK;
}

int clrtoeol(void)
{
    return wclrtoeol(stdscr);
}

int clrtobot(void)
{
    int y;
    if (!stdscr) return ERR;
    clrtoeol();
    for (y = stdscr->cury + 1; y < stdscr->maxy; y++) {
        int x;
        for (x = 0; x < stdscr->maxx; x++) {
            stdscr->buf[y * stdscr->maxx + x] = ' ';
        }
    }
    stdscr->dirty = 1;
    return OK;
}

/* --- Public API: Refresh --- */

int wrefresh(WINDOW *win)
{
    int y, x, offy, offx;

    if (!win) return ERR;

    offy = win->begy;
    offx = win->begx;

    /* Emit changes: compare buf vs oldbuf, output diffs */
    for (y = 0; y < win->maxy; y++) {
        for (x = 0; x < win->maxx; x++) {
            int idx = y * win->maxx + x;
            chtype ch = win->buf[idx];
            chtype old = win->oldbuf[idx];

            if (ch != old) {
                /* Position cursor */
                printf("\033[%d;%dH", offy + y + 1, offx + x + 1);
                /* Set attributes */
                emit_attrs(ch & ~A_CHARTEXT);
                /* Output character */
                if ((ch & A_CHARTEXT) == 0) {
                    putchar(' ');
                } else {
                    putchar((int)(ch & A_CHARTEXT));
                }
                win->oldbuf[idx] = ch;
            }
        }
    }

    /* Position cursor at current location */
    printf("\033[%d;%dH", offy + win->cury + 1, offx + win->curx + 1);
    fflush(stdout);

    win->dirty = 0;
    return OK;
}

int refresh(void)
{
    return wrefresh(stdscr);
}

/* --- Public API: Borders --- */

int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br)
{
    int y;

    if (!win) return ERR;

    if (ls == 0) ls = ACS_VLINE;
    if (rs == 0) rs = ACS_VLINE;
    if (ts == 0) ts = ACS_HLINE;
    if (bs == 0) bs = ACS_HLINE;
    if (tl == 0) tl = ACS_ULCORNER;
    if (tr == 0) tr = ACS_URCORNER;
    if (bl == 0) bl = ACS_LLCORNER;
    if (br == 0) br = ACS_LRCORNER;

    /* Corners */
    buf_putch(win, 0, 0, tl);
    buf_putch(win, 0, win->maxx - 1, tr);
    buf_putch(win, win->maxy - 1, 0, bl);
    buf_putch(win, win->maxy - 1, win->maxx - 1, br);

    /* Top and bottom */
    {
        int x;
        for (x = 1; x < win->maxx - 1; x++) {
            buf_putch(win, 0, x, ts);
            buf_putch(win, win->maxy - 1, x, bs);
        }
    }

    /* Left and right */
    for (y = 1; y < win->maxy - 1; y++) {
        buf_putch(win, y, 0, ls);
        buf_putch(win, y, win->maxx - 1, rs);
    }

    win->dirty = 1;
    return OK;
}

int box(WINDOW *win, chtype verch, chtype horch)
{
    return wborder(win, verch, verch, horch, horch, 0, 0, 0, 0);
}
