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
