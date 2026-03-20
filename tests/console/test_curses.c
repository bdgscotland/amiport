/*
 * test_curses.c — Tests for amiport console-shim (ncurses subset)
 *
 * These tests verify the non-interactive parts of the console shim:
 * window allocation, attribute handling, buffer operations, and
 * cursor management. Interactive tests (getch, screen output) require
 * FS-UAE.
 */

#include "test_framework.h"
#include <amiport-console/curses.h>
#include <stdio.h>
#include <string.h>

static const char *verstag = "$VER: test_curses 1.0 (20.03.2026)";
long __stack = 32768;

TEST(initscr_creates_stdscr)
{
    WINDOW *win;

    win = initscr();
    ASSERT(win != NULL);
    ASSERT(stdscr != NULL);
    ASSERT(stdscr == win);
    ASSERT(LINES > 0);
    ASSERT(COLS > 0);

    endwin();
    ASSERT(stdscr == NULL);
}

TEST(window_dimensions)
{
    WINDOW *win;

    initscr();

    ASSERT_EQ(getmaxy(stdscr), LINES);
    ASSERT_EQ(getmaxx(stdscr), COLS);

    endwin();
}

TEST(newwin_basic)
{
    WINDOW *sub;

    initscr();

    sub = newwin(10, 40, 2, 5);
    ASSERT(sub != NULL);
    ASSERT_EQ(getmaxy(sub), 10);
    ASSERT_EQ(getmaxx(sub), 40);

    delwin(sub);
    endwin();
}

TEST(move_cursor)
{
    int y, x;

    initscr();

    ASSERT_EQ(move(5, 10), OK);
    getyx(stdscr, y, x);
    ASSERT_EQ(y, 5);
    ASSERT_EQ(x, 10);

    /* Out of bounds should fail */
    ASSERT_EQ(move(-1, 0), ERR);
    ASSERT_EQ(move(LINES + 1, 0), ERR);

    endwin();
}

TEST(attributes)
{
    initscr();

    ASSERT_EQ(attrset(A_NORMAL), OK);
    ASSERT_EQ(stdscr->attrs, A_NORMAL);

    ASSERT_EQ(attron(A_BOLD), OK);
    ASSERT(stdscr->attrs & A_BOLD);

    ASSERT_EQ(attron(A_UNDERLINE), OK);
    ASSERT(stdscr->attrs & A_BOLD);
    ASSERT(stdscr->attrs & A_UNDERLINE);

    ASSERT_EQ(attroff(A_BOLD), OK);
    ASSERT(!(stdscr->attrs & A_BOLD));
    ASSERT(stdscr->attrs & A_UNDERLINE);

    endwin();
}

TEST(color_pair)
{
    initscr();

    ASSERT_EQ(start_color(), OK);
    ASSERT_EQ(has_colors(), TRUE);
    ASSERT_EQ(init_pair(1, COLOR_RED, COLOR_BLACK), OK);

    /* Verify COLOR_PAIR macro */
    {
        chtype cp = COLOR_PAIR(1);
        ASSERT(cp != 0);
        ASSERT_EQ(PAIR_NUMBER(cp), 1);
    }

    endwin();
}

TEST(addch_advances_cursor)
{
    int y, x;

    initscr();
    move(0, 0);

    addch('A');
    getyx(stdscr, y, x);
    ASSERT_EQ(y, 0);
    ASSERT_EQ(x, 1);

    addch('B');
    getyx(stdscr, y, x);
    ASSERT_EQ(y, 0);
    ASSERT_EQ(x, 2);

    endwin();
}

TEST(addstr_multiple_chars)
{
    int y, x;

    initscr();
    move(0, 0);

    addstr("Hello");
    getyx(stdscr, y, x);
    ASSERT_EQ(y, 0);
    ASSERT_EQ(x, 5);

    endwin();
}

TEST(erase_clears_buffer)
{
    initscr();
    move(0, 0);
    addstr("Test data");

    ASSERT_EQ(erase(), OK);

    /* After erase, cursor should be at 0,0 */
    {
        int y, x;
        getyx(stdscr, y, x);
        ASSERT_EQ(y, 0);
        ASSERT_EQ(x, 0);
    }

    endwin();
}

TEST(scrollok_setting)
{
    initscr();

    ASSERT_EQ(stdscr->scrollok, 0);
    ASSERT_EQ(scrollok(stdscr, TRUE), OK);
    ASSERT_EQ(stdscr->scrollok, TRUE);
    ASSERT_EQ(scrollok(stdscr, FALSE), OK);
    ASSERT_EQ(stdscr->scrollok, FALSE);

    endwin();
}

TEST(window_attributes)
{
    WINDOW *win;

    initscr();

    win = newwin(10, 40, 0, 0);
    ASSERT(win != NULL);

    ASSERT_EQ(wattron(win, A_REVERSE), OK);
    ASSERT(win->attrs & A_REVERSE);

    ASSERT_EQ(wattroff(win, A_REVERSE), OK);
    ASSERT(!(win->attrs & A_REVERSE));

    ASSERT_EQ(wattrset(win, A_BOLD | A_UNDERLINE), OK);
    ASSERT(win->attrs & A_BOLD);
    ASSERT(win->attrs & A_UNDERLINE);

    delwin(win);
    endwin();
}

TEST(box_drawing)
{
    WINDOW *win;

    initscr();

    win = newwin(5, 10, 0, 0);
    ASSERT(win != NULL);

    ASSERT_EQ(box(win, 0, 0), OK);

    /* Verify corners are set in buffer */
    ASSERT((win->buf[0] & A_CHARTEXT) == '+');              /* top-left */
    ASSERT((win->buf[9] & A_CHARTEXT) == '+');              /* top-right */
    ASSERT((win->buf[4 * 10] & A_CHARTEXT) == '+');         /* bottom-left */
    ASSERT((win->buf[4 * 10 + 9] & A_CHARTEXT) == '+');    /* bottom-right */

    /* Verify horizontal edges */
    ASSERT((win->buf[1] & A_CHARTEXT) == '-');
    /* Verify vertical edges */
    ASSERT((win->buf[10] & A_CHARTEXT) == '|');

    delwin(win);
    endwin();
}

TEST(term_capabilities)
{
    int err;

    ASSERT_EQ(setupterm(NULL, 1, &err), OK);
    ASSERT_EQ(err, 1);

    ASSERT_EQ(tigetnum("cols"), COLS);
    ASSERT_EQ(tigetnum("lines"), LINES);
    ASSERT_EQ(tigetnum("colors"), 8);
    ASSERT_EQ(tigetflag("am"), 1);
}

int main(void)
{
    (void)verstag;
    printf("=== Console Shim Tests ===\n");

    RUN_TEST(initscr_creates_stdscr);
    RUN_TEST(window_dimensions);
    RUN_TEST(newwin_basic);
    RUN_TEST(move_cursor);
    RUN_TEST(attributes);
    RUN_TEST(color_pair);
    RUN_TEST(addch_advances_cursor);
    RUN_TEST(addstr_multiple_chars);
    RUN_TEST(erase_clears_buffer);
    RUN_TEST(scrollok_setting);
    RUN_TEST(window_attributes);
    RUN_TEST(box_drawing);
    RUN_TEST(term_capabilities);

    return test_summary();
}
