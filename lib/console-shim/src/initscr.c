/*
 * initscr.c — Console initialization and cleanup for AmigaOS
 *
 * Opens raw console for character-at-a-time I/O, queries window size,
 * allocates stdscr, registers atexit cleanup.
 */

#include <amiport-console/curses.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#else
/* Host-side stubs for compilation testing */
#endif

/* Globals (stdscr, curscr, LINES, COLS) are in globals.c
 * to avoid pulling in initscr.o just for variable references */

static int console_initialized = 0;
static int original_mode = 0; /* 0 = CON:, 1 = RAW: */
static int echo_mode = 1;

/* --- Internal helpers --- */

static WINDOW *alloc_window(int nlines, int ncols, int begy, int begx)
{
    WINDOW *win;
    int bufsz;

    win = (WINDOW *)calloc(1, sizeof(WINDOW));
    if (!win) return NULL;

    win->maxy = nlines;
    win->maxx = ncols;
    win->begy = begy;
    win->begx = begx;
    win->cury = 0;
    win->curx = 0;
    win->attrs = A_NORMAL;
    win->scrollok = 0;
    win->keypad = 0;
    win->nodelay = 0;
    win->timeout_ms = -1;
    win->dirty = 1;
    win->is_sub = 0;
    win->parent = NULL;

    bufsz = nlines * ncols;
    win->buf = (chtype *)calloc(bufsz, sizeof(chtype));
    win->oldbuf = (chtype *)calloc(bufsz, sizeof(chtype));
    if (!win->buf || !win->oldbuf) {
        free(win->buf);
        free(win->oldbuf);
        free(win);
        return NULL;
    }

    return win;
}

static void free_window(WINDOW *win)
{
    if (!win) return;
    if (!win->is_sub) {
        free(win->buf);
        free(win->oldbuf);
    }
    free(win);
}

/* Query window size via ANSI escape: CSI 0 SP q (Window Status Request)
 * Fallback to 80x24 if query fails */
static void query_window_size(void)
{
#ifdef __AMIGA__
    /* Use Amiga-specific window bounds query:
     * Send CSI '0' ' ' 'q' — returns CSI '1' ';' '1' ';' rows ';' cols ' ' 'r'
     * Simpler approach: use the console's ConUnit via intuition */
    BPTR fh;
    struct InfoData *id;
    struct Window *intuiwin;

    /* Try to get window pointer from the console */
    fh = Output();
    if (fh) {
        id = (struct InfoData *)AllocMem(sizeof(struct InfoData), 0);
        if (id) {
            /* Default fallback */
            LINES = 24;
            COLS = 80;

            /* Try getting the Intuition window from the console */
            if (IsInteractive(fh)) {
                /* Use CSI query as a portable fallback */
                /* Send: ESC [ 6 n (Device Status Report)
                 * For now, use sensible defaults */
            }
            FreeMem(id, sizeof(struct InfoData));
        }
    }
#else
    LINES = 24;
    COLS = 80;
#endif
}

/* Switch console to raw mode for character-at-a-time input */
static void set_raw_mode(void)
{
#ifdef __AMIGA__
    SetMode(Input(), 1); /* 1 = RAW mode */
#endif
}

/* Restore console to cooked mode */
static void set_cooked_mode(void)
{
#ifdef __AMIGA__
    SetMode(Input(), 0); /* 0 = CON mode */
#endif
}

/* atexit handler */
static void console_atexit(void)
{
    if (console_initialized) {
        endwin();
    }
}

/* --- Public API: Initialization --- */

WINDOW *initscr(void)
{
    if (console_initialized) {
        return stdscr;
    }

    query_window_size();

    stdscr = alloc_window(LINES, COLS, 0, 0);
    if (!stdscr) return NULL;

    curscr = alloc_window(LINES, COLS, 0, 0);
    if (!curscr) {
        free_window(stdscr);
        stdscr = NULL;
        return NULL;
    }

    /* Switch to raw mode for character input */
    set_raw_mode();

    /* Save cursor, switch to alternate screen (if supported) */
    /* ESC 7 = save cursor, ESC [ ? 47 h = alternate screen */
    printf("\0337");
    printf("\033[?47h");

    /* Clear screen */
    printf("\033[2J");
    printf("\033[H");
    fflush(stdout);

    console_initialized = 1;
    echo_mode = 1;

    atexit(console_atexit);

    return stdscr;
}

int endwin(void)
{
    if (!console_initialized) return ERR;

    /* Reset attributes */
    printf("\033[0m");

    /* Restore alternate screen, restore cursor */
    printf("\033[?47l");
    printf("\0338");

    /* Show cursor */
    printf("\033[?25h");
    fflush(stdout);

    /* Restore cooked mode */
    set_cooked_mode();

    /* Free windows */
    free_window(curscr);
    curscr = NULL;
    free_window(stdscr);
    stdscr = NULL;

    console_initialized = 0;

    return OK;
}

int isendwin(void)
{
    return !console_initialized;
}

/* --- Public API: Cursor --- */

int move(int y, int x)
{
    return wmove(stdscr, y, x);
}

int wmove(WINDOW *win, int y, int x)
{
    if (!win) return ERR;
    if (y < 0 || y >= win->maxy || x < 0 || x >= win->maxx) return ERR;
    win->cury = y;
    win->curx = x;
    return OK;
}

/* --- Public API: Screen size --- */

int getmaxy(WINDOW *win)
{
    return win ? win->maxy : ERR;
}

int getmaxx(WINDOW *win)
{
    return win ? win->maxx : ERR;
}

int getyx_y(WINDOW *win)
{
    return win ? win->cury : ERR;
}

int getyx_x(WINDOW *win)
{
    return win ? win->curx : ERR;
}

/* --- Public API: Scrolling --- */

int scrollok(WINDOW *win, int bf)
{
    if (!win) return ERR;
    win->scrollok = bf;
    return OK;
}

/* --- Public API: Terminal modes --- */

int raw(void)
{
    set_raw_mode();
    return OK;
}

int noraw(void)
{
    set_cooked_mode();
    return OK;
}

int cbreak(void)
{
    return raw(); /* On Amiga, cbreak ≈ raw */
}

int nocbreak(void)
{
    return noraw();
}

int echo_on(void)
{
    echo_mode = 1;
    return OK;
}

int noecho(void)
{
    echo_mode = 0;
    return OK;
}

int nl(void)
{
    return OK; /* Default behavior */
}

int nonl(void)
{
    return OK; /* No-op on Amiga console */
}

char *longname(void)
{
    return "Amiga Console Device";
}

/* --- Public API: Window management --- */

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
    if (nlines <= 0) nlines = LINES - begin_y;
    if (ncols <= 0) ncols = COLS - begin_x;

    return alloc_window(nlines, ncols, begin_y, begin_x);
}

WINDOW *subwin(WINDOW *parent, int nlines, int ncols, int begin_y, int begin_x)
{
    WINDOW *win;

    if (!parent) return NULL;

    if (nlines <= 0) nlines = parent->maxy - (begin_y - parent->begy);
    if (ncols <= 0) ncols = parent->maxx - (begin_x - parent->begx);

    win = (WINDOW *)calloc(1, sizeof(WINDOW));
    if (!win) return NULL;

    win->maxy = nlines;
    win->maxx = ncols;
    win->begy = begin_y;
    win->begx = begin_x;
    win->attrs = parent->attrs;
    win->is_sub = 1;
    win->parent = parent;
    win->timeout_ms = -1;

    /* Subwindow shares parent's buffer (offset into it) */
    win->buf = parent->buf;
    win->oldbuf = parent->oldbuf;

    return win;
}

int delwin(WINDOW *win)
{
    if (!win) return ERR;
    if (win == stdscr || win == curscr) return ERR;
    free_window(win);
    return OK;
}

/* --- Public API: Misc --- */

int beep(void)
{
    putchar('\007');
    fflush(stdout);
    return OK;
}

int flash(void)
{
    /* Amiga: briefly invert screen — approximate with beep */
    return beep();
}
