/*
 * globals.c — Console-shim global variables
 *
 * Separated from initscr.c so that programs linking only against
 * termcap functions (tgetstr, tgetnum, etc.) don't pull in initscr.o
 * and its AmigaOS library dependencies (intuition.library).
 */

#include <amiport-console/curses.h>
#include <stddef.h>

WINDOW *stdscr = NULL;
WINDOW *curscr = NULL;
int LINES = 24;
int COLS = 80;

/*
 * Classic termcap interface globals (BSD termcap compatibility).
 * Programs like tetris declare these as extern and set them from
 * tgetstr() results. They are defined here so the linker can find them.
 *
 * PC  -- pad character (set to pcstr value, or NUL if none)
 * BC  -- backspace capability string pointer
 * UP  -- cursor-up capability string pointer
 *
 * These are used by tgoto() and tputs() in traditional termcap code.
 */
char  PC = 0;     /* pad character -- set by app from tgetstr("pc",...) */
char *BC = NULL;  /* backspace string -- set by app from tgetstr("le",...) */
char *UP = NULL;  /* cursor-up string -- set by app from tgetstr("up",...) */
