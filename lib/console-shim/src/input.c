/*
 * input.c — Raw keyboard input for AmigaOS console
 *
 * Reads characters from RAW: console. Decodes ANSI escape sequences
 * for arrow keys, function keys, etc. when keypad mode is enabled.
 */

#include <amiport-console/curses.h>

#include <stdio.h>

#ifdef __AMIGA__
#include <proto/dos.h>
#endif

/* Ungetch buffer (single character) */
static int ungetch_buf = ERR;
static int ungetch_valid = 0;

/* --- Internal: read one raw character --- */

static int raw_getchar(int timeout_ms)
{
#ifdef __AMIGA__
    BPTR fh = Input();
    char ch;

    if (timeout_ms == 0) {
        /* Non-blocking: check if data available */
        if (!WaitForChar(fh, 0)) return ERR;
    } else if (timeout_ms > 0) {
        /* Timed wait: WaitForChar takes microseconds */
        if (!WaitForChar(fh, (long)timeout_ms * 1000)) return ERR;
    }
    /* else timeout_ms < 0: blocking read */

    if (Read(fh, &ch, 1) == 1) {
        return (int)(unsigned char)ch;
    }
    return ERR;
#else
    /* Host stub */
    (void)timeout_ms;
    return getchar();
#endif
}

/* --- Internal: decode ANSI escape sequence --- */

static int decode_escape(int timeout_ms)
{
    int ch;

    ch = raw_getchar(100); /* Brief timeout for escape sequence chars */
    if (ch == ERR) return 27; /* Just an ESC key */

    if (ch == '[') {
        /* CSI sequence */
        ch = raw_getchar(100);
        switch (ch) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        case 'H': return KEY_HOME;
        case 'F': return KEY_END;
        case '1':
            ch = raw_getchar(100);
            if (ch == '~') return KEY_HOME;
            /* Function keys: ESC [ 1 1 ~ = F1, etc. */
            if (ch >= '1' && ch <= '9') {
                int fkey = ch - '0';
                ch = raw_getchar(100); /* consume ~ */
                return KEY_F(fkey);
            }
            return ERR;
        case '2':
            ch = raw_getchar(100);
            if (ch == '~') return KEY_IC;
            /* F9-F12: ESC [ 2 0 ~ through ESC [ 2 4 ~ */
            if (ch >= '0' && ch <= '4') {
                raw_getchar(100); /* consume ~ */
                return KEY_F(ch - '0' + 9);
            }
            return ERR;
        case '3':
            ch = raw_getchar(100);
            if (ch == '~') return KEY_DC;
            return ERR;
        case '4':
            ch = raw_getchar(100);
            if (ch == '~') return KEY_END;
            return ERR;
        case '5':
            ch = raw_getchar(100);
            if (ch == '~') return KEY_PPAGE;
            return ERR;
        case '6':
            ch = raw_getchar(100);
            if (ch == '~') return KEY_NPAGE;
            return ERR;
        default:
            return ERR;
        }
    } else if (ch == 'O') {
        /* SS3 sequence (alternate function keys) */
        ch = raw_getchar(100);
        switch (ch) {
        case 'P': return KEY_F(1);
        case 'Q': return KEY_F(2);
        case 'R': return KEY_F(3);
        case 'S': return KEY_F(4);
        default:  return ERR;
        }
    }

    return ERR;
}

/* --- Public API --- */

int wgetch(WINDOW *win)
{
    int ch;
    int timeout_ms;

    if (!win) return ERR;

    /* Check ungetch buffer first */
    if (ungetch_valid) {
        ungetch_valid = 0;
        return ungetch_buf;
    }

    timeout_ms = win->timeout_ms;

    ch = raw_getchar(timeout_ms);
    if (ch == ERR) return ERR;

    /* Decode escape sequences if keypad mode is on */
    if (ch == 27 && win->keypad) {
        return decode_escape(timeout_ms);
    }

    /* Map common control characters */
    if (ch == 127 || ch == 8) return KEY_BACKSPACE;
    if (ch == 13)  return '\n';

    return ch;
}

int getch(void)
{
    return wgetch(stdscr);
}

int ungetch(int ch)
{
    if (ungetch_valid) return ERR;
    ungetch_buf = ch;
    ungetch_valid = 1;
    return OK;
}

int nodelay_w(WINDOW *win, int bf)
{
    if (!win) return ERR;
    win->nodelay = bf;
    win->timeout_ms = bf ? 0 : -1;
    return OK;
}

int timeout_w(WINDOW *win, int delay)
{
    if (!win) return ERR;
    win->timeout_ms = delay;
    win->nodelay = (delay == 0);
    return OK;
}

int keypad_w(WINDOW *win, int bf)
{
    if (!win) return ERR;
    win->keypad = bf;
    return OK;
}

/* --- Cursor visibility --- */

int curs_set(int visibility)
{
    switch (visibility) {
    case 0:
        printf("\033[?25l"); /* Hide cursor */
        break;
    case 1:
        printf("\033[?25h"); /* Normal cursor */
        break;
    case 2:
        printf("\033[?25h"); /* Very visible = normal on Amiga */
        break;
    default:
        return ERR;
    }
    fflush(stdout);
    return OK;
}
