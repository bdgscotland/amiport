/*
 * color.c — ANSI color support for AmigaOS console
 *
 * Maps ncurses color pairs to ANSI SGR sequences.
 * Amiga console.device supports standard 8 ANSI colors.
 */

#include <amiport-console/curses.h>

/* Color pair storage — shared with output.c via extern linkage */
struct amiport_color_pair { short fg, bg; };
struct amiport_color_pair amiport_color_pairs[COLOR_PAIRS];
static int colors_started = 0;

int start_color(void)
{
    int i;
    colors_started = 1;

    /* Initialize all pairs to default (white on black) */
    for (i = 0; i < COLOR_PAIRS; i++) {
        amiport_color_pairs[i].fg = COLOR_WHITE;
        amiport_color_pairs[i].bg = COLOR_BLACK;
    }

    return OK;
}

int has_colors(void)
{
    /* Amiga console.device always supports ANSI colors */
    return TRUE;
}

int init_pair(short pair, short f, short b)
{
    if (pair < 0 || pair >= COLOR_PAIRS) return ERR;
    if (!colors_started) return ERR;

    amiport_color_pairs[pair].fg = f;
    amiport_color_pairs[pair].bg = b;

    return OK;
}
