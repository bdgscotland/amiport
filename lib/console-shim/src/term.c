/*
 * term.c — Terminal capability stubs for AmigaOS
 *
 * Provides minimal terminfo-like API. Returns fixed values since
 * Amiga console.device has known capabilities.
 */

#include <amiport-console/term.h>
#include <amiport-console/curses.h>

#include <stdio.h>
#include <string.h>

int setupterm(const char *term, int fildes, int *errret)
{
    (void)term;
    (void)fildes;

    /* Always succeed — we know our terminal */
    if (errret) *errret = 1;
    return OK;
}

int tigetnum(const char *capname)
{
    if (!capname) return -2;

    if (strcmp(capname, "cols") == 0) return COLS;
    if (strcmp(capname, "lines") == 0) return LINES;
    if (strcmp(capname, "colors") == 0) return 8;
    if (strcmp(capname, "pairs") == 0) return COLOR_PAIRS;

    return -1; /* Capability not found */
}

int tigetflag(const char *capname)
{
    if (!capname) return -1;

    if (strcmp(capname, "am") == 0)   return 1; /* Auto margins */
    if (strcmp(capname, "bce") == 0)  return 0; /* No back color erase */
    if (strcmp(capname, "hs") == 0)   return 0; /* No status line */
    if (strcmp(capname, "mir") == 0)  return 1; /* Move in insert mode ok */
    if (strcmp(capname, "msgr") == 0) return 1; /* Move in standout mode ok */

    return 0; /* Unknown = false */
}

char *tigetstr(const char *capname)
{
    static char buf[64];

    if (!capname) return (char *)-1;

    /* Cursor movement */
    if (strcmp(capname, "cup") == 0)   return "\033[%i%p1%d;%p2%dH";
    if (strcmp(capname, "cuu1") == 0)  return "\033[A";
    if (strcmp(capname, "cud1") == 0)  return "\033[B";
    if (strcmp(capname, "cuf1") == 0)  return "\033[C";
    if (strcmp(capname, "cub1") == 0)  return "\033[D";
    if (strcmp(capname, "home") == 0)  return "\033[H";

    /* Clearing */
    if (strcmp(capname, "clear") == 0) return "\033[2J\033[H";
    if (strcmp(capname, "el") == 0)    return "\033[K";
    if (strcmp(capname, "ed") == 0)    return "\033[J";

    /* Attributes */
    if (strcmp(capname, "bold") == 0)  return "\033[1m";
    if (strcmp(capname, "sgr0") == 0)  return "\033[0m";
    if (strcmp(capname, "smul") == 0)  return "\033[4m";
    if (strcmp(capname, "rmul") == 0)  return "\033[24m";
    if (strcmp(capname, "rev") == 0)   return "\033[7m";

    /* Cursor visibility */
    if (strcmp(capname, "civis") == 0) return "\033[?25l";
    if (strcmp(capname, "cnorm") == 0) return "\033[?25h";

    /* Color */
    if (strcmp(capname, "setaf") == 0) {
        snprintf(buf, sizeof(buf), "\033[3%%p1%%dm");
        return buf;
    }
    if (strcmp(capname, "setab") == 0) {
        snprintf(buf, sizeof(buf), "\033[4%%p1%%dm");
        return buf;
    }

    return (char *)-1; /* Not found */
}

int tputs(const char *str, int affcnt, int (*putc_func)(int))
{
    (void)affcnt;

    if (!str || str == (char *)-1) return ERR;

    while (*str) {
        if (putc_func) {
            putc_func((int)(unsigned char)*str);
        } else {
            putchar((int)(unsigned char)*str);
        }
        str++;
    }
    return OK;
}

char *tparm(const char *str, ...)
{
    /* Simplified: just return the string unchanged.
     * Real tparm processes % parameters — for Amiga console
     * we generate escape sequences directly rather than via terminfo. */
    static char result[256];
    if (!str || str == (char *)-1) return "";
    strncpy(result, str, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';
    return result;
}
