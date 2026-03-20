/*
 * color.c — ANSI color support for AmigaOS console
 *
 * Maps ncurses color pairs to ANSI SGR sequences.
 * Amiga console.device supports standard 8 ANSI colors.
 */

#include <amiport-console/curses.h>

/* Color pair storage (shared with output.c via extern) */
static struct { short fg, bg; } color_pairs_storage[COLOR_PAIRS];
static int colors_started = 0;

/* Provide external linkage for output.c */
/* Note: output.c has its own copy — in a real build these would be unified.
 * For now, start_color/init_pair work through the ANSI sequence emission
 * in the attribute system. */

int start_color(void)
{
    int i;
    colors_started = 1;

    /* Initialize all pairs to default (white on black) */
    for (i = 0; i < COLOR_PAIRS; i++) {
        color_pairs_storage[i].fg = COLOR_WHITE;
        color_pairs_storage[i].bg = COLOR_BLACK;
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

    color_pairs_storage[pair].fg = f;
    color_pairs_storage[pair].bg = b;

    return OK;
}
