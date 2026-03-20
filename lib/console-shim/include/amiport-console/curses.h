/*
 * amiport-console/curses.h — Minimal ncurses API for AmigaOS console.device
 *
 * Drop-in replacement for <curses.h> / <ncurses.h>.
 * Implements a subset of ncurses using ANSI escape sequences sent to
 * the Amiga console.device, which is VT100-compatible.
 *
 * See ADR-009 for supported API surface and limitations.
 *
 * Tier 1 (direct ANSI mapping): initscr, endwin, move, addch, addstr,
 *   printw, clear, erase, clrtoeol, clrtobot, refresh, attron, attroff,
 *   attrset, start_color, init_pair, COLOR_PAIR, scrollok
 *
 * Tier 2 (emulated with caveats): getch, ungetch, nodelay, timeout,
 *   keypad, curs_set, newwin, subwin, wborder, box
 *
 * Not supported: mouse, wide chars, panels, menus, forms, pads, terminfo
 */

#ifndef AMIPORT_CONSOLE_CURSES_H
#define AMIPORT_CONSOLE_CURSES_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Types --- */

typedef unsigned long chtype;
typedef unsigned long attr_t;

/* Window structure */
typedef struct _amiport_win {
    int  cury, curx;       /* Current cursor position */
    int  maxy, maxx;       /* Window dimensions */
    int  begy, begx;       /* Window origin on screen */
    attr_t attrs;          /* Current attributes */
    int  scrollok;         /* Scrolling enabled */
    int  keypad;           /* Keypad mode enabled */
    int  nodelay;          /* Non-blocking input */
    int  timeout_ms;       /* Input timeout (-1 = blocking) */
    /* Internal buffer for refresh optimization */
    chtype *buf;           /* Character buffer [maxy * maxx] */
    chtype *oldbuf;        /* Previous frame for diff */
    int  dirty;            /* Needs refresh */
    int  is_sub;           /* Is a subwindow */
    struct _amiport_win *parent; /* Parent window (for subwin) */
} WINDOW;

/* The standard screen */
extern WINDOW *stdscr;
extern WINDOW *curscr;

/* Screen dimensions */
extern int LINES;
extern int COLS;

/* --- Attributes --- */

#define A_NORMAL     0x00000000UL
#define A_STANDOUT   0x00010000UL
#define A_UNDERLINE  0x00020000UL
#define A_REVERSE    0x00040000UL
#define A_BLINK      0x00080000UL
#define A_DIM        0x00100000UL
#define A_BOLD       0x00200000UL
#define A_INVIS      0x00400000UL

/* Attribute manipulation */
#define A_CHARTEXT   0x000000FFUL
#define A_COLOR      0x0000FF00UL

/* --- Colors --- */

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

#define COLOR_PAIRS   64
#define COLORS        8

#define COLOR_PAIR(n) (((n) & 0xFF) << 8)
#define PAIR_NUMBER(a) (((a) & A_COLOR) >> 8)

/* --- Special keys --- */

#define KEY_DOWN      0x102
#define KEY_UP        0x103
#define KEY_LEFT      0x104
#define KEY_RIGHT     0x105
#define KEY_HOME      0x106
#define KEY_BACKSPACE 0x107
#define KEY_F0        0x108
#define KEY_F(n)      (KEY_F0 + (n))
#define KEY_DC        0x14A  /* Delete */
#define KEY_IC        0x14B  /* Insert */
#define KEY_NPAGE     0x152  /* Page Down */
#define KEY_PPAGE     0x153  /* Page Up */
#define KEY_END       0x166
#define KEY_ENTER     0x157
#define KEY_RESIZE    0x19A

#define ERR           (-1)
#define OK            0
#define TRUE          1
#define FALSE         0

/* --- Line drawing characters --- */
/* Use ASCII fallbacks since Amiga fonts vary */

#define ACS_ULCORNER  ((chtype)'+')
#define ACS_LLCORNER  ((chtype)'+')
#define ACS_URCORNER  ((chtype)'+')
#define ACS_LRCORNER  ((chtype)'+')
#define ACS_LTEE      ((chtype)'+')
#define ACS_RTEE      ((chtype)'+')
#define ACS_BTEE      ((chtype)'+')
#define ACS_TTEE      ((chtype)'+')
#define ACS_HLINE     ((chtype)'-')
#define ACS_VLINE     ((chtype)'|')
#define ACS_PLUS      ((chtype)'+')

/* ================================================================
 * Tier 1 — Direct ANSI mapping
 * ================================================================ */

/* Initialization and cleanup */
WINDOW *initscr(void);
int     endwin(void);
int     isendwin(void);

/* Output */
int addch(chtype ch);
int addstr(const char *str);
int addnstr(const char *str, int n);
int printw(const char *fmt, ...);
int mvaddch(int y, int x, chtype ch);
int mvaddstr(int y, int x, const char *str);
int mvprintw(int y, int x, const char *fmt, ...);

/* Cursor */
int move(int y, int x);

/* Clearing */
int clear(void);
int erase(void);
int clrtoeol(void);
int clrtobot(void);

/* Refresh */
int refresh(void);

/* Attributes */
int attron(attr_t attrs);
int attroff(attr_t attrs);
int attrset(attr_t attrs);

/* Colors */
int  start_color(void);
int  has_colors(void);
int  init_pair(short pair, short f, short b);

/* Scrolling */
int scrollok(WINDOW *win, int bf);

/* Screen size */
int getmaxy(WINDOW *win);
int getmaxx(WINDOW *win);
int getyx_y(WINDOW *win);
int getyx_x(WINDOW *win);

/* Macro forms */
#define getyx(win, y, x) do { (y) = getyx_y(win); (x) = getyx_x(win); } while(0)
#define getmaxyx(win, y, x) do { (y) = getmaxy(win); (x) = getmaxx(win); } while(0)
#define getbegyx(win, y, x) do { (y) = (win)->begy; (x) = (win)->begx; } while(0)

/* ================================================================
 * Tier 2 — Emulated with caveats
 * ================================================================ */

/* Input */
int getch(void);
int ungetch(int ch);
int nodelay_w(WINDOW *win, int bf);
int timeout_w(WINDOW *win, int delay);
int keypad_w(WINDOW *win, int bf);

/* Macros for ncurses-compatible nodelay/timeout/keypad signatures */
#define nodelay(win, bf) nodelay_w((win), (bf))
#define timeout(delay) timeout_w(stdscr, (delay))
#define keypad(win, bf) keypad_w((win), (bf))

/* Cursor visibility */
int curs_set(int visibility);

/* Window management */
WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x);
WINDOW *subwin(WINDOW *parent, int nlines, int ncols, int begin_y, int begin_x);
int     delwin(WINDOW *win);

/* Window-specific output */
int waddch(WINDOW *win, chtype ch);
int waddstr(WINDOW *win, const char *str);
int wprintw(WINDOW *win, const char *fmt, ...);
int wmove(WINDOW *win, int y, int x);
int wrefresh(WINDOW *win);
int wclear(WINDOW *win);
int werase(WINDOW *win);
int wclrtoeol(WINDOW *win);
int wattron(WINDOW *win, attr_t attrs);
int wattroff(WINDOW *win, attr_t attrs);
int wattrset(WINDOW *win, attr_t attrs);
int wgetch(WINDOW *win);
int mvwaddch(WINDOW *win, int y, int x, chtype ch);
int mvwaddstr(WINDOW *win, int y, int x, const char *str);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);

/* Borders and boxes */
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs,
            chtype tl, chtype tr, chtype bl, chtype br);
int box(WINDOW *win, chtype verch, chtype horch);

/* Misc */
int  beep(void);
int  flash(void);
int  raw(void);
int  noraw(void);
int  cbreak(void);
int  nocbreak(void);
int  echo_on(void);
int  noecho(void);
int  nl(void);
int  nonl(void);
char *longname(void);

/* ncurses-compatible echo/noecho (avoid name clash with AmigaOS) */
#ifndef AMIPORT_NO_CURSES_MACROS
#define echo()  echo_on()
#endif

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_CONSOLE_CURSES_H */
