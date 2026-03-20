/*
 * attr.c — Attribute handling for AmigaOS console
 *
 * Manages the current attribute state. Actual ANSI sequence emission
 * happens in output.c during refresh.
 */

#include <amiport-console/curses.h>

int wattron(WINDOW *win, attr_t attrs)
{
    if (!win) return ERR;
    win->attrs |= attrs;
    return OK;
}

int wattroff(WINDOW *win, attr_t attrs)
{
    if (!win) return ERR;
    win->attrs &= ~attrs;
    return OK;
}

int wattrset(WINDOW *win, attr_t attrs)
{
    if (!win) return ERR;
    win->attrs = attrs;
    return OK;
}

int attron(attr_t attrs)
{
    return wattron(stdscr, attrs);
}

int attroff(attr_t attrs)
{
    return wattroff(stdscr, attrs);
}

int attrset(attr_t attrs)
{
    return wattrset(stdscr, attrs);
}
